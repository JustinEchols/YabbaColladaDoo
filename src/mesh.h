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
	mat4 Transform;
};

// NOTE(Justin): For now TimeCount and TransformCount are equal and the same for
// each set of times/transforms. This does not have to be the case in general
// but that increases the complexity of animation which is already difficult
// enough. Therefore it is assumed that these two values are always equal.

// NOTE(Justin): The animation data in 1-1 correspondence is
//
// JointNames
// Times
// Transforms

// That is,
//
// JointNames[0] Is the name of the first joint
// Times[0] is the array of times of the first joint in the animation
// Transforms[0] is the array of joint transforms for the first joint in the animation
struct animation_info
{
	u32 JointCount;
	string *JointNames;

	f32 CurrentTime;
	u32 KeyFrameIndex;

	u32 TimeCount;
	f32 **Times;

	u32 TransformCount;
	mat4 **Transforms;
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

	f32 *Positions;
	f32 *Normals;
	f32 *UV;
	u32 *Indices;

	u32 JointCount;
	u32 JointInfoCount;

	string *JointNames;
	joint *Joints;
	joint_info *JointsInfo;

	mat4 BindTransform;
	mat4 *InvBindTransforms;
	mat4 *JointTransforms;
	mat4 *ModelSpaceTransforms;

	material_spec MaterialSpec;
};

struct model
{
	basis Basis;

	u32 MeshCount;
	mesh *Meshes;

	// TODO(Justin): Should this be stored at the mesh level?
	u32 VA[2];
	u32 PosVB[2], NormVB[2], JointInfoVB[2];
	u32 IBO[2];

	animation_info AnimationsInfo;
};

#define MESH_H
#endif
