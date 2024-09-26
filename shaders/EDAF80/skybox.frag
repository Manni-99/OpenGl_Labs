#version 410

in vec3 textCoords;
out vec4 FragColor;

uniform samplerCube skybox;

void main()
{
    FragColor = texture(skybox, textCoords);
}