#version 430 core
in vec3 N;
in vec2 UV;

uniform sampler2D Texture;

out vec4 Result;
void main()
{
	vec3 MaterialDiffuse = 0.1 * N + texture(Texture, UV).rgb;
	Result = vec4(MaterialDiffuse, 1.0);
}
