#include "GlWidget.h"

#include <QOpenGLShaderProgram>

//=============================================================================
GlWidget::GlWidget(QWidget *parent_p) : QOpenGLWidget(parent_p),
        m_program_p(nullptr),
        m_shadersChanged(false),
        m_uMvp(-1),
        m_aPos(-1)
{
    // TODO: Set the QSurfaceFormat here.
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
void GlWidget::initializeGL()
{
    initializeOpenGLFunctions();
    connect(context(), &QOpenGLContext::aboutToBeDestroyed,
        this, &GlWidget::cleanup);

    if(m_shadersChanged) buildShaders();

    glClearColor(1.0, 0.0, 0.0, 1.0);
    // TODO
}

//=============================================================================
void GlWidget::resizeGL(int w, int h)
{
    // NOTE: No GL calls in here!
    // TODO: Recalculate the projection matrix.
    (void)w;
    (void)h;
}

//=============================================================================
void GlWidget::paintGL()
{
    if(m_shadersChanged) buildShaders();
    if(!m_program_p) return;
    m_program_p->bind();

    glClear(GL_COLOR_BUFFER_BIT);

    QMatrix4x4 mvp;
    m_program_p->setUniformValue(m_uMvp, mvp);

    // TODO: Draw dynamically loaded model.
    GLfloat vertices[] = {
         0.0f,  0.707f,
        -0.5f, -0.5f,
         0.5f, -0.5f
    };
    glVertexAttribPointer(m_aPos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(m_aPos);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisableVertexAttribArray(m_aPos);

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

    m_uMvp = m_program_p->uniformLocation("uMvp");
    m_aPos = m_program_p->attributeLocation("aPos");
}
