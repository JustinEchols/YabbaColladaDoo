#version 430 core

in vec3 TangentP;
in vec3 TangentLightP;
in vec3 TangentCameraP;
in vec3 SurfaceP;
in vec3 SurfaceN;
in vec3 LightP;
in vec3 CameraP;
in vec2 UV;

uniform bool UsingTexture;
uniform bool UsingDiffuse;
uniform bool UsingSpecular;
uniform bool UsingNormal;

uniform sampler2D DiffuseTexture;
uniform sampler2D SpecularTexture;
uniform sampler2D NormalTexture;

uniform vec3 Ambient;
uniform vec4 Diffuse;
uniform vec4 Specular;
uniform vec3 LightColor;

out vec4 Result;
void main()
{
	vec3 SurfaceToLight = vec3(0.0);
	vec3 SurfaceToCamera = vec3(0.0);
	vec3 N = vec3(0.0);
	if(UsingNormal)
	{
		SurfaceToLight = TangentLightP - TangentP;
		SurfaceToCamera = TangentCameraP - TangentP;
		vec3 Normal = texture(NormalTexture, UV).rgb;
		N = normalize(2.0 * Normal - vec3(1.0));
	}
	else
	{
		SurfaceToLight = LightP - SurfaceP;
		SurfaceToCamera = CameraP - SurfaceP;
		N = normalize(SurfaceN);
	}

	vec3 SurfaceToLightNormalized = normalize(SurfaceToLight);
	vec3 SurfaceToCameraNormalized = normalize(SurfaceToCamera);
	vec3 ReflectedDirection = reflect(-SurfaceToCameraNormalized, N);

	float D = max(dot(N, SurfaceToLightNormalized), 0.0);
	float S = pow(max(dot(SurfaceToLightNormalized, ReflectedDirection), 0.0), 32);

	vec3 MaterialDiffuse = vec3(0.0);
	vec3 MaterialSpecular = vec3(0.0);
	if(UsingDiffuse)
	{
		MaterialDiffuse = D * LightColor * texture(DiffuseTexture, UV).rgb;

	}
	else
	{
		MaterialDiffuse = D * LightColor * Diffuse.xyz;

	}

	if(UsingSpecular)
	{
		MaterialSpecular = S * LightColor * texture(SpecularTexture, UV).rgb;
	}
	else
	{
		MaterialSpecular = S * LightColor * Specular.xyz;
	}

	Result = vec4(Ambient + MaterialDiffuse + MaterialSpecular, 1.0);
}
