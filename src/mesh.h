#if !defined(MESH_H)

struct joint_info
{
	u32 Count;
	u32 JointIndex[3];
	f32 Weights[3];
};

// TODO(Justin): Do we really need Index?
struct joint
{
	string Name;
	s32 ParentIndex;
	s32 Index;
	mat4 Transform;
};

struct animation_info
{
	string JointName;
	s32 JointIndex;

	u32 TimeCount;
	f32 *Times;

	u32 TransformCount;
	mat4 *Transforms;
};

struct material_spec
{
	v4 Ambient;
	v4 Diffuse;
	v4 Specular;

	f32 Shininess;
};

// NOTE(Justin): Vertex data in 1-1 correspondence is
//
// P
// N
// UV
// JointsInfo
//
// Skeletal data in 1-1 correspondence is
//
// JointNames
// InvBindTransforms
// JointTransforms
// ModelSpaceTransforms

// TODO(Justin): Think about removing Weights. They are already stored in
// joint_info for each vertex.
struct mesh
{
	string Name;

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

	u32 JointCount;
	u32 JointInfoCount;
	u32 AnimationInfoCount;

	string *JointNames;
	joint *Joints;
	joint_info *JointsInfo;

	mat4 BindTransform;
	mat4 *InvBindTransforms;
	mat4 *JointTransforms;
	mat4 *ModelSpaceTransforms;

	animation_info *AnimationsInfo;

	material_spec MaterialSpec;
};

struct model
{
	basis Basis;

	u32 MeshCount;
	mesh *Meshes;

	// TODO(Justin): OpenGL info here.

	//u32 AnimationInfoCount;
	//animation_info *AnimationsInfo;
};

#define MESH_H
#endif
