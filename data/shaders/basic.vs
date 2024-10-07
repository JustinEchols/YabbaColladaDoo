#version 430 core
layout (location = 0) in vec3 P;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 Tex;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

out vec3 N;
out vec2 UV;

void main()
{
	gl_Position = Projection * View * Model * vec4(P, 1.0);
	N = vec3(transpose(inverse(Model)) * vec4(Normal, 1.0));
	UV = Tex;
}
