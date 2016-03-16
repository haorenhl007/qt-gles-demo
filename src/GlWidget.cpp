#include "GlWidget.h"

#include <QFile>
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QRegularExpression>

#include "Ply/PlyModel.h"

constexpr int numVertexValues = 11;
constexpr int stride = numVertexValues * sizeof(GLfloat);

//=============================================================================
static GLfloat *makeGrid(int w, int h, int *vertexCountOut)
{
    struct Point
    {
        GLfloat x;
        GLfloat y;
    };
    const Point vertexOffsets[] = {
        { 0.0, 0.0 },
        { 1.0, 1.0 },
        { 0.0, 1.0 },
        { 0.0, 0.0 },
        { 1.0, 0.0 },
        { 1.0, 1.0 }
    };

    const Point textureCoords[] = {
        { 0.0, 0.0 },
        { 1.0, 1.0 },
        { 0.0, 1.0 },
        { 0.0, 0.0 },
        { 1.0, 0.0 },
        { 1.0, 1.0 }
    };

    const int cellCount = w * h;
    const int vertexCount = cellCount * 6;
    auto data_p = new GLfloat[numVertexValues * vertexCount];
    GLfloat *vertex_p = data_p;
    for(int y = -h/2; y < h/2; ++y) {
        for(int x = -w/2; x < w/2; ++x) {
            for(int i = 0; i < 6; ++i) {
                vertex_p[0] = 2*x + 2*vertexOffsets[i].x; // x
                vertex_p[1] = 2*y + 2*vertexOffsets[i].y; // y
                vertex_p[2] = 0.0f; // z
                vertex_p[3] = 0.0f; // nx
                vertex_p[4] = 0.0f; // ny
                vertex_p[5] = 1.0f; // nz
                vertex_p[6] = 0.0f; // fnx
                vertex_p[7] = 0.0f; // fny
                vertex_p[8] = 1.0f; // fnz
                vertex_p[9] = textureCoords[i].x; // s
                vertex_p[10] = textureCoords[i].y; // t
                vertex_p += numVertexValues;
            }
        }
    }
    if(vertexCountOut) *vertexCountOut = vertexCount;
    return data_p;
}

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
        m_mouseActive(false),
        m_cameraAngleX(15.0),
        m_cameraAngleZ(-45.0),
        m_enableFaceCulling(true),
        m_enableDepthTesting(true),
        m_enableFacetedRender(false),
        m_modelData_p(nullptr),
        m_modelVertexCount(0),
        m_texture_p(nullptr),
        m_modelChanged(false),
        m_gridData_p(nullptr),
        m_gridVertexCount(0),
        m_gridTexture_p(nullptr),
        m_program_p(nullptr),
        m_shadersChanged(false),
        m_uModel(-1),
        m_uView(-1),
        m_uProjection(-1),
        m_uTexture(-1),
        m_aPosition(-1),
        m_aNormal(-1),
        m_aTextureCoord(-1)
{
    QSurfaceFormat f = format();
    f.setDepthBufferSize(24);
    setFormat(f);

    m_gridData_p = makeGrid(10, 10, &m_gridVertexCount);
}

//=============================================================================
GlWidget::~GlWidget()
{
    cleanup();
    delete m_gridData_p;
    delete m_modelData_p;
}

//=============================================================================
void GlWidget::setModel(const QString& modelPath)
{
    QFile file(modelPath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        notify(QString("Could not open file \"%1\"").arg(modelPath));
        return;
    }
    QTextStream stream(&file);
    PlyModel ply = PlyModel::parse(stream);
    if(!ply.isValid()) {
        notify(QString("Invalid PLY file \"%1\"").arg(modelPath));
        return;
    }
    delete m_modelData_p;
    m_modelData_p = convertPly(ply, &m_modelVertexCount);

    QString texturePath = modelPath;
    (void)texturePath.replace(QRegularExpression("\\.[Pp][Ll][Yy]$"),
            "-texture.png");
    m_textureData = QImage(texturePath);

    // TODO: Attempt to load normal map.

    m_modelChanged = true;
}

