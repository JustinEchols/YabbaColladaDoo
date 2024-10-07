#if !defined(ASSET_H)

#pragma pack(push, 1)
struct asset_mesh_info
{
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

#define MODEL_FILE_MAGIC_NUMBER ((1 << 24) | (2 << 16) | (3 << 8) | (4 << 0))
#define MODEL_FILE_VERSION 0
struct asset_model_header
{
	u32 MagicNumber;
	u32 Version;
	b32 HasSkeleton;
	u32 MeshCount;
	u64 OffsetToMeshInfo;
};

struct asset_joint_name
{
	u8 Name[64];
};

struct asset_animation_info
{
	u64 OffsetToPositions;
	u64 OffsetToQuaternions;
	u64 OffsetToScales;
};

#define ANIMATION_FILE_MAGIC_NUMBER ((0x1 << 24) | (0x2 << 16) | (0x3 << 8) | (0x4 << 0))
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
	u64 OffsetToAnimationInfo;
};

// NOTE(Justin): Below is information used to do the asset conversion.
struct asset
{
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

#if 0
char *ModelSourceFiles[] = 
{
	"models\\XBot.dae",
	"models\\Sphere.dae",
	"models\\VampireALusth.dae",
};
char *XBotAnimSourceFiles[] =
{
	"animations\\XBot_ActionIdle.dae",
	"animations\\XBot_ActionIdleToStandingIdle.dae",
	"animations\\XBot_FemaleWalk.dae",
	"animations\\XBot_IdleLookAround.dae",
	//"animations\\XBot_IdleShiftWeight.dae",
	"animations\\XBot_IdleToSprint.dae",
	"animations\\XBot_LeftTurn.dae",
	"animations\\XBot_Pushing.dae",
	"animations\\XBot_PushingStart.dae",
	"animations\\XBot_PushingStop.dae",
	"animations\\XBot_RightTurn.dae",
	"animations\\XBot_RunToStop.dae",
	"animations\\XBot_Running.dae",
	"animations\\XBot_Running180.dae",
	"animations\\XBot_RunningChangeDirection.dae",
	"animations\\XBot_RunningToTurn.dae",
	"animations\\XBot_Walking.dae",
	"animations\\XBot_WalkingTurn180.dae"
};
char *VampireAnimSourceFiles [] =
{
	"animations\\Vampire_Bite.dae",
	"animations\\Vampire_Walking.dae",
};
#endif
#define ASSET_H
#endif
