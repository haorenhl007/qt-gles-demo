#include "GlWidget.h"

#include <QFile>
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QRegularExpression>

#include "ModelTools.h"

//=============================================================================
GlWidget::GlWidget(QWidget *parent_p) : QOpenGLWidget(parent_p),
        m_mouseActive(false),
        m_cameraAngleX(15.0),
        m_cameraAngleZ(-45.0),
        m_aspectRatio(1.0),
        m_projection(Projection::PERSPECTIVE),
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
        m_ornamentProgram_p(nullptr),
        m_shadersChanged(false)
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
void GlWidget::setProjection(Projection p)
{
    m_projection = p;
    updateProjectionMatrix();
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
    buildOrnamentShaders();

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
    if(h > 0) {
        m_aspectRatio = w / (double)h;
    }
    updateProjectionMatrix();
}

//=============================================================================
void GlWidget::paintGL()
{
    if(m_shadersChanged) buildShaders();
    if(m_modelChanged) prepareModel();

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

    if(m_program_p) {
        m_program_p->bind();

        m_program_p->setUniformValue(m_vars.uModel, m_modelMatrix);
        m_program_p->setUniformValue(m_vars.uView, m_viewMatrix);
        m_program_p->setUniformValue(m_vars.uProjection, m_projectionMatrix);
        m_program_p->setUniformValue(m_vars.uNormalMatrix,
                (m_viewMatrix * m_modelMatrix).inverted().transposed());

        if(m_modelData_p) {
            glVertexAttribPointer(m_vars.aPosition,
                    3, GL_FLOAT, GL_FALSE, STRIDE, m_modelData_p);
            glEnableVertexAttribArray(m_vars.aPosition);

            if(m_vars.aNormal >= 0) {
                GLfloat *data_p = m_modelData_p +
                        (m_enableFacetedRender ? 6 : 3);
                glVertexAttribPointer(m_vars.aNormal,
                        3, GL_FLOAT, GL_FALSE, STRIDE, data_p);
                glEnableVertexAttribArray(m_vars.aNormal);
            }

            if(m_vars.aTextureCoord >= 0) {
                glVertexAttribPointer(m_vars.aTextureCoord,
                        2, GL_FLOAT, GL_FALSE, STRIDE, m_modelData_p+9);
                glEnableVertexAttribArray(m_vars.aTextureCoord);
            }

            if(m_texture_p && m_vars.uTexture >= 0) {
                constexpr int textureUnit = 0;
                m_texture_p->bind(textureUnit);
                glUniform1i(m_vars.uTexture, textureUnit);
            }

            glDrawArrays(GL_TRIANGLES, 0, m_modelVertexCount);

            if(m_vars.aTextureCoord >= 0) {
                glDisableVertexAttribArray(m_vars.aTextureCoord);
            }

            if(m_vars.aNormal >= 0) {
                glDisableVertexAttribArray(m_vars.aNormal);
            }

            glDisableVertexAttribArray(m_vars.aPosition);
        }

        m_program_p->release();
    }

    glDisable(GL_CULL_FACE);

    if(m_ornamentProgram_p) {
        m_ornamentProgram_p->bind();

        m_ornamentProgram_p->setUniformValue(
                m_ornamentVars.uModel, QMatrix4x4());
        m_ornamentProgram_p->setUniformValue(
                m_ornamentVars.uView, m_viewMatrix);
        m_ornamentProgram_p->setUniformValue(
                m_ornamentVars.uProjection, m_projectionMatrix);

        glVertexAttribPointer(m_ornamentVars.aPosition,
                3, GL_FLOAT, GL_FALSE, STRIDE, m_gridData_p);
        glEnableVertexAttribArray(m_ornamentVars.aPosition);

        if(m_ornamentVars.aTextureCoord >= 0) {
            glVertexAttribPointer(m_ornamentVars.aTextureCoord,
                    2, GL_FLOAT, GL_FALSE, STRIDE, m_gridData_p+9);
            glEnableVertexAttribArray(m_ornamentVars.aTextureCoord);
        }

        if(m_gridTexture_p && m_ornamentVars.uTexture >= 0) {
            constexpr int textureUnit = 0;
            m_gridTexture_p->bind(textureUnit);
            glUniform1i(m_ornamentVars.uTexture, textureUnit);
        }

        glDrawArrays(GL_TRIANGLES, 0, m_gridVertexCount);

        if(m_ornamentVars.aTextureCoord >= 0) {
            glDisableVertexAttribArray(m_ornamentVars.aTextureCoord);
        }

        glDisableVertexAttribArray(m_ornamentVars.aPosition);

        m_ornamentProgram_p->release();
    }
}

//=============================================================================
void GlWidget::cleanup()
{
    makeCurrent();

    delete m_program_p;
    m_program_p = nullptr;

    delete m_ornamentProgram_p;
    m_ornamentProgram_p = nullptr;

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
void GlWidget::updateProjectionMatrix()
{
    m_projectionMatrix = QMatrix4x4();

    switch(m_projection) {
    case Projection::ORTHOGRAPHIC:
        m_projectionMatrix.ortho(
                -10*m_aspectRatio, 10*m_aspectRatio, // left, right
                -10, 10, // bottom, top
                0.01f, 100.0f); // near, far
        break;
    case Projection::PERSPECTIVE:
        m_projectionMatrix.perspective(60, m_aspectRatio, 0.01f, 100.0f);
        break;
    }

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

    m_vars.uModel = program_p->uniformLocation("uModel");
    m_vars.uView = program_p->uniformLocation("uView");
    m_vars.uProjection = program_p->uniformLocation("uProjection");
    m_vars.uNormalMatrix = program_p->uniformLocation("uNormalMatrix");
    m_vars.uTexture = program_p->uniformLocation("uTexture");
    m_vars.aPosition = program_p->attributeLocation("aPosition");
    m_vars.aNormal = program_p->attributeLocation("aNormal");
    m_vars.aTextureCoord = program_p->attributeLocation("aTextureCoord");
}

//=============================================================================
void GlWidget::buildOrnamentShaders()
{
    auto program_p = new QOpenGLShaderProgram(this);
    (void)program_p->addShaderFromSourceFile(
            QOpenGLShader::Vertex, ":/ornaments.vert");
    (void)program_p->addShaderFromSourceFile(
            QOpenGLShader::Fragment, ":/ornaments.frag");
    (void)program_p->link();

    delete m_ornamentProgram_p;
    m_ornamentProgram_p = program_p;

    m_ornamentVars.uModel = program_p->uniformLocation("uModel");
    m_ornamentVars.uView = program_p->uniformLocation("uView");
    m_ornamentVars.uProjection = program_p->uniformLocation("uProjection");
    m_ornamentVars.uTexture = program_p->uniformLocation("uTexture");
    m_ornamentVars.aPosition = program_p->attributeLocation("aPosition");
    m_ornamentVars.aTextureCoord =
            program_p->attributeLocation("aTextureCoord");
}

//=============================================================================
void GlWidget::prepareModel()
{
    m_modelChanged = false;

    delete m_texture_p;
    m_texture_p = new QOpenGLTexture(m_textureData.mirrored());
}
