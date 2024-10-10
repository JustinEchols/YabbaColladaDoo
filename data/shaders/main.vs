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

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

#define MAX_JOINT_COUNT 100
uniform mat4 Transforms[MAX_JOINT_COUNT];

out vec3 SurfaceP;
out vec3 SurfaceN;
out vec2 UV;

void main()
{
	vec4 Pos = vec4(0.0);
	vec4 Norm = vec4(0.0);
	for(uint i = 0; i < 3; ++i)
	{
		if(i < JointCount)
		{
			uint JIndex = JointTransformIndices[i];
			float W = Weights[i];
			mat4 T = Transforms[JIndex];
			Pos += W * T * vec4(P, 1.0);
			Norm += W * T * vec4(Normal, 0.0);
		}
	}

	Norm = normalize(Norm);

	gl_Position = Projection * View * Model * Pos;
	SurfaceP = vec3(Model * Pos);
	SurfaceN = vec3(transpose(inverse(Model)) * vec4(Norm.xyz, 1.0));
	UV = Tex;
}
