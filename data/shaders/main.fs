#version 430 core

in vec3 N;
in vec2 UV;

uniform vec4 Diffuse;
//uniform vec4 Specular;
//uniform float Shininess;

//uniform vec3 LightDir;
//uniform vec3 CameraP;

uniform bool UsingTexture;
uniform sampler2D Texture;

out vec4 Result;
void main()
{
	//vec3 LightColor = vec3(1.0);
	//vec3 Normal = normalize(N);
	//vec3 Direction = -1.0f * LightDir;
	//float D = max(dot(Normal, LightDir), 0.0);

	//vec3 Diff = D * LightColor * Diffuse.xyz;
	//vec3 Spec = vec3(0.0);

	//bool FacingTowards = dot(Normal, LightDir) > 0.0;

	//vec3 Diff = vec3(0.0);
	//vec3 Spec = vec3(0.0);
	//if(FacingTowards)
	//{
	//	float D = max(dot(Normal, LightDir), 0.0);
	//	Diff = LightColor * D * Diffuse.xyz;

	//	vec3 ViewDir = normalize(CameraP);
	//	vec3 R = reflect(LightDir, Normal);
	//	float S = pow(max(dot(ViewDir, R), 0.0), Shininess);
	//	Spec = LightColor * S * Specular.xyz;
	//}

	//Result = vec4(Diff + Spec, 1.0) * texture(Texture, UV);
	if(UsingTexture)
	{
		Result = 0.1 * N + texture(Texture, UV);
	}
	else
	{
		Result = 0.1 * vec4(N, 1.0f) + Diffuse;
	}
}
