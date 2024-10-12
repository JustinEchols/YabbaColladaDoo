#version 430 core

in vec3 TangentP;
in vec3 TangentLightP;
in vec2 UV;
in vec4 Color;

uniform sampler2D DiffuseTexture;
uniform sampler2D NormalTexture;

out vec4 Result;
void main()
{
	vec3 SurfaceToLight = TangentLightP - TangentP;
	vec3 Normal = texture(NormalTexture, UV).rgb;

	vec3 SurfaceToLightNormalized = normalize(SurfaceToLight);
	vec3 N = normalize(2.0 * Normal - vec3(1.0));

	float D = max(dot(SurfaceToLightNormalized, N), 0.0);

	vec3 MaterialDiffuse = D * Color.xyz * texture(DiffuseTexture, UV).rgb;
	Result = vec4(MaterialDiffuse, 1.0);
}
