#version 430 core
layout (location = 0) in vec3 P;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec3 Tangent;
layout (location = 3) in vec3 BiTangent;
layout (location = 4) in vec2 Tex;
layout (location = 5) in vec4 VertexColor;

uniform vec3 LightP;
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

out vec3 TangentP;
out vec3 TangentLightP;
out vec2 UV;
out vec4 Color;
void main()
{
	gl_Position = Projection * View * Model * vec4(P, 1.0);

	mat3 NormalMatrix = transpose(inverse(mat3(Model)));
	vec3 T = normalize(NormalMatrix * Tangent);
	vec3 B = normalize(NormalMatrix * BiTangent);
	vec3 N = normalize(NormalMatrix * Normal);
	mat3 TBN = transpose(mat3(T, B, N));

	TangentP = TBN * vec3(Model * vec4(P, 1.0));
	TangentLightP = TBN * LightP;

	UV = Tex;
	Color = VertexColor;
}
