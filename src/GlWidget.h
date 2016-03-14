#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>

class QOpenGLShaderProgram;
class QOpenGLTexture;

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

public slots:
    void enableFaceCulling(bool enable);
    void enableDepthTesting(bool enable);
    void enableFacetedRender(bool enable);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private slots:
    void cleanup();

private:
    void buildShaders();
    void prepareModel();

    QMatrix4x4 m_projectionMatrix;

    bool m_enableFaceCulling;
    bool m_enableDepthTesting;
    bool m_enableFacetedRender;

    GLfloat *m_modelData_p;
    int m_modelVertexCount;
    QImage m_textureData;
    QOpenGLTexture *m_texture_p;
    bool m_modelChanged;

    QOpenGLShaderProgram *m_program_p;

    QString m_vertexSource;
    QString m_fragmentSource;
    bool m_shadersChanged;

    int m_uModel;
    int m_uView;
    int m_uProjection;
    int m_uTexture;
    int m_aPosition;
    int m_aNormal;
    int m_aTextureCoord;
};
