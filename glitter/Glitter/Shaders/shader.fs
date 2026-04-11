#version 330 core

out vec4 FragColor;
in vec4 vertexColor;

uniform float sineTime;

void main()
{
 FragColor = vec4(vertexColor.xyz * sineTime, 1.0f);
}

