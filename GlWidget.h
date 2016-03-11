#include <QOpenGLFunctions>
#include <QOpenGLWidget>

class GlWidget final : public QOpenGLWidget, private QOpenGLFunctions {
    Q_OBJECT;

public:
    GlWidget(QWidget *parent_p = nullptr);
    ~GlWidget() override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private slots:
    void cleanup();
};
