#version 110

uniform sampler2D tex_0;

void main()
{
    gl_FragColor = gl_Color * vec4(1.0, 1.0, 1.0, texture2D(tex_0, gl_TexCoord[0].xy).r);
}
