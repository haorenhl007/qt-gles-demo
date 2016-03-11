#include "GlWidget.h"

//=============================================================================
GlWidget::GlWidget(QWidget *parent_p) : QOpenGLWidget(parent_p) {
    // TODO: Set the QSurfaceFormat here.
}

//=============================================================================
GlWidget::~GlWidget() {
    cleanup();
}

//=============================================================================
void GlWidget::initializeGL() {
    initializeOpenGLFunctions();
    connect(context(), &QOpenGLContext::aboutToBeDestroyed,
        this, &GlWidget::cleanup);

    glClearColor(1.0, 0.0, 0.0, 1.0);
    // TODO
}

//=============================================================================
void GlWidget::resizeGL(int w, int h) {
    // NOTE: No GL calls in here!
    // TODO: Recalculate the projection matrix.
    (void)w;
    (void)h;
}

//=============================================================================
void GlWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);
    // TODO
}

//=============================================================================
void GlWidget::cleanup() {
    makeCurrent();
    // TODO: Clean up GL resources here.
    doneCurrent();
}
