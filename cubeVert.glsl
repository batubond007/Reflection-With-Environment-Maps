#version 460 core

uniform mat4 viewingMatrix;
uniform mat4 projectionMatrix;

layout(location=0) in vec3 inVertex;

out vec3 TexCoords;

void main(void)
{ 
    gl_Position = projectionMatrix * viewingMatrix * vec4(inVertex, 1) - vec4(0,0,-1,0);
    TexCoords = inVertex;
}  

