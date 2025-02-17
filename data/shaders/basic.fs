#version 430 core
in vec3 N;
in vec3 TangentP;
in vec3 TangentLightP;
in vec3 LightPos;
in vec3 SurfaceP;
in vec2 UV;

uniform bool UsingDiffuse;
uniform bool UsingNormal;

uniform sampler2D DiffuseTexture;
uniform sampler2D NormalTexture;

uniform vec3 Diffuse;

out vec4 Result;
void main()
{
	vec3 SurfaceToLight = vec3(0.0);
	vec3 SurfaceN = vec3(0.0);
	if(UsingNormal)
	{
		SurfaceToLight = TangentLightP - TangentP;
		SurfaceN = texture(NormalTexture, UV).rgb;
	}
	else
	{
		vec3 SurfaceToLight = LightPos - SurfaceP;
		vec3 SurfaceN = N;
	}

	vec3 SurfaceToLightNormalized = normalize(SurfaceToLight);
	vec3 Normal = normalize(2.0 * SurfaceN - vec3(1.0));

	float D = max(dot(SurfaceToLightNormalized, Normal), 0.0);

	vec3 MaterialDiffuse = vec3(0.0);
	if(UsingDiffuse)
	{
		MaterialDiffuse = D * texture(DiffuseTexture, UV).rgb;
	}
	else
	{
		MaterialDiffuse = Diffuse;
	}

	Result = vec4(MaterialDiffuse, 1.0);
}
