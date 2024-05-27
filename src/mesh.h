#if !defined(MESH_H)

// NOTE(Justin): A vertex is set to be affected by AT MOST 3 joints;
struct joint_info
{
	u32 Count;
	u32 JointIndex[3];
	u32 WeightIndex[3];
};

// NOTE(Justin): The pose transform of a joint is at index PoseTransformIndex in the uniform array

// NOTE(Justin): PosTransform is in model space. This is the transform that is sent to the
// shader in a uniform array and is one of the transforms used to calculate the new model space position of the vertex in the shader

// NOTE(Justin): LocalResPoseTransform is in parent space. This is the transform that tells us what
// the joint is supposed to be when the model is at rest but it is in terms
// of the parent space. It is mainly used to calculate the inverse rest pose
// transform. 
struct joint
{
	string Name;

	s32 ParentIndex;

	s32 PoseTransformIndex;
	f32 *PoseTransform;

	f32 *LocalRestPoseTransform;
	f32 *InverseRestPose;
};

// NOTE(Justin): The last f32 in the array of times gives the length of the
// animation.
// TODO(Justin): Animation name
struct animation_info
{
	string JointName;

	u32 TimeCount;
	f32 *Times;

	u32 JointTransformCount;
	f32 *JointTransforms;
};

// NOTE(Justin): Vertex data in 1-1 correspondence is
// P
// N
// UV
// JointInfo
//
struct mesh
{
	u32 PositionsCount;
	u32 NormalsCount;
	u32 UVCount;
	u32 IndicesCount;
	u32 WeightCount;
	u32 RestPosTransformCount;

	f32 *Positions;
	f32 *Normals;
	f32 *UV;
	u32 *Indices;
	f32 *Weights;

	// NOTE(Justin): The bind poses are a mat4 and are stored by rows. So the
	// first 16 floats represents a mat4 moreoever this is the inverse bind
	// matrix for the first node in JointNames which is the root. 
	f32 *RestPoseTransforms;

	u32 JointNameCount;
	u32 JointInfoCount;
	u32 JointCount;
	u32 AnimationInfoCount;

	string *JointNames;
	joint_info *JointsInfo;
	joint *Joints;
	animation_info *AnimationsInfo;
};

#define MESH_H
#endif
