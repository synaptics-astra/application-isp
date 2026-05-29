#ifndef CAMERA_RENDERNODE_H
#define CAMERA_RENDERNODE_H

#include <QSGRenderNode>
#include <QOpenGLFunctions>
#include "camera_texture.h"
#include <QOpenGLShaderProgram>

class CameraRenderNode : public QSGRenderNode, protected QOpenGLFunctions
{
public:
    CameraRenderNode(CameraTexture *texture);

    RenderingFlags flags() const override;
    QRectF rect() const override;
    void render(const RenderState *state) override;

private:
    CameraTexture *m_texture;

    QOpenGLShaderProgram *m_program;
    GLuint m_vbo;
};

#endif
