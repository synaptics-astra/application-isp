#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

varying highp vec2 qt_TexCoord;
uniform lowp float opacity;

uniform sampler2D qt_Texture_luma;
uniform sampler2D qt_Texture_chroma;

const mat3 yuv2rgb = mat3(
        1, 0, 1.13983,
        1, -0.39465, -0.58060,
        1, 2.03211, 0
        );

void main()
{
    vec3 yuv = vec3(
            1.1643 * (texture2D(qt_Texture_luma, qt_TexCoord).r - 0.0625),
            texture2D(qt_Texture_chroma, qt_TexCoord).r - 0.5,
            texture2D(qt_Texture_chroma, qt_TexCoord).a - 0.5
            );
    vec3 rgb = yuv * yuv2rgb;

    gl_FragColor = vec4(rgb, 1.0) * opacity;

}
