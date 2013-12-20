#version 110
uniform sampler2D tex[2];
void main()
{
    vec2 y_coord = gl_TexCoord[0].xy;
    vec2 cb_coord = vec2(y_coord.x, y_coord.y/2.0);
    vec2 cr_coord = vec2(y_coord.x, y_coord.y/2.0 + 0.5);
    float y = texture2D(tex[0], y_coord).r;
    float cb = texture2D(tex[1], cb_coord).r - 0.5;
    float cr = texture2D(tex[1], cr_coord).r - 0.5;

    gl_FragColor = vec4(
        y + 1.4021 * cr,
        y - 0.34482 * cb - 0.71405 * cr,
        y + 1.7713 * cb,
        1.0);
}
