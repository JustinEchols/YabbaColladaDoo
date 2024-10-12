#version 430 core
in vec3 N;
in vec3 TangentP;
in vec3 TangentLightP;
in vec2 UV;

uniform bool UsingTexture;
uniform sampler2D DiffuseTexture;
uniform sampler2D NormalTexture;

uniform vec3 Diffuse;

out vec4 Result;
void main()
{
	vec3 SurfaceToLight = TangentLightP - TangentP;
	vec3 SurfaceN = texture(NormalTexture, UV).rgb;
	if(UsingTexture)
	{
		SurfaceToLight = TangentLightP - TangentP;
		SurfaceN = texture(NormalTexture, UV).rgb;
	}

	vec3 SurfaceToLightNormalized = normalize(SurfaceToLight);
	vec3 Normal = normalize(2.0 * SurfaceN - vec3(1.0));
	float D = max(dot(SurfaceToLightNormalized, Normal), 0.0);

	vec3 MaterialDiffuse = vec3(0.0);
	if(UsingTexture)
	{
		MaterialDiffuse = D * texture(DiffuseTexture, UV).rgb;
	}
	else
	{
		MaterialDiffuse = Diffuse;
	}
	Result = vec4(MaterialDiffuse, 1.0);
}
