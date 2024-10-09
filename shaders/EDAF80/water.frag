#version 410 core

uniform vec3 light_position;
uniform vec3 camera_position;
uniform samplerCube skybox;
uniform sampler2D water;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
    vec3 texCoord;
    vec3 nbump;
} fs_in;

out vec4 frag_color;

void main()
{
    vec4 waterDeep = vec4(0.0, 0.0, 0.1, 1.0);  // Deep water color (dark blue)
    vec4 waterShallow = vec4(0.0, 0.5, 0.5, 1.0);  // Shallow water color (turquoise)

    // Calculate view direction (V)
    vec3 V = normalize(camera_position - fs_in.vertex); // Fixed the vertex - camera_position
    // Normal mapped normal
    vec3 N = fs_in.nbump;
    // Calculate the facing component
    float facing = 1.0 - max(dot(V, N), 0.0);

    // Incident vector (I) can be taken as the view vector (V)
    vec3 I = V; // Incident vector pointing towards the camera

    // Calculate the reflection
    vec3 R = reflect(-V, N);

    // Fresnel Effect
    float n1 = 1.0; // Refractive index of air
    float n2 = 1.33; // Refractive index of water
    float eta = n2 / n1; // Ratio of indices


    vec3 refractedDirection = refract(-I, N, eta);
    // R0 refractive factor
    float R0 = pow((n1 - n2) / (n1 + n2), 2.0);
    // Fresnel factor
    float fresnelFactor = R0 + (1.0 - R0) * pow(1.0 - dot(V, N), 5.0);
    
    vec4 reflectionColor = vec4(texture(skybox, R).rgb, 1.0);   

    // Refraction color
    vec4 refraction = vec4(texture(skybox, refractedDirection).rgb, 1.0);

    // Result to water color
    vec4 waterColor = mix(waterDeep, waterShallow, facing);

    // Result vector 
    vec4 result = waterColor + reflectionColor * fresnelFactor + refraction * (1 - fresnelFactor);

    // Combine lighting with water color
    frag_color = result;
} 

