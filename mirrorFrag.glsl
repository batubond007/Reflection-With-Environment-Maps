#version 460 core

in vec3 eye;
in vec3 normal;
in vec4 pos;
in vec4 color;

uniform samplerCube ourTexture;

out vec4 FragColor;

void main()
{
    vec3 dir = normalize(pos.xyz - eye);
    vec3 texCoord = reflect(dir, normalize(normal));
    FragColor = mix(texture(ourTexture, texCoord), color, 0.3);
}