#version 330 core

out vec4 FragColor;
in vec4 vertexColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;

uniform float sineTime;

void main()
{
 FragColor = texture(ourTexture, TexCoord) * sineTime * vertexColor;
}