//=============================================================================
void GlWidget::setModelAngle(int degrees)
{
    m_modelMatrix = QMatrix4x4();
    m_modelMatrix.rotate(degrees, 0.0f, 0.0f, 1.0f);
    update();
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
void GlWidget::enableFaceCulling(bool enable)
{
    m_enableFaceCulling = enable;
    update();
}

//=============================================================================
void GlWidget::enableDepthTesting(bool enable)
{
    m_enableDepthTesting = enable;
    update();
}

//=============================================================================
void GlWidget::enableFacetedRender(bool enable)
{
    m_enableFacetedRender = enable;
    update();
}

//=============================================================================
void GlWidget::mousePressEvent(QMouseEvent *event_p)
{
    if(event_p->button() == Qt::LeftButton) {
        m_lastMouse = event_p->pos();
        m_mouseActive = true;
    }
}

//=============================================================================
void GlWidget::mouseMoveEvent(QMouseEvent *event_p)
{
    if(m_mouseActive) {
        QPoint position = event_p->pos();
        m_cameraAngleZ += position.x() - m_lastMouse.x();
        m_cameraAngleX += position.y() - m_lastMouse.y();
        m_lastMouse = position;
        updateViewMatrix();
    }
}

//=============================================================================
void GlWidget::mouseReleaseEvent(QMouseEvent *)
{
    m_mouseActive = false;
}

//=============================================================================
void GlWidget::initializeGL()
{
    initializeOpenGLFunctions();
    connect(context(), &QOpenGLContext::aboutToBeDestroyed,
        this, &GlWidget::cleanup);

    if(m_shadersChanged) buildShaders();

    delete m_gridTexture_p;
    m_gridTexture_p = new QOpenGLTexture(
            QImage(":/grid-texture.png").mirrored());
    m_gridTexture_p->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    m_gridTexture_p->setMagnificationFilter(QOpenGLTexture::Linear);

    glClearColor(1.0, 1.0, 1.0, 1.0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    updateViewMatrix();
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
    if(m_modelChanged) prepareModel();
    if(!m_program_p) return;
    m_program_p->bind();

    if(m_enableDepthTesting) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    if(m_enableFaceCulling) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
    } else {
        glDisable(GL_CULL_FACE);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_program_p->setUniformValue(m_uView, m_viewMatrix);
    m_program_p->setUniformValue(m_uProjection, m_projectionMatrix);

    if(m_modelData_p) {
        m_program_p->setUniformValue(m_uModel, m_modelMatrix);

        glVertexAttribPointer(
                m_aPosition, 3, GL_FLOAT, GL_FALSE, stride, m_modelData_p);
        glEnableVertexAttribArray(m_aPosition);

        if(m_aNormal >= 0) {
            GLfloat *data_p = m_modelData_p + (m_enableFacetedRender ? 6 : 3);
            glVertexAttribPointer(
                    m_aNormal, 3, GL_FLOAT, GL_FALSE, stride, data_p);
            glEnableVertexAttribArray(m_aNormal);
        }

        if(m_aTextureCoord >= 0) {
            glVertexAttribPointer(
                    m_aTextureCoord, 2, GL_FLOAT, GL_FALSE, stride,
                    m_modelData_p+9);
            glEnableVertexAttribArray(m_aTextureCoord);
        }

        if(m_texture_p && m_uTexture >= 0) {
            constexpr int textureUnit = 0;
            m_texture_p->bind(textureUnit);
            glUniform1i(m_uTexture, textureUnit);
        }

        glDrawArrays(GL_TRIANGLES, 0, m_modelVertexCount);

        if(m_aTextureCoord >= 0) glDisableVertexAttribArray(m_aTextureCoord);
        if(m_aNormal >= 0) glDisableVertexAttribArray(m_aNormal);
        glDisableVertexAttribArray(m_aPosition);
    }

    {
        m_program_p->setUniformValue(m_uModel, QMatrix4x4());

        glVertexAttribPointer(
                m_aPosition, 3, GL_FLOAT, GL_FALSE, stride, m_gridData_p);
        glEnableVertexAttribArray(m_aPosition);

        if(m_aNormal >= 0) {
            glVertexAttribPointer(
                    m_aNormal, 3, GL_FLOAT, GL_FALSE, stride, m_gridData_p+3);
            glEnableVertexAttribArray(m_aNormal);
        }

        if(m_aTextureCoord >= 0) {
            glVertexAttribPointer(
                    m_aTextureCoord, 2, GL_FLOAT, GL_FALSE, stride,
                    m_gridData_p+9);
            glEnableVertexAttribArray(m_aTextureCoord);
        }

        if(m_gridTexture_p && m_uTexture >= 0) {
            constexpr int textureUnit = 0;
            m_gridTexture_p->bind(textureUnit);
            glUniform1i(m_uTexture, textureUnit);
        }

        glDrawArrays(GL_TRIANGLES, 0, m_gridVertexCount);

        if(m_aTextureCoord >= 0) glDisableVertexAttribArray(m_aTextureCoord);
        if(m_aNormal >= 0) glDisableVertexAttribArray(m_aNormal);
        glDisableVertexAttribArray(m_aPosition);
    }

    m_program_p->release();
}

//=============================================================================
void GlWidget::cleanup()
{
    makeCurrent();

    delete m_program_p;
    m_program_p = nullptr;

    delete m_gridTexture_p;
    m_gridTexture_p = nullptr;

    delete m_texture_p;
    m_texture_p = nullptr;

    doneCurrent();
}

//=============================================================================
void GlWidget::updateViewMatrix()
{
    m_viewMatrix = QMatrix4x4();
    m_viewMatrix.rotate(-90.0f, 1.0f, 0.0f, 0.0f);
    m_viewMatrix.translate(0.0f, 20.0f, 0.0f);
    m_viewMatrix.rotate(m_cameraAngleX, 1.0f, 0.0f, 0.0f);
    m_viewMatrix.rotate(m_cameraAngleZ, 0.0f, 0.0f, 1.0f);
    m_viewMatrix.translate(0.0f, 0.0f, -6.0f);
    update();
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
    m_uTexture = m_program_p->uniformLocation("uTexture");
    m_aPosition = m_program_p->attributeLocation("aPosition");
    m_aNormal = m_program_p->attributeLocation("aNormal");
    m_aTextureCoord = m_program_p->attributeLocation("aTextureCoord");
}

//=============================================================================
void GlWidget::prepareModel()
{
    m_modelChanged = false;

    delete m_texture_p;
    m_texture_p = new QOpenGLTexture(m_textureData.mirrored());
}
