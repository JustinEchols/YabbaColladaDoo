#if !defined(MESH_H)

// 
// NOTE(Justin): If writing out these must be aligned.
// 

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

struct material_spec
{
	v4 Ambient;
	v4 Diffuse;
	v4 Specular;

	f32 Shininess;
};

enum material_flags
{
	MaterialFlag_Ambient = (1 << 1),
	MaterialFlag_Diffuse = (1 << 2),
	MaterialFlag_Specular = (1 << 3),
	MaterialFlag_Normal = (1 << 4),
};

struct mesh
{
	string Name;

	u32 IndicesCount;
	u32 VertexCount;
	u32 JointCount;

	u32 *Indices;
	vertex *Vertices;

	v3 *Tangents;
	v3 *BiTangents;

	joint *Joints;

	mat4 BindTransform;
	mat4 *InvBindTransforms;
	mat4 *JointTransforms;
	mat4 *ModelSpaceTransforms;

	material_spec MaterialSpec;
	u32 MaterialFlags;

	u32 AmbientTexture;
	u32 DiffuseTexture;
	u32 SpecularTexture;
	u32 NormalTexture;
};

struct model
{
	b32 HasSkeleton;

	u32 MeshCount;
	mesh *Meshes;

	// TODO(Justin): Should this be stored at the mesh level?
	u32 VA[4];
	u32 VB[4];
	u32 IBO[4];
	u32 TangentVB[4];
	u32 BiTangentVB[4];

	animation Animations;
};


#define MESH_H
#endif
