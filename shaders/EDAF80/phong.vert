#version 410 core

// Vertex attributes
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 textures;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

// Uniforms
uniform mat4 normal_model_to_world;     
uniform mat4 vertex_model_to_world;     
uniform mat4 vertex_world_to_clip;     
uniform vec3 camera_position;           // Camera position (for specular reflections)
// Outputs to fragment shader
out vec3 FragPos;           // Position in world space
out vec3 Normal;            // Normal in world space
out vec3 ViewDir;           // View direction (camera position - fragment position)
out vec3 textCoords;
out vec3 Tangent;
out vec3 Bitangent;
void main()
{
    // Transform the vertex position to world space
    FragPos = vec3(vertex_model_to_world * vec4(position, 1.0));

    // Pass the normal in world space
    Normal = (normal_model_to_world * vec4(normal, 0.0)).xyz; 

    Tangent = (normal_model_to_world * vec4(tangent, 0.0)).xyz; 

    Bitangent = (normal_model_to_world * vec4(bitangent, 0.0)).xyz; 

    // Calculate view direction (camera position - fragment position)
    ViewDir = normalize(camera_position - FragPos);

    textCoords = textures;

    // Transform the position to clip space for rasterization
    gl_Position = vertex_world_to_clip * vec4(FragPos, 1.0);
}
