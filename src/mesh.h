#if !defined(MESH_H)

// NOTE(Justin): A vertex is set to be affected by AT MOST 3 joints;

// NOTE(Justin): The pose transform of a joint is at index PoseTransformIndex in the uniform array

// NOTE(Justin): PosTransform is in model space. This is the transform that is sent to the
// shader in a uniform array and is one of the transforms used to calculate the new model space position of the vertex in the shader
// When calculating the final model space transform of the joint we use the
// PoseTransforIndex to put the transform into the array of transforms that is
// sent to the GPU.
//
// This is also the same index we can use to get the inverse rest pose transform for the joint
// assuming that the file contains this data

// NOTE(Justin): LocalResPoseTransform is in parent space. This is the transform that tells us what
// the joint is supposed to be when the model is at rest but it is in terms
// of the parent space. It is mainly used to calculate the inverse rest pose
// transform. 

struct joint_info
{
	u32 Count;
	u32 JointIndex[3];
	f32 Weights[3];
};

struct joint
{
	string Name;

	s32 ParentIndex;

	s32 Index;

	mat4 *Transform;
};

// TODO(Justin): Do i care about the joint name? Can just store and index
struct animation_info
{
	string JointName;
	s32 JointIndex;

	u32 TimeCount;
	f32 *Times;

	u32 TransformCount;
	mat4 *AnimationTransforms;
};

// NOTE(Justin): Vertex data in 1-1 correspondence is
// P
// N
// UV
// JointInfo

struct mesh
{
	basis Basis;

	u32 PositionsCount;
	u32 NormalsCount;
	u32 UVCount;
	u32 IndicesCount;
	u32 WeightCount;

	f32 *Positions;
	f32 *Normals;
	f32 *UV;
	u32 *Indices;
	f32 *Weights;

	// NOTE(Justin): The bind poses are a mat4 and are stored by rows. So the
	// first 16 floats represents a mat4 moreoever this is the inverse bind
	// matrix for the first node in JointNames which is the root. 

	u32 JointCount;
	u32 JointInfoCount;
	u32 AnimationInfoCount;

	string *JointNames;
	joint *Joints;
	joint_info *JointsInfo;

	mat4 *InvBindTransforms;
	mat4 *JointTransforms;
	mat4 *ModelSpaceTransforms;

	animation_info *AnimationsInfo;
};

#if 0
internal mat4
RestPoseGet(mesh *Mesh, u32 Index)
{
	if(Index >= 0 && (Index < Mesh->RestPosTransformCount))
}
#endif

#define MESH_H
#endif
