#version 430 core

in vec3 SurfaceP;
in vec3 SurfaceN;
in vec2 UV;

uniform bool UsingTexture;
uniform sampler2D DiffuseTexture;
uniform vec4 Diffuse;
uniform vec3 LightP;
uniform vec3 LightColor;

out vec4 Result;
void main()
{
	vec3 N = normalize(SurfaceN);
	vec3 SurfaceToLight = normalize(LightP - SurfaceP);
	float D = max(dot(N, SurfaceToLight), 0.0);

	vec3 MaterialDiffuse = vec3(0.0);
	if(UsingTexture)
	{
		MaterialDiffuse = D * LightColor * texture(DiffuseTexture, UV).rgb;
	}
	else
	{
		MaterialDiffuse = D * LightColor * Diffuse;
	}

	Result = vec4(MaterialDiffuse, 1.0);
}
