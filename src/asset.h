#if !defined(ASSET_H)

#pragma pack(push, 1)
struct asset_mesh_info
{
	// NOTE(Justin): Since this struct is serialize/de-serialzed we use a fixed sized buffer for the name.
	u8 Name[32];
	u32 IndicesCount;
	u32 VertexCount;
	u32 JointCount;
	material_spec MaterialSpec;
	mat4 XFormBind;

	u64 OffsetToIndices;
	u64 OffsetToVertices;
	u64 OffsetToJoints;
	u64 OffsetToInvBindXForms;
};

struct asset_joint_info
{
	u8 Name[32];
	s32 ParentIndex;
	mat4 Transform;
};

#define MODEL_FILE_MAGIC_NUMBER ((1 << 24) | (2 << 16) | (3 << 4) | (4 << 0))
#define MODEL_FILE_VERSION 0
struct asset_model_header
{
	u32 MagicNumber;
	u32 Version;
	b32 HasSkeleton;
	u32 MeshCount;
};

struct asset
{
	//u64 OffsetToData;
	union
	{
		asset_mesh_info MeshInfo;
	};
};

#pragma pack(pop)

#define ASSET_COUNT 256
struct asset_manager
{
	u32 AssetCount;
	asset Assets[ASSET_COUNT];
};

#define ASSET_H
#endif
