#include "GlWidget.h"

#include <QOpenGLShaderProgram>

//=============================================================================
GlWidget::GlWidget(QWidget *parent_p) : QOpenGLWidget(parent_p),
        m_program_p(nullptr),
        m_shadersChanged(false)
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
    // TODO
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
}
