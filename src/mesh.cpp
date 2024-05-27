internal void
AnimationInfoGet(memory_arena *Arena, xml_node *Root, animation_info *AnimationInfo, u32 *AnimationInfoIndex)
{
	for(s32 ChildIndex = 0; ChildIndex < Root->ChildrenCount; ++ChildIndex)
	{
		xml_node *Node = Root->Children[ChildIndex];
		Assert(Node);
		if(StringsAreSame(Node->Tag, "float_array"))
		{
			if(SubStringExists(Node->Attributes[0].Value, "pose_matrix-input-array"))
			{
				animation_info *Info = AnimationInfo + *AnimationInfoIndex;
				Info->TimeCount = U32FromAttributeValue(Node);
				Info->Times = PushArray(Arena, Info->TimeCount, f32);
				ParseF32Array(Info->Times, Info->TimeCount, Node->InnerText);
			}
			else if(SubStringExists(Node->Attributes[0].Value, "pose_matrix-output-array"))
			{
				animation_info *Info = AnimationInfo + *AnimationInfoIndex;
				Info->JointTransformCount = U32FromAttributeValue(Node);
				Info->JointTransforms = PushArray(Arena, Info->JointTransformCount, f32);
				ParseF32Array(Info->JointTransforms, Info->JointTransformCount, Node->InnerText);
			}
		}
		else if(StringsAreSame(Node->Tag, "channel"))
		{
			xml_attribute Attr = NodeAttributeGet(Node, "target");
			char *AtForwardSlash = strstr((char *)Attr.Value.Data, "/");
			string JointName = StringFromRange(Attr.Value.Data, (u8 *)AtForwardSlash);

			animation_info *Info = AnimationInfo + *AnimationInfoIndex;
			Info->JointName = StringAllocAndCopy(Arena, JointName);

			(*AnimationInfoIndex)++;
		}

		if(*Node->Children)
		{
			AnimationInfoGet(Arena, Node, AnimationInfo, AnimationInfoIndex);
		}
	}
}

// NOTE(Justin): What is a good return value? The indices are stored as a u32
// but if we initialize the result to 0 and return it then the current joint
// will think that the parent is the root joint. We could initialize a return
// value to -1 which is an s32 then cast the s32 to a u32. Or initialize the
// value to a large u32?
internal s32
JointIndexGet(string *JointNames, u32 JointNameCount, string JointName)
{
	s32 Result = -1;

	for(u32 Index = 0; Index < JointNameCount; ++Index)
	{
		string *Name = JointNames + Index;
		if(StringsAreSame(*Name, JointName))
		{
			Result = Index;
			break;
		}
	}

	return(Result);
}

// TODO(Justin): How much guarding/checking should there be?
// TODO(Justin): Only the matrix, joint name, and parent index are retrieved
// now. What about angle, quaternion?
internal void
JointsGet(memory_arena *Arena, xml_node *Root, mesh *Mesh, joint *Joints, u32 *JointIndex)
{
	for(s32 ChildIndex = 0; ChildIndex < Root->ChildrenCount; ++ChildIndex)
	{
		xml_node *Node = Root->Children[ChildIndex];
		Assert(Node);
		if(StringsAreSame(Node->Tag, "node"))
		{
			joint *Joint = Joints + *JointIndex;

			xml_attribute SID = NodeAttributeGet(Node, "sid");
			xml_attribute ParentSID = NodeAttributeGet(Node->Parent, "sid");

			Joint->Name = SID.Value;

			s32 PoseTransformIndex = JointIndexGet(Mesh->JointNames, Mesh->JointNameCount, SID.Value);
			s32 ParentIndex = JointIndexGet(Mesh->JointNames, Mesh->JointNameCount, ParentSID.Value);
			Assert(PoseTransformIndex != - 1);
			Assert(ParentIndex != - 1);

			Joint->PoseTransformIndex = (u32)PoseTransformIndex;
			Joint->ParentIndex = (u32)ParentIndex;

		}
		else if(StringsAreSame(Node->Tag, "matrix"))
		{
			joint *Joint = Joints + *JointIndex;

			// TODO(Justin): Allocate beforehand. We know the size of each
			// transfor 16 f32s and we know how many joints there are as well
			Joint->LocalRestPoseTransform = PushArray(Arena, 16, f32);
			ParseF32Array(Joint->LocalRestPoseTransform, 16, Node->InnerText);

			(*JointIndex)++;
		}

		if(*Node->Children)
		{
			JointsGet(Arena, Node, Mesh, Joints, JointIndex);
		}
	}
}

