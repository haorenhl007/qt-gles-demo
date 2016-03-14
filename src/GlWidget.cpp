#include "GlWidget.h"

#include <QFile>
#include <QOpenGLShaderProgram>

#include "Ply/PlyModel.h"

constexpr int numVertexValues = 11;
constexpr int stride = numVertexValues * sizeof(GLfloat);

//=============================================================================
static GLfloat *convertPly(PlyModel model, int *vertexCountOut)
{
    struct Vertex
    {
        double x;
        double y;
        double z;
        double nx;
        double ny;
        double nz;
        double s;
        double t;
    };

    QSet<QString> elements = model.elements();
    if(!elements.contains("vertex")) return nullptr;
    if(!elements.contains("face")) return nullptr;

    QSet<QString> vertexProperties = model.scalarProperties("vertex");
    if(!vertexProperties.contains("x")) return nullptr;
    if(!vertexProperties.contains("y")) return nullptr;
    if(!vertexProperties.contains("z")) return nullptr;
    if(!vertexProperties.contains("nx")) return nullptr;
    if(!vertexProperties.contains("ny")) return nullptr;
    if(!vertexProperties.contains("nz")) return nullptr;
    if(!vertexProperties.contains("s")) return nullptr;
    if(!vertexProperties.contains("t")) return nullptr;

    QSet<QString> faceProperties = model.listProperties("face");
    if(!faceProperties.contains("vertex_indices")) return nullptr;

    QList<Vertex> verts;
    const int vertexCount = model.count("vertex");
    for(int v = 0; v < vertexCount; ++v) {
        Vertex vert;
        vert.x = model.scalarValue("vertex", v, "x");
        vert.y = model.scalarValue("vertex", v, "y");
        vert.z = model.scalarValue("vertex", v, "z");
        vert.nx = model.scalarValue("vertex", v, "nx");
        vert.ny = model.scalarValue("vertex", v, "ny");
        vert.nz = model.scalarValue("vertex", v, "nz");
        vert.s = model.scalarValue("vertex", v, "s");
        vert.t = model.scalarValue("vertex", v, "t");
        verts.append(vert);
    }

    QList<double> face_verts;
    const int faceCount = model.count("face");
    for(int f = 0; f < faceCount; ++f) {
        QList<double> indices = model.listValue("face", f, "vertex_indices");
        if(indices.count() != 3) return nullptr;

        QVector3D faceNormal;
        {
            Vertex v1 = verts.value(indices.value(0));
            Vertex v2 = verts.value(indices.value(1));
            Vertex v3 = verts.value(indices.value(2));
            QVector3D p1(v1.x, v1.y, v1.z);
            QVector3D p2(v2.x, v2.y, v2.z);
            QVector3D p3(v3.x, v3.y, v3.z);
            faceNormal = QVector3D::crossProduct((p2 - p1), (p3 - p2));
        }

        for(int i = 0; i < indices.count(); ++i) {
            int v = (int)indices.value(i);
            if(v >= verts.count()) return nullptr;
            Vertex vert = verts.value(v);
            face_verts.append(vert.x);
            face_verts.append(vert.y);
            face_verts.append(vert.z);
            face_verts.append(vert.nx);
            face_verts.append(vert.ny);
            face_verts.append(vert.nz);
            face_verts.append(faceNormal.x());
            face_verts.append(faceNormal.y());
            face_verts.append(faceNormal.z());
            face_verts.append(vert.s);
            face_verts.append(vert.t);
        }
    }

    GLfloat *data_p = new GLfloat[face_verts.count()];
    for(int i = 0; i < face_verts.count(); ++i) {
        data_p[i] = face_verts.value(i);
    }
    if(vertexCountOut) *vertexCountOut = face_verts.count() / numVertexValues;
    return data_p;
}

//=============================================================================
GlWidget::GlWidget(QWidget *parent_p) : QOpenGLWidget(parent_p),
        m_modelData_p(nullptr),
        m_modelVertexCount(0),
        m_program_p(nullptr),
        m_shadersChanged(false),
        m_uModel(-1),
        m_uView(-1),
        m_uProjection(-1),
        m_aPosition(-1),
        m_aNormal(-1)
{
    QSurfaceFormat f = format();
    f.setDepthBufferSize(24);
    setFormat(f);
}

