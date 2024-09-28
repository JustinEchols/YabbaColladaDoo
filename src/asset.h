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

struct asset_joint_name
{
	u8 Name[64];
};

struct asset_key_frame
{
	v3 *Positions;
	quaternion *Quaternions;
	v3 *Scales;
};

struct asset_animation_info
{
	u64 OffsetToPositions;
	u64 OffsetToQuaternions;
	u64 OffsetToScales;
};

#define ANIMATION_FILE_MAGIC_NUMBER ((0x1 << 24) | (0x2 << 16) | (0x3 << 4) | (0x4 << 0))
#define ANIMATION_FILE_VERSION 0
struct asset_animation_header
{
	u32 MagicNumber;
	u32 Version;
	u32 JointCount;
	u32 KeyFrameCount;
	u32 TimeCount;
	u32 KeyFrameIndex;
	f32 CurrentTime;
	f32 Duration;
	f32 FrameRate;

	u64 OffsetToTimes;
	u64 OffsetToNames;
	u64 OffsetToKeyFrames;
};

struct asset
{
	//u64 OffsetToData;
	union
	{
		asset_mesh_info MeshInfo;
		asset_animation_info AnimationInfo;
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
