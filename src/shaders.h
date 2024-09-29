#if !defined(SHADERS_H)

// WARNING(Justin): The max loop count must be a CONSTANT we cannot pass the
// joint count in and use it to cap the number of iterations of a loop.

// WARNING(Justin): If the compiler and linker determines that an attribute is
// not used IT WILL OPTIMIZE IT OUT OF THE PIPELINE AND CHAOS ENSUES.
// To detect this, everytime an attribute is initialized a counter can be
// incremented. After initializing all attributes use glGetProgramiv with
// GL_ACTIVE_ATTRIBUTES and ASSERT that the return value is equal to the
// counter.

char *BasicVsSrc = R"(
#version 430 core
layout (location = 0) in vec3 P;
layout (location = 1) in vec3 Normal;
layout (location = 2) in uint JointCount;
layout (location = 3) in uvec3 JointTransformIndices;
layout (location = 4) in vec3 Weights;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

#define MAX_JOINT_COUNT 70
uniform mat4 Transforms[MAX_JOINT_COUNT];

out vec3 N;

void main()
{
	vec4 Pos = vec4(0.0);
	for(uint i = 0; i < 3; ++i)
	{
		if(i < JointCount)
		{
			uint JIndex = JointTransformIndices[i];
			float W = Weights[i];
			mat4 T = Transforms[JIndex];
			Pos += W * T * vec4(P, 1.0);
		}
	}

	gl_Position = Projection * View * Model * Pos;
	N = vec3(transpose(inverse(Model)) * vec4(Normal, 1.0));
})";

char *BasicFsSrc = R"(
#version 430 core

in vec3 N;

uniform vec4 Diffuse;
uniform vec4 Specular;
uniform float Shininess;

uniform vec3 LightDir;
uniform vec3 CameraP;

out vec4 Result;
void main()
{
	vec3 LightColor = vec3(1.0);
	vec3 Normal = normalize(N);
	vec3 Direction = -1.0f * LightDir;
	float D = max(dot(Normal, LightDir), 0.0);

	vec3 Diff = D * LightColor * Diffuse.xyz;
	vec3 Spec = vec3(0.0);

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

	Result = vec4(Diff + Spec, 1.0);
})";

char *CubeVS = R"(
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
})";

char *CubeFS = R"(
#version 430 core

in vec3 N;
in vec2 UV;

uniform sampler2D Texture;

out vec4 Result;
void main()
{
	Result = 0.01 * N + texture(Texture, UV);
})";

#endif
