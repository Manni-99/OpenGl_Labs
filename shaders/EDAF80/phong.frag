#version 410 core

// Inputs from vertex shader
in vec3 FragPos;   // Position in world space
in vec3 Normal;    // Normal in world space
in vec3 ViewDir;   // View direction
in vec3 textCoords; //Texture
in vec3 Tangent;   //Tangent
in vec3 Bitangent; //Bitangent
// Uniforms
uniform vec3 light_position;   
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;
uniform vec3 ambient_colour;
uniform float shininess_value;
uniform bool use_normal_mapping;

uniform sampler2D demo_diffuse_texture;
uniform sampler2D demo_specular_texture;
uniform sampler2D demo_normal_texture;
// Output color
out vec4 FragColor;

void main()
{
    vec3 ambient = ambient_colour;

    if(use_normal_mapping){
        vec3 T = normalize(Tangent);
        vec3 B = normalize(Bitangent);
        vec3 N = normalize(Normal);
        mat3 TBN = mat3(T, B, N);

        vec3 normalMap = texture(demo_normal_texture, textCoords.xy).rgb;
        normalMap = normalize(normalMap * 2.0 - 1.0); 
        vec3 normal = normalize(TBN * normalMap);

        vec3 light_dir = normalize(light_position - FragPos);   // Light direction
        float diff = max(dot(normal, light_dir), 0.0);          // Angle between light and normal
        vec3 diffuse = diff * texture(demo_diffuse_texture, textCoords.xy).rgb;

        // Specular lighting (Phong reflectance)
        vec3 reflect_dir = reflect(-light_dir, normal);                         // Reflect the light direction around the normal
        float spec = pow(max(dot(ViewDir, reflect_dir), 0.0), shininess_value); // Specular intensity
        vec3 specular = spec * texture(demo_specular_texture, textCoords.xy).rgb;
        // Combine all three components (ambient + diffuse + specular)
        vec3 result = (ambient + diffuse + specular);
        FragColor = vec4(result, 1.0);
    } else 
    {
        vec3 normal = normalize(Normal);

        vec3 light_dir = normalize(light_position - FragPos);   // Light direction
        float diff = max(dot(normal, light_dir), 0.0);          // Angle between light and normal
        vec3 diffuse = diff * texture(demo_diffuse_texture, textCoords.xy).rgb;

        // Specular lighting (Phong reflectance)
        vec3 reflect_dir = reflect(-light_dir, normal);                         // Reflect the light direction around the normal
        float spec = pow(max(dot(ViewDir, reflect_dir), 0.0), shininess_value); // Specular intensity
        vec3 specular = spec * texture(demo_specular_texture, textCoords.xy).rgb;

        // Combine all three components (ambient + diffuse + specular)
        vec3 result = (ambient + diffuse + specular);

        // Output the final color (with alpha value of 1.0)
        FragColor = vec4(result, 1.0);
    }
}
