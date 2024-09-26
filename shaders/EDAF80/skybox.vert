#version 410

layout (location = 0) in vec3 aPos;

out vec3 textCoords;

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

void main()
{
    textCoords = vec3(aPos.x, aPos.y, -aPos.z); //Test 
    vec4 pos = vertex_world_to_clip * vertex_model_to_world * vec4(aPos, 1.0f);
    gl_Position = vec4(pos.x, pos.y, pos.w, pos.w);
    
}