#version 430 core
layout (location = 0) in vec3 P;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 Tex;
layout (location = 3) in vec3 Tangent;
layout (location = 4) in vec3 BiTangent;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

uniform bool UsingTexture;
uniform vec3 LightP;

out vec3 N;
out vec3 TangentP;
out vec3 TangentLightP;
out vec2 UV;

void main()
{
	N = vec3(0.0);
	TangentP = vec3(0.0);
	TangentLightP = vec3(0.0);
	mat3 NormalMatrix = transpose(inverse(mat3(Model)));
	if(UsingTexture)
	{
		vec3 T = normalize(NormalMatrix * Tangent);
		vec3 B = normalize(NormalMatrix * BiTangent);
		vec3 N = normalize(NormalMatrix * Normal);
		mat3 TBN = transpose(mat3(T, B, N));
		TangentP = TBN * vec3(Model * vec4(P, 1.0));
		TangentLightP = TBN * LightP;
	}
	else
	{
		N = NormalMatrix * Normal;
	}

	gl_Position = Projection * View * Model * vec4(P, 1.0);
	UV = Tex;
}
