#if !defined(MESH_H)

#pragma pack(push, 1)
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
	mat4 Transform;
};

struct vertex
{
	v3 P;
	v3 N;
	v2 UV;

	joint_info JointInfo;
};
#pragma pack(pop)

struct key_frame
{
	v3 *Positions;
	quaternion *Quaternions;
	v3 *Scales;

	mat4 *Transforms;
};

struct animation_info
{
	u32 JointCount;
	u32 TimeCount;
	u32 KeyFrameCount;
	u32 KeyFrameIndex;
	f32 CurrentTime;

	string *JointNames;
	f32 *Times;
	key_frame *KeyFrames;
};

struct animation
{
	u32 Count;
	u32 Index;
	animation_info *Info;
};

struct material_spec
{
	v4 Ambient;
	v4 Diffuse;
	v4 Specular;

	f32 Shininess;
};



struct mesh
{
	string Name;

	u32 IndicesCount; //load
	u32 VertexCount; //load
	u32 JointCount; //load

	u32 *Indices; //do not have ot load
	vertex *Vertices; //load

	string *JointNames; //load
	joint *Joints; //load

	mat4 BindTransform; //load
	mat4 *InvBindTransforms;//load
	mat4 *JointTransforms; //runtime
	mat4 *ModelSpaceTransforms; //runtime

	material_spec MaterialSpec;

	u32 TextureHandle;//runtime
};

struct model
{
	basis Basis;

	b32 HasSkeleton;

	u32 MeshCount;
	mesh *Meshes;

	// TODO(Justin): Should this be stored at the mesh level?
	u32 VA[2];
	u32 VB[2];
	u32 IBO[2];

	animation Animations;
};


#define MESH_H
#endif
