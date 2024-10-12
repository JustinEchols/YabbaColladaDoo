// WARNING(Justin): The max loop count must be a CONSTANT we cannot pass the
// joint count in and use it to cap the number of iterations of a loop.

// WARNING(Justin): If the compiler and linker determines that an attribute is
// not used IT WILL OPTIMIZE IT OUT OF THE PIPELINE AND CHAOS ENSUES.
// To detect this, everytime an attribute is initialized a counter can be
// incremented. After initializing all attributes use glGetProgramiv with
// GL_ACTIVE_ATTRIBUTES and ASSERT that the return value is equal to the
// counter.

#version 430 core
layout (location = 0) in vec3 P;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 Tex;
layout (location = 3) in uint JointCount;
layout (location = 4) in uvec3 JointTransformIndices;
layout (location = 5) in vec3 Weights;
layout (location = 6) in vec3 Tangent;
layout (location = 7) in vec3 BiTangent;

uniform bool UsingTexture;
uniform vec3 LightPosition;
uniform vec3 CameraPosition;
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

#define MAX_JOINT_COUNT 100
uniform mat4 Transforms[MAX_JOINT_COUNT];

out vec3 TangentP;
out vec3 TangentLightP;
out vec3 TangentCameraP;
out vec3 SurfaceP;
out vec3 SurfaceN;
out vec3 LightP;
out vec3 CameraP;
out vec2 UV;

void main()
{
	vec4 Pos = vec4(0.0);
	vec4 Norm = vec4(0.0);
	for(uint i = 0; i < 3; ++i)
	{
		if(i < JointCount)
		{
			uint JointIndex = JointTransformIndices[i];
			float Weight = Weights[i];
			mat4 Xform = Transforms[JointIndex];
			Pos += Weight * Xform * vec4(P, 1.0);
			Norm += Weight * Xform * vec4(Normal, 0.0);
		}
	}

	TangentP = vec3(0.0);
	TangentLightP = vec3(0.0);
	TangentCameraP = vec3(0.0);
	SurfaceP = vec3(0.0);
	SurfaceN = vec3(0.0);
	LightP = vec3(0.0);
	CameraP = vec3(0.0);
	mat3 NormalMatrix = transpose(inverse(mat3(Model)));

	if(UsingTexture)
	{
		vec3 T = normalize(NormalMatrix * Tangent);
		vec3 B = normalize(NormalMatrix * BiTangent);
		vec3 N = normalize(NormalMatrix * Norm.xyz);
		mat3 TBN = transpose(mat3(T, B, N));
		TangentP = TBN * vec3(Model * Pos);
		TangentLightP = TBN * LightPosition;
		TangentCameraP = TBN * CameraPosition;
	}
	else
	{
		SurfaceP = vec3(Model * Pos);
		SurfaceN = NormalMatrix * Norm.xyz;
		LightP = LightPosition;
		CameraP = CameraPosition;
	}

	gl_Position = Projection * View * Model * Pos;
	UV = Tex;
}