//=============================================================================
GlWidget::~GlWidget()
{
    cleanup();
    delete m_modelData_p;
}

//=============================================================================
void GlWidget::setModel(const QString& modelPath)
{
    QFile file(modelPath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // TODO: notify.
        qDebug("Could not open file \"%s\"", modelPath.toUtf8().constData());
        return;
    }
    QTextStream stream(&file);
    PlyModel ply = PlyModel::parse(stream);
    if(!ply.isValid()) {
        // TODO: notify.
        qDebug("Invalid PLY file \"%s\"", modelPath.toUtf8().constData());
        return;
    }
    delete m_modelData_p;
    m_modelData_p = convertPly(ply, &m_modelVertexCount);
}

//=============================================================================
void GlWidget::installShaders(const QString& vertexSource,
        const QString& fragmentSource)
{
    emit notify("Building shader program...");
    m_vertexSource = vertexSource;
    m_fragmentSource = fragmentSource;
    m_shadersChanged = true;
    update();
}

//=============================================================================
void GlWidget::initializeGL()
{
    initializeOpenGLFunctions();
    connect(context(), &QOpenGLContext::aboutToBeDestroyed,
        this, &GlWidget::cleanup);

    if(m_shadersChanged) buildShaders();

    glClearColor(1.0, 0.0, 0.0, 1.0);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
}

//=============================================================================
void GlWidget::resizeGL(int w, int h)
{
    m_projectionMatrix = QMatrix4x4();
    m_projectionMatrix.perspective(60, w/(float)h, 0.01f, 100.0f);
}

//=============================================================================
void GlWidget::paintGL()
{
    if(m_shadersChanged) buildShaders();
    if(!m_program_p) return;
    m_program_p->bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO: Let user control camera.
    QVector3D eye(20, 18, 10);
    QVector3D target(0, 0, 5);
    QVector3D up(0, 0, 1);
    QMatrix4x4 viewMatrix;
    viewMatrix.lookAt(eye, target, up);

    // TODO: Let user reposition model.
    QMatrix4x4 modelMatrix;

    m_program_p->setUniformValue(m_uModel, modelMatrix);
    m_program_p->setUniformValue(m_uView, viewMatrix);
    m_program_p->setUniformValue(m_uProjection, m_projectionMatrix);

    if(m_modelData_p) {
        glVertexAttribPointer(
                m_aPosition, 3, GL_FLOAT, GL_FALSE, stride, m_modelData_p);
        glEnableVertexAttribArray(m_aPosition);

        if(m_aNormal >= 0) {
            glVertexAttribPointer(
                    m_aNormal, 3, GL_FLOAT, GL_FALSE, stride, m_modelData_p+6);
            glEnableVertexAttribArray(m_aNormal);
        }

        glDrawArrays(GL_TRIANGLES, 0, m_modelVertexCount);

        if(m_aNormal >= 0) glDisableVertexAttribArray(m_aNormal);
        glDisableVertexAttribArray(m_aPosition);
    }

    m_program_p->release();
}

//=============================================================================
void GlWidget::cleanup()
{
    makeCurrent();
    // TODO: Clean up GL resources here.
    doneCurrent();
}

//=============================================================================
void GlWidget::buildShaders()
{
    m_shadersChanged = false;

    auto program_p = new QOpenGLShaderProgram(this);
    if(!program_p->addShaderFromSourceCode(
            QOpenGLShader::Vertex, m_vertexSource)) {
        emit notify(program_p->log());
        delete program_p;
        return;
    }
    if(!program_p->addShaderFromSourceCode(
            QOpenGLShader::Fragment, m_fragmentSource)) {
        emit notify(program_p->log());
        delete program_p;
        return;
    }
    if(!program_p->link()) {
        emit notify(program_p->log());
        delete program_p;
        return;
    }
    delete m_program_p;
    m_program_p = program_p;
    emit notify("Shader program built successfully!");

    m_uModel = m_program_p->uniformLocation("uModel");
    m_uView = m_program_p->uniformLocation("uView");
    m_uProjection = m_program_p->uniformLocation("uProjection");
    m_aPosition = m_program_p->attributeLocation("aPosition");
    m_aNormal = m_program_p->attributeLocation("aNormal");
}
