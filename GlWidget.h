#include <QOpenGLWidget>

class GlWidget : public QOpenGLWidget {
    Q_OBJECT;

public:
    GlWidget(QWidget *parent_p = nullptr) : QOpenGLWidget(parent_p) {}

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
};
