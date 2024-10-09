#version 410 core
layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout (location = 2) in vec3 texCoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform float ellapsed_time_s;
uniform vec2 texScale = vec2(8, 4);
uniform vec2 normalSpeed = vec2(-0.05, 0.0);
uniform sampler2D water;
uniform float time;  // Added


out VS_OUT
{
	vec3 vertex;
	vec3 normal;
    vec3 texCoord;
    vec3 nbump;
}
vs_out;
// Wave function to calculate wave height at a given position
float wave(vec2 position, vec2 direction, float amplitude, float frequency,
		   float phase, float sharpness, float time)
{
	return amplitude * pow(sin((position.x * direction.x + position.y * direction.y) * frequency + phase * time) * 0.5 + 0.5, sharpness);
}
// Derivative for wave in the X direction
float derivateWaveX(vec2 position, vec2 direction, float amplitude, float frequency,
		   float phase, float sharpness, float time)
{
    float waveValue = wave(position, direction, amplitude, frequency, phase, sharpness-1, time);

	return  (0.5 * sharpness * frequency * amplitude * waveValue) * (cos((direction.x * position.x + direction.y * position.y) * frequency + time * phase) * direction.x);
}
// Derivative for wave in the Z direction
float derivateWaveZ(vec2 position, vec2 direction, float amplitude, float frequency,
		   float phase, float sharpness, float time)
{
    float waveValue = wave(position, direction, amplitude, frequency, phase, sharpness -1, time);
	return  (0.5 * sharpness * frequency * amplitude * waveValue) * (cos((direction.x * position.x + direction.y * position.y) * frequency + time * phase) * direction.y);
}

void main()
{
    
	vec3 displaced_vertex = vertex;
	displaced_vertex.y += wave(vertex.xy, vec2(-1.0, 0.0),      //Wave 1
                                 1.0f,
                                 0.2f,
                                 0.5f,
                                 2.0f,
                                 ellapsed_time_s);

    displaced_vertex.y += wave(vertex.xy, vec2(-0.7, 0.7),      //Wave 2
                                 0.5f,
                                 0.4f,
                                 1.3f,
                                 2.0f,
                                 ellapsed_time_s);
    float dWaveX = derivateWaveX(vertex.xz, vec2(-1.0, 0.0), 1.0f, 0.2f, 0.5f, 2.0f, ellapsed_time_s);
    float dWaveZ = derivateWaveZ(vertex.xz, vec2(-0.7, 0.7), 0.5f, 0.4f, 1.3f, 2.0f, ellapsed_time_s);

    float normalTime = mod(time, 100.0);
    vec2 normalCoord0 = texCoord.xy * texScale + normalTime * normalSpeed;
    vec2 normalCoord1 = texCoord.xy * texScale * 2.0 + normalTime * normalSpeed * 4.0;
    vec2 normalCoord2 = texCoord.xy * texScale * 4.0 + normalTime * normalSpeed * 8.0;
    
    vec3 ni0 = vec3(texture(water, normalCoord0).rgb * 2.0 - 1.0);
    vec3 ni1 = vec3(texture(water, normalCoord1).rgb * 2.0 - 1.0);
    vec3 ni2 = vec3(texture(water, normalCoord2).rgb * 2.0 - 1.0);


    // We'll implement the TBN matrix here, get our derivates x and z -> transform them and send it over to our frag for shading 
    vec3 T = normalize(vec3(1, dWaveX, 0));        
    vec3 B = normalize(vec3(0, dWaveZ, 1));
    vec3 N = normalize(cross(B, T));
    mat3 TBN = mat3(T, B, N);

    vec3 normalMap = texture(water, texCoord.xy).rgb;
    normalMap = normalize(normalMap * 2.0 - 1.0); 
    vec3 normal = normalize(TBN * normalMap);
    

	vs_out.vertex = vec3(vertex_model_to_world * vec4(displaced_vertex, 1.0));
    vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0)); // Normal mapping
    vs_out.texCoord = texCoord;

    vec3 nbump = normalize(ni0 + ni1 + ni2);
    
	vs_out.nbump = TBN * nbump;
    
//  vs_out.normal = vec3(normal_model_to_world * vec4(N, 0.0)); Regular normal 
    
	gl_Position = vertex_world_to_clip  * vec4(vs_out.vertex, 1.0);
}