#include <QOpenGLFunctions>
#include <QOpenGLWidget>

class QOpenGLShaderProgram;

class GlWidget final : public QOpenGLWidget, private QOpenGLFunctions
{
    Q_OBJECT;

public:
    GlWidget(QWidget *parent_p = nullptr);
    ~GlWidget() override;

    void installShaders(const QString& vertexSource,
            const QString& fragmentSource);

signals:
    void notify(const QString& text);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private slots:
    void cleanup();

private:
    void buildShaders();

    QOpenGLShaderProgram *m_program_p;

    QString m_vertexSource;
    QString m_fragmentSource;
    bool m_shadersChanged;
};
