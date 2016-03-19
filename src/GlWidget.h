#pragma once

#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>

class QOpenGLShaderProgram;
class QOpenGLTexture;

enum class Projection
{
    ORTHOGRAPHIC,
    PERSPECTIVE
};

//=============================================================================
class GlWidget final : public QOpenGLWidget, private QOpenGLFunctions
{
    Q_OBJECT;

public:
    GlWidget(QWidget *parent_p = nullptr);
    ~GlWidget() override;

    void installShaders(const QString& vertexSource,
            const QString& fragmentSource);

    void setModel(const QString& modelPath);
    void setModelAngle(int degrees);
    void setProjection(Projection p);

signals:
    void notify(const QString& text);

public slots:
    void enableFaceCulling(bool enable);
    void enableDepthTesting(bool enable);
    void enableFacetedRender(bool enable);
    void enableVisibleNormals(bool enable);

protected:
    void mousePressEvent(QMouseEvent *event_p) override;
    void mouseMoveEvent(QMouseEvent *event_p) override;
    void mouseReleaseEvent(QMouseEvent *event_p) override;
    void wheelEvent(QWheelEvent *event_p) override;

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private slots:
    void cleanup();

private:
    void updateViewMatrix();
    void updateProjectionMatrix();
    void buildShaders();
    void buildOrnamentShaders();
    void loadModel();
    void loadOrnaments();
    void loadNormals(GLfloat *data_p, int vertexCount);
    void drawNormals();

    // ==== Misc. Options ====
    bool m_enableFaceCulling;
    bool m_enableDepthTesting;
    bool m_enableFacetedRender;
    bool m_enableVisibleNormals;

    // ==== View Matrix ====
    bool m_mouseActive;
    QPoint m_lastMouse;

    double m_cameraDistance;
    double m_cameraAngleX;
    double m_cameraAngleZ;
    QMatrix4x4 m_viewMatrix;

    // ==== Projection Matrix ====
    double m_aspectRatio;
    Projection m_projection;
    QMatrix4x4 m_projectionMatrix;

    // ==== Model ====
    QString m_modelPath;
    bool m_modelChanged;
    GLuint m_modelBuffer;
    int m_modelVertexCount;
    QOpenGLTexture *m_texture_p;

    QMatrix4x4 m_modelMatrix;

    // ==== Grid ====
    GLuint m_gridBuffer;
    int m_gridVertexCount;
    QOpenGLTexture *m_gridTexture_p;

    // ==== Arrows ====
    GLuint m_arrowBuffer;
    int m_arrowVertexCount;
    QOpenGLTexture *m_arrowTexture_p;

    QList<QMatrix4x4> m_smoothArrows;
    QList<QMatrix4x4> m_facetedArrows;

    // ==== Shaders ====
    struct ShaderVars {
        int uModel = -1;
        int uView = -1;
        int uProjection = -1;
        int uNormalMatrix = -1;
        int uTexture = -1;

        int aPosition = -1;
        int aNormal = -1;
        int aTextureCoord = -1;
    };

    QOpenGLShaderProgram *m_program_p;
    ShaderVars m_vars;

    QOpenGLShaderProgram *m_ornamentProgram_p;
    ShaderVars m_ornamentVars;

    QString m_vertexSource;
    QString m_fragmentSource;
    bool m_shadersChanged;
};
