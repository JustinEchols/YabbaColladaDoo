#if !defined(MESH_H)

// 
// NOTE(Justin): If writing out these must be aligned.
// 

#pragma pack(push, 1)
struct joint_info
{
	u32 Count;
	u32 JointIndex[4];
	f32 Weights[4];
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

// TODO(Justin): Mesh material info!
struct mesh
{
	string Name;

	u32 IndicesCount;
	u32 *Indices;

	u32 VertexCount;
	vertex *Vertices;

	v3 *Tangents;
	v3 *BiTangents;

	u32 JointCount;
	string *JointNames;
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

	// TODO(Justin): Use textures as part of model itself

	// Old file format uses joints per mesh
	joint *Joints;
};

struct skeleton
{
	u32 JointCount;
	joint *Joints;
	mat4 *JointTransforms;
	mat4 *ModelSpaceTransforms;
};

struct model
{
	string FullPath;

	b32 HasSkeleton;

	u32 MeshCount;
	mesh *Meshes;

	// TODO(Justin): Should this be stored at the mesh level?
	u32 VA[16];
	u32 VB[16];
	u32 IBO[16];
	u32 TangentVB[16];
	u32 BiTangentVB[16];

	skeleton Skeleton;

	// For testing only!
	animation Animations;

	u32 TextureCount;
	texture Textures[32];
	//texture *Textures;
};


#define MESH_H
#endif
