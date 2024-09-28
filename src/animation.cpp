
inline f32
KeyFrameDeltaTime(f32 *Times, u32 KeyFrameIndex)
{
	f32 Result = (Times[KeyFrameIndex + 1] - Times[KeyFrameIndex]);
	return(Result);
}

inline mat4
KeyFrameInterpolatedTransform(key_frame *Current, f32 t, key_frame *Next, u32 JointIndex)
{
	mat4 Result = Mat4Identity();

	v3 P1 = Current->Positions[JointIndex];
	quaternion Q1 = Current->Quaternions[JointIndex];
	v3 Scale1 = Current->Scales[JointIndex];

	v3 P2 = Next->Positions[JointIndex];
	quaternion Q2 = Next->Quaternions[JointIndex];
	v3 Scale2 = Next->Scales[JointIndex];

	v3 P = Lerp(P1, t, P2);
	quaternion Q = LerpShortest(Q1, t, Q2);
	v3 Scale = Lerp(Scale1, t, Scale2);

	mat4 R = QuaternionToMat4(Q);
	v3 Rx = Scale.x * Mat4ColumnGet(R, 0);
	v3 Ry = Scale.y * Mat4ColumnGet(R, 1);
	v3 Rz = Scale.z * Mat4ColumnGet(R, 2);

	Result = Mat4(Rx, Ry, Rz, P);
	return(Result);
}

#if 1
internal void
AnimationUpdate(model *Model, f32 dT)
{
	animation_info *AnimInfo = Model->Animations.Info + Model->Animations.Index;

	AnimInfo->CurrentTime += dT;
	u32 KeyFrameIndex = AnimInfo->KeyFrameIndex;
	f32 KeyFrameDt = KeyFrameDeltaTime(AnimInfo->Times, KeyFrameIndex);
	f32 t = (AnimInfo->CurrentTime - AnimInfo->Times[KeyFrameIndex]) / KeyFrameDt;
	if(t >= 1.0f)
	{
		AnimInfo->KeyFrameIndex += 1;
		if(AnimInfo->KeyFrameIndex >= (AnimInfo->KeyFrameCount - 1))
		{
			AnimInfo->KeyFrameIndex = 0;
			AnimInfo->CurrentTime = dT;

			// NOTE(Justin): Test code
			Model->Animations.Index += 1;
			if(Model->Animations.Index >= Model->Animations.Count)
			{
				Model->Animations.Index = 0;
			}
		}

		t = (AnimInfo->CurrentTime - AnimInfo->Times[KeyFrameIndex]) / KeyFrameDeltaTime(AnimInfo->Times, AnimInfo->KeyFrameIndex);
	}

	key_frame *KeyFrame = AnimInfo->KeyFrames + AnimInfo->KeyFrameIndex;
	key_frame *NextKeyFrame = AnimInfo->KeyFrames + (AnimInfo->KeyFrameIndex + 1);
	for(u32 MeshIndex = 0; MeshIndex < Model->MeshCount; ++MeshIndex)
	{
		mesh *Mesh = Model->Meshes + MeshIndex;
		joint RootJoint = Mesh->Joints[0];
		mat4 RootJointT = RootJoint.Transform;
		mat4 RootInvBind = Mesh->InvBindTransforms[0];
		mat4 Bind = Mesh->BindTransform;

		u32 JointIndex = JointIndexGet(AnimInfo->JointNames, AnimInfo->JointCount, RootJoint.Name);
		if(JointIndex != -1)
		{
			RootJointT = KeyFrameInterpolatedTransform(KeyFrame, t, NextKeyFrame, JointIndex);
		}

		Mesh->JointTransforms[0] = RootJointT;
		Mesh->ModelSpaceTransforms[0] = RootJointT * RootInvBind;
		for(u32 Index = 1; Index < Mesh->JointCount; ++Index)
		{
			joint *Joint = Mesh->Joints + Index;
			mat4 JointTransform = Joint->Transform;

			JointIndex = JointIndexGet(AnimInfo->JointNames, AnimInfo->JointCount, Joint->Name);
			if(JointIndex != -1)
			{
				JointTransform = KeyFrameInterpolatedTransform(KeyFrame, t, NextKeyFrame, JointIndex);
			}

			mat4 ParentTransform = Mesh->JointTransforms[Joint->ParentIndex];
			JointTransform = ParentTransform * JointTransform;
			mat4 InvBind = Mesh->InvBindTransforms[Index];

			Mesh->JointTransforms[Index] = JointTransform;
			Mesh->ModelSpaceTransforms[Index] = JointTransform * InvBind;
		}
	}
}
#endif
