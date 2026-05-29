#include "camera_rendernode.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLShader>

CameraRenderNode::CameraRenderNode(CameraTexture *texture)
    : m_texture(texture),
      m_program(nullptr),
      m_vbo(0)
{
    initializeOpenGLFunctions();

    // Create shader
    m_program = new QOpenGLShaderProgram();

    // Vertex shader
    const char *vsrc =
        "attribute vec2 position;\n"
        "attribute vec2 texcoord;\n"
        "varying vec2 v_tex;\n"
        "void main() {\n"
        "   v_tex = texcoord;\n"
        "   gl_Position = vec4(position, 0.0, 1.0);\n"
        "}";

    // Fragment shader (simple texture)
    const char *fsrc =
        "precision mediump float;\n"
        "varying vec2 v_tex;\n"
        "uniform sampler2D tex;\n"
        "uniform sampler2D tex_uv;\n"

        "void main() {\n"
        "    float y = texture2D(tex, v_tex).r;\n"
        "    vec2 uv = texture2D(tex_uv, v_tex).ra;\n"

        "    float U = uv.x - 0.5;\n"
        "    float V = uv.y - 0.5;\n"

        "    float r = y + 1.402 * V;\n"
        "    float g = y - 0.344 * U - 0.714 * V;\n"
        "    float b = y + 1.772 * U;\n"

        "    gl_FragColor = vec4(r, g, b, 1.0);\n"
        "}\n";
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vsrc);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fsrc);
    m_program->link();

    // Fullscreen quad (position + texcoord)
    float vertices[] = {
        // x,   y,    u, v
        -1,  -1,   0, 1,
        -1,   1,   0, 0,
         1,  -1,   1, 1,
         1,   1,   1, 0
    };

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

QSGRenderNode::RenderingFlags CameraRenderNode::flags() const
{
    return BoundedRectRendering;
}

QRectF CameraRenderNode::rect() const
{
    QSize size = QOpenGLContext::currentContext()->surface()->size();
    return QRectF(0, 0, size.width(), size.height());
}

/*
void CameraRenderNode::render(const RenderState *)
{
    if (!m_texture || !m_program)
        return;

    glViewport(0, 0, 1920, 1080);
    glDisable(GL_DEPTH_TEST);

    // bind shader
    m_program->bind();

    // bind texture
    glActiveTexture(GL_TEXTURE0);
    m_texture->bind();
    m_program->setUniformValue("tex", 0);

    // bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    int posLoc = m_program->attributeLocation("position");
    int texLoc = m_program->attributeLocation("texcoord");

    m_program->enableAttributeArray(posLoc);
    m_program->enableAttributeArray(texLoc);

    m_program->setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 4 * sizeof(float));
    m_program->setAttributeBuffer(texLoc, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));

    // draw quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_program->disableAttributeArray(posLoc);
    m_program->disableAttributeArray(texLoc);

    m_program->release();
}
*/
void CameraRenderNode::render(const RenderState *)
{
    if (!m_texture || !m_program)
        return;


    QSize size = QOpenGLContext::currentContext()->surface()->size();
    glViewport(0, 0, size.width(), size.height());

    glDisable(GL_DEPTH_TEST);

    // ALWAYS clear first (GREEN fallback)
    glClearColor(0.0f, 0.6f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //bind shader
    m_program->bind();

    //bind texture
    glActiveTexture(GL_TEXTURE0);
    m_texture->bind();
    m_program->setUniformValue("tex", 0);
    m_program->setUniformValue("tex_uv", 1);
    // bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    int posLoc = m_program->attributeLocation("position");
    int texLoc = m_program->attributeLocation("texcoord");

    m_program->enableAttributeArray(posLoc);
    m_program->enableAttributeArray(texLoc);

    m_program->setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 4 * sizeof(float));
    m_program->setAttributeBuffer(texLoc, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));

    //draw quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_program->disableAttributeArray(posLoc);
    m_program->disableAttributeArray(texLoc);

    m_program->release();
}

