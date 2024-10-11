#version 430 core

in vec3 SurfaceP;
in vec3 SurfaceN;
in vec2 UV;

uniform bool UsingTexture;
uniform sampler2D DiffuseTexture;
uniform sampler2D SpecularTexture;

uniform vec3 Ambient;
uniform vec4 Diffuse;
uniform vec4 Specular;

uniform vec3 CameraP;
uniform vec3 LightP;
uniform vec3 LightColor;

out vec4 Result;
void main()
{
	vec3 SurfaceToLight = LightP - SurfaceP;
	vec3 SurfaceToCamera = CameraP - SurfaceP;

	vec3 N = normalize(SurfaceN);
	vec3 SurfaceToLightNormalized = normalize(SurfaceToLight);
	vec3 SurfaceToCameraNormalized = normalize(SurfaceToCamera);
	vec3 ReflectedDirection = reflect(-SurfaceToCameraNormalized, N);

	float D = max(dot(N, SurfaceToLightNormalized), 0.0);
	float S = pow(max(dot(SurfaceToLightNormalized, ReflectedDirection), 0.0), 32);

	vec3 MaterialDiffuse = vec3(0.0);
	vec3 MaterialSpecular = vec3(0.0);
	if(UsingTexture)
	{
		MaterialDiffuse = D * LightColor * texture(DiffuseTexture, UV).rgb;
		MaterialSpecular = S * LightColor * texture(SpecularTexture, UV).rgb;
	}
	else
	{
		MaterialDiffuse = D * LightColor * Diffuse.xyz;
		MaterialSpecular = S * LightColor * Specular.xyz;
	}

	Result = vec4(Ambient + MaterialDiffuse + MaterialSpecular, 1.0);
}
