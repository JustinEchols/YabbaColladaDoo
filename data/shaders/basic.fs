#version 430 core
in vec3 N;
in vec2 UV;

uniform sampler2D Texture;

out vec4 Result;
void main()
{
	Result = 0.1 * N + texture(Texture, UV);
}
