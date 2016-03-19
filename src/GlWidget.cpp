#include "GlWidget.h"

#include <QFile>
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QRegularExpression>

#include "ModelTools.h"

//=============================================================================
GlWidget::GlWidget(QWidget *parent_p) : QOpenGLWidget(parent_p),
        m_enableFaceCulling(true),
        m_enableDepthTesting(true),
        m_enableFacetedRender(false),
        m_enableVisibleNormals(false),
        m_mouseActive(false),
        m_cameraDistance(20.0),
        m_cameraAngleX(15.0),
        m_cameraAngleZ(-45.0),
        m_aspectRatio(1.0),
        m_projection(Projection::PERSPECTIVE),
        m_modelChanged(false),
        m_modelBuffer(0),
        m_modelVertexCount(0),
        m_texture_p(nullptr),
        m_gridBuffer(0),
        m_gridVertexCount(0),
        m_gridTexture_p(nullptr),
        m_arrowBuffer(0),
        m_arrowVertexCount(0),
        m_arrowTexture_p(nullptr),
        m_program_p(nullptr),
        m_ornamentProgram_p(nullptr),
        m_shadersChanged(false)
{
    updateViewMatrix();
}

//=============================================================================
GlWidget::~GlWidget()
{
    cleanup();
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
void GlWidget::setModel(const QString& modelPath)
{
    m_modelPath = modelPath;
    m_modelChanged = true;
    update();
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
void GlWidget::enableVisibleNormals(bool enable)
{
    m_enableVisibleNormals = enable;
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
void GlWidget::wheelEvent(QWheelEvent *event_p)
{
    const double degrees = (event_p->angleDelta().y() / 15);
    m_cameraDistance -= 0.25 * degrees;
    updateViewMatrix();
}

//=============================================================================
void GlWidget::initializeGL()
{
    initializeOpenGLFunctions();
    connect(context(), &QOpenGLContext::aboutToBeDestroyed,
        this, &GlWidget::cleanup);

    if(m_shadersChanged) buildShaders();
    buildOrnamentShaders();

    loadOrnaments();

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//=============================================================================
void GlWidget::resizeGL(int w, int h)
{
    if(h > 0) m_aspectRatio = w / (double)h;
    updateProjectionMatrix();
}

//=============================================================================
void GlWidget::paintGL()
{
    if(m_shadersChanged) buildShaders();
    if(m_modelChanged) loadModel();

    glDepthMask(GL_TRUE);
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

        if(m_modelBuffer) {
            glBindBuffer(GL_ARRAY_BUFFER, m_modelBuffer);

            if(m_vars.aPosition >= 0) {
                const intptr_t offset = POSITION_OFFSET;
                glVertexAttribPointer(m_vars.aPosition,
                        3, GL_FLOAT, GL_FALSE, STRIDE, (void *)offset);
                glEnableVertexAttribArray(m_vars.aPosition);
            }

            if(m_vars.aNormal >= 0) {
                const intptr_t offset = m_enableFacetedRender ?
                        FACE_NORMAL_OFFSET : NORMAL_OFFSET;
                glVertexAttribPointer(m_vars.aNormal,
                        3, GL_FLOAT, GL_FALSE, STRIDE, (void *)offset);
                glEnableVertexAttribArray(m_vars.aNormal);
            }

            if(m_vars.aTextureCoord >= 0) {
                const intptr_t offset = TEXTURE_COORD_OFFSET;
                glVertexAttribPointer(m_vars.aTextureCoord,
                        2, GL_FLOAT, GL_FALSE, STRIDE, (void *)offset);
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

            if(m_vars.aPosition >= 0) {
                glDisableVertexAttribArray(m_vars.aPosition);
            }

            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        m_program_p->release();
    }

    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    if(m_ornamentProgram_p) {
        m_ornamentProgram_p->bind();

        m_ornamentProgram_p->setUniformValue(
                m_ornamentVars.uModel, QMatrix4x4());
        m_ornamentProgram_p->setUniformValue(
                m_ornamentVars.uView, m_viewMatrix);
        m_ornamentProgram_p->setUniformValue(
                m_ornamentVars.uProjection, m_projectionMatrix);

        glBindBuffer(GL_ARRAY_BUFFER, m_gridBuffer);

        if(m_ornamentVars.aPosition >= 0) {
            const intptr_t offset = POSITION_OFFSET;
            glVertexAttribPointer(m_ornamentVars.aPosition,
                    3, GL_FLOAT, GL_FALSE, STRIDE, (void *)offset);
            glEnableVertexAttribArray(m_ornamentVars.aPosition);
        }

        if(m_ornamentVars.aTextureCoord >= 0) {
            const intptr_t offset = TEXTURE_COORD_OFFSET;
            glVertexAttribPointer(m_ornamentVars.aTextureCoord,
                    2, GL_FLOAT, GL_FALSE, STRIDE, (void *)offset);
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

        if(m_ornamentVars.aPosition >= 0) {
            glDisableVertexAttribArray(m_ornamentVars.aPosition);
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        if(m_enableVisibleNormals) drawNormals();

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

    glDeleteBuffers(1, &m_modelBuffer);
    m_modelBuffer = 0;
    glDeleteBuffers(1, &m_gridBuffer);
    m_gridBuffer = 0;
    glDeleteBuffers(1, &m_arrowBuffer);
    m_arrowBuffer = 0;

    delete m_texture_p;
    m_texture_p = nullptr;
    delete m_gridTexture_p;
    m_gridTexture_p = nullptr;
    delete m_arrowTexture_p;
    m_arrowTexture_p = nullptr;

    doneCurrent();
}

//=============================================================================
void GlWidget::updateViewMatrix()
{
    m_viewMatrix = QMatrix4x4();
    m_viewMatrix.rotate(-90.0f, 1.0f, 0.0f, 0.0f);
    m_viewMatrix.translate(0.0f, m_cameraDistance, 0.0f);
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
void GlWidget::loadModel()
{
    m_modelChanged = false;
    if(m_modelPath.isNull()) return;

    // ==== Load model data ====
    QFile file(m_modelPath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit notify(QString("Could not open file \"%1\"").arg(m_modelPath));
        return;
    }
    QTextStream stream(&file);
    PlyModel ply = PlyModel::parse(stream);
    if(!ply.isValid()) {
        emit notify(QString("Invalid PLY file \"%1\"").arg(m_modelPath));
        return;
    }
    GLfloat *data_p = convertPly(ply, &m_modelVertexCount);

    glDeleteBuffers(1, &m_modelBuffer);
    glGenBuffers(1, &m_modelBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_modelBuffer);
    glBufferData(GL_ARRAY_BUFFER,
            (m_modelVertexCount * STRIDE), data_p, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    loadNormals(data_p, m_modelVertexCount);

    delete[] data_p;

    // ==== Load texture ====
    QString texturePath = m_modelPath;
    (void)texturePath.replace(QRegularExpression("\\.[Pp][Ll][Yy]$"),
            "-texture.png");
    delete m_texture_p;
    m_texture_p = new QOpenGLTexture(QImage(texturePath).mirrored());

    // TODO: ==== Load normal map ====
}

//=============================================================================
void GlWidget::loadOrnaments()
{
    // ==== Grid ====
    {
        GLfloat *data_p = makeGrid(10, 10, &m_gridVertexCount);
        glDeleteBuffers(1, &m_gridBuffer);
        glGenBuffers(1, &m_gridBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_gridBuffer);
        glBufferData(GL_ARRAY_BUFFER,
                (m_gridVertexCount * STRIDE), data_p, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        delete data_p;

        delete m_gridTexture_p;
        m_gridTexture_p = new QOpenGLTexture(
                QImage(":/grid-texture.png").mirrored());
        m_gridTexture_p->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_gridTexture_p->setMagnificationFilter(QOpenGLTexture::Linear);
    }

    // ==== Arrow ====
    {
        QFile file(":/arrow.ply");
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
        QTextStream stream(&file);
        PlyModel ply = PlyModel::parse(stream);
        if(!ply.isValid()) return;
        GLfloat *data_p = convertPly(ply, &m_arrowVertexCount);

        glDeleteBuffers(1, &m_arrowBuffer);
        glGenBuffers(1, &m_arrowBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_arrowBuffer);
        glBufferData(GL_ARRAY_BUFFER,
                (m_arrowVertexCount * STRIDE), data_p, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        delete[] data_p;

        delete m_arrowTexture_p;
        QImage image(":/arrow-texture.png");
        m_arrowTexture_p = new QOpenGLTexture(image.mirrored());

    }
}

//=============================================================================
void GlWidget::loadNormals(GLfloat *data_p, int vertexCount)
{
    for(int i = 0; i < vertexCount; ++i) {
        auto v = data_p + (i * NUM_VERTEX_VALUES);
        QVector3D position(v[0], v[1], v[2]);

        const QVector3D up(0.0f, 0.0f, 1.0f);

        QVector3D smoothNormal(v[3], v[4], v[5]);
        QMatrix4x4 smoothTransform;
        smoothTransform.translate(position);
        smoothTransform.rotate(QQuaternion::rotationTo(up, smoothNormal));
        m_smoothArrows.append(smoothTransform);

        QVector3D facetedNormal(v[6], v[7], v[8]);
        QMatrix4x4 facetedTransform;
        facetedTransform.translate(position);
        facetedTransform.rotate(QQuaternion::rotationTo(up, facetedNormal));
        m_facetedArrows.append(facetedTransform);
    }
}

//=============================================================================
void GlWidget::drawNormals()
{
    glBindBuffer(GL_ARRAY_BUFFER, m_arrowBuffer);

    if(m_ornamentVars.aPosition >= 0) {
        const intptr_t offset = POSITION_OFFSET;
        glVertexAttribPointer(m_ornamentVars.aPosition,
                3, GL_FLOAT, GL_FALSE, STRIDE, (void *)offset);
        glEnableVertexAttribArray(m_ornamentVars.aPosition);
    }

    if(m_ornamentVars.aTextureCoord >= 0) {
        const intptr_t offset = TEXTURE_COORD_OFFSET;
        glVertexAttribPointer(m_ornamentVars.aTextureCoord,
                2, GL_FLOAT, GL_FALSE, STRIDE, (void *)offset);
        glEnableVertexAttribArray(m_ornamentVars.aTextureCoord);
    }

    if(m_arrowTexture_p && m_ornamentVars.uTexture >= 0) {
        constexpr int textureUnit = 0;
        m_arrowTexture_p->bind(textureUnit);
        glUniform1i(m_ornamentVars.uTexture, textureUnit);
    }

    const auto& arrowTransforms =
            m_enableFacetedRender ? m_facetedArrows : m_smoothArrows;
    for(auto transform : arrowTransforms) {
        m_ornamentProgram_p->setUniformValue(
                m_ornamentVars.uModel, m_modelMatrix * transform);
        glDrawArrays(GL_TRIANGLES, 0, m_arrowVertexCount);
    }

    if(m_ornamentVars.aTextureCoord >= 0) {
        glDisableVertexAttribArray(m_ornamentVars.aTextureCoord);
    }

    if(m_ornamentVars.aPosition >= 0) {
        glDisableVertexAttribArray(m_ornamentVars.aPosition);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
