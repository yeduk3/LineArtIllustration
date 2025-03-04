#version 330 core

out vec4 out_Color;

in vec2 texCoords;

uniform sampler2D pdTexture;

void main()
{
    vec4 texColor = texture(pdTexture, texCoords);
    if (texColor.w == 1)
    {
        texColor = vec4(1, 1, 1, 1);
    }
    out_Color = texColor;
}