#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>

class QOpenGLShaderProgram;

class GlWidget final : public QOpenGLWidget, private QOpenGLFunctions
{
    Q_OBJECT;

public:
    GlWidget(QWidget *parent_p = nullptr);
    ~GlWidget() override;

    void setModel(const QString& modelPath);
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

    QMatrix4x4 m_projectionMatrix;

    GLfloat *m_modelData_p;
    int m_modelVertexCount;

    QOpenGLShaderProgram *m_program_p;

    QString m_vertexSource;
    QString m_fragmentSource;
    bool m_shadersChanged;

    int m_uMvp;
    int m_aPos;
};