internal mesh
MeshInit(memory_arena *Arena, loaded_dae DaeFile)
{
	mesh Mesh = {};

	xml_node *Root = DaeFile.Root;

	//
	// NOTE(Justin): Mesh/Skin info
	//

	xml_node Geometry = {};
	xml_node NodePos = {};
	xml_node NodeNormal = {};
	xml_node NodeUV = {};
	xml_node NodeIndex = {};

	NodeGet(Root, &Geometry, "library_geometries");
	NodeGet(&Geometry, &NodePos, "float_array", "mesh-positions-array");
	NodeGet(&Geometry, &NodeNormal, "float_array", "mesh-normals-array");
	NodeGet(&Geometry, &NodeUV, "float_array", "mesh-map-0-array");
	NodeGet(&Geometry, &NodeIndex, "p");

	u32 TriangleCount = U32FromAttributeValue(NodeIndex.Parent);

	Mesh.IndicesCount = 3 * TriangleCount;
	Mesh.PositionsCount = U32FromAttributeValue(&NodePos);
	Mesh.NormalsCount = Mesh.PositionsCount;
	Mesh.UVCount = 2 * (Mesh.PositionsCount / 3);

	Mesh.Positions = PushArray(Arena, Mesh.PositionsCount, f32);
	Mesh.Normals = PushArray(Arena, Mesh.NormalsCount, f32);
	Mesh.UV = PushArray(Arena, Mesh.UVCount, f32);
	Mesh.Indices = PushArray(Arena, Mesh.IndicesCount, u32);

	u32 IndicesCount = 3 * 3 * TriangleCount;
	u32 *Indices = PushArray(Arena, IndicesCount, u32);

	char *ContextI;
	char *TokI = strtok_s((char *)NodeIndex.InnerText.Data, " ", &ContextI);

	u32 Index = 0;
	u32 TriIndex = 0;
	Indices[Index++] = U32FromASCII((u8 *)TokI);
	Mesh.Indices[TriIndex++] = U32FromASCII((u8 *)TokI);
	while(TokI)
	{
		TokI = strtok_s(0, " ", &ContextI);
		Indices[Index++] = U32FromASCII((u8 *)TokI);

		TokI = strtok_s(0, " ", &ContextI);
		Indices[Index++] = U32FromASCII((u8 *)TokI);

		TokI = strtok_s(0, " ", &ContextI);
		if(TokI)
		{
			Indices[Index++] = U32FromASCII((u8 *)TokI);
			Mesh.Indices[TriIndex++] = U32FromASCII((u8 *)TokI);
		}
	}

	u32 PositionCount = Mesh.PositionsCount;
	f32 *Positions = PushArray(Arena, Mesh.PositionsCount, f32);
	ParseF32Array(Positions, PositionCount, NodePos.InnerText);

	u32 NormalCount = U32FromAttributeValue(&NodeNormal);
	f32 *Normals = PushArray(Arena, NormalCount, f32);
	ParseF32Array(Normals, NormalCount, NodeNormal.InnerText);

	u32 UVCount = U32FromAttributeValue(&NodeUV);
	f32 *UV = PushArray(Arena, UVCount, f32);
	ParseF32Array(UV, UVCount, NodeUV.InnerText);

	//
	// NOTE(Justin): Unify indices
	//

	b32 *UniqueIndexTable = PushArray(Arena, Mesh.PositionsCount/3, b32);
	for(u32 i = 0; i < Mesh.PositionsCount/3; ++i)
	{
		UniqueIndexTable[i] = true;
	}

	u32 Stride = 3;
	u32 UVStride = 2;
	for(u32 i = 0; i < IndicesCount; i += 3)
	{
		u32 IndexP = Indices[i];
		u32 IndexN = Indices[i + 1];
		u32 IndexUV = Indices[i + 2];

		b32 IsUniqueIndex = UniqueIndexTable[IndexP];
		if(IsUniqueIndex)
		{
			f32 X = Positions[Stride * IndexP];
			f32 Y = Positions[Stride * IndexP + 1];
			f32 Z = Positions[Stride * IndexP + 2];

			f32 Nx = Normals[Stride * IndexN];
			f32 Ny = Normals[Stride * IndexN + 1];
			f32 Nz = Normals[Stride * IndexN + 2];

			f32 U = UV[UVStride * IndexUV];
			f32 V = UV[UVStride * IndexUV + 1];

			Mesh.Positions[Stride * IndexP] = X;
			Mesh.Positions[Stride * IndexP + 1] = Y;
			Mesh.Positions[Stride * IndexP + 2] = Z;

			Mesh.Normals[Stride * IndexP] = Nx;
			Mesh.Normals[Stride * IndexP + 1] = Ny;
			Mesh.Normals[Stride * IndexP + 2] = Nz;

			Mesh.UV[UVStride * IndexP] = U;
			Mesh.UV[UVStride * IndexP + 1] = V;

			UniqueIndexTable[IndexP] = false;
		}
	}

	//
	// NOTE(Jusitn): Skeletion info
	//
	
	xml_node Controllers = {};
	NodeGet(Root, &Controllers, "library_controllers");
	if(Controllers.ChildrenCount != 0)
	{
		ParseXMLStringArray(Arena, &Controllers, &Mesh.JointNames, &Mesh.JointNameCount, "skin-joints-array");
		ParseXMLFloatArray(Arena, &Controllers, &Mesh.RestPoseTransforms, &Mesh.RestPosTransformCount, "skin-bind_poses-array");
		ParseXMLFloatArray(Arena, &Controllers, &Mesh.Weights, &Mesh.WeightCount, "skin-weights-array");

		xml_node NodeJointCount = {};
		NodeGet(&Controllers, &NodeJointCount, "vertex_weights");
		u32 JointCount = U32FromAttributeValue(&NodeJointCount);
		u32 *JointCountArray = PushArray(Arena, JointCount, u32);
		ParseXMLU32Array(Arena, &NodeJointCount, &JointCountArray, JointCount, "vcount");

		xml_node NodeJointsAndWeights = {};
		NodeGet(&Controllers, &NodeJointsAndWeights, "v");

		u32 JointsAndWeightsCount = 2 * U32ArraySum(JointCountArray, JointCount);
		u32 *JointsAndWeights = PushArray(Arena, JointsAndWeightsCount, u32);
		ParseU32Array(JointsAndWeights, JointsAndWeightsCount, NodeJointsAndWeights.InnerText);

		Mesh.JointInfoCount = JointCount;
		Mesh.JointsInfo = PushArray(Arena, Mesh.JointInfoCount, joint_info);

		// TODO(Justin): Models could have many joints that affect a single vertex and this must be handled.
		// TODO(Justin): Make sure weights are a convex combination. 
		u32 JointsAndWeightsIndex = 0;
		for(u32 JointIndex = 0; JointIndex < Mesh.JointInfoCount; ++JointIndex)
		{
			u32 JointCountForVertex = JointCountArray[JointIndex];

			joint_info *JointInfo = Mesh.JointsInfo + JointIndex;
			JointInfo->Count = JointCountForVertex;
			for(u32 k = 0; k < JointInfo->Count; ++k)
			{
				JointInfo->JointIndex[k] = JointsAndWeights[JointsAndWeightsIndex++];
				u32 WeightIndex = JointsAndWeights[JointsAndWeightsIndex++];
				JointInfo->Weights[k] = Mesh.Weights[WeightIndex];
			}
		}
	}

	//
	// NOTE(Justin): Animations
	//

	xml_node LibAnimations = {};
	NodeGet(Root, &LibAnimations, "library_animations");
	if(LibAnimations.ChildrenCount != 0)
	{
		xml_node *AnimRoot = LibAnimations.Children[0];

		Mesh.AnimationInfoCount = AnimRoot->ChildrenCount;
		Mesh.AnimationsInfo = PushArray(Arena, Mesh.AnimationInfoCount, animation_info);

		animation_info *Info = Mesh.AnimationsInfo;
		u32 AnimationInfoIndex = 0;

		AnimationInfoGet(Arena, AnimRoot, Info, &AnimationInfoIndex);
		Assert(AnimationInfoIndex == Mesh.AnimationInfoCount);
	}

	//
	// NOTE(Justin): Visual Scenes (but only joint tree)
	//

	xml_node LibVisScenes = {};
	NodeGet(Root, &LibVisScenes, "library_visual_scenes");
	if(LibVisScenes.ChildrenCount != 0)
	{
		Mesh.JointCount = Mesh.JointNameCount;
		Mesh.Joints = PushArray(Arena, Mesh.JointCount, joint);

		xml_node JointRoot = {};
		// TODO(Justin): Better way to get the root joint node in tree.
		FirstNodeWithAttrValue(&LibVisScenes, &JointRoot, "JOINT");
		if(JointRoot.ChildrenCount != 0)
		{
			u32 JointIndex = 0;
			joint *Joints = Mesh.Joints;

			Joints->Name = Mesh.JointNames[0];
			Joints->ParentIndex = -1;

			JointsGet(Arena, &JointRoot, &Mesh, Joints, &JointIndex);
		}
	}

	return(Mesh);
}
