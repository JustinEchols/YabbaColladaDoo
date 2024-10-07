#version 430 core

in vec3 N;
in vec2 UV;

uniform sampler2D Texture;
//uniform vec4 Color;

out vec4 Result;
void main()
{
	Result = 0.01 * N + texture(Texture, UV);
	//Result = Color * 0.01 * vec4(N, 1.0) * vec4(UV.x, UV.y, 0, 1);
}
