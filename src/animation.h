#if !defined(ANIMATION_H)

struct key_frame
{
	v3 *Positions;
	quaternion *Quaternions;
	v3 *Scales;

	// TODO(Justin): Remove this.
	mat4 *Transforms;
};

// TODO(Justin): string hash for joint names
// struct joint_name_hash
// {
//		string Name;
//		s32 Index;
// };
struct animation_info
{
	u32 JointCount;
	u32 KeyFrameCount;

	//u32 TimeCount;
	u32 TimeCount;

	//u32 KeyFrameIndex;
	u32 KeyFrameIndex;

	f32 CurrentTime;
	f32 Duration;
	f32 FrameRate;

	string *JointNames;

	//f32 *Times;
	f32 *Times;

	key_frame *KeyFrames;
};

struct animation
{
	u32 Count;
	u32 Index;
	animation_info *Info;
};

#define ANIMATION_H
#endif
