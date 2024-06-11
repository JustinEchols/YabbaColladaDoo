
// NOTE(Justin): What is a good return value? The indices are stored as a u32
// but if we initialize the result to 0 and return it then the current joint
// will think that the parent is the root joint. We could initialize a return
// value to -1 which is an s32 then cast the s32 to a u32. Or initialize the
// value to a large u32?
internal s32
JointIndexGet(string *JointNames, u32 JointCount, string JointName)
{
	s32 Result = -1;
	for(u32 Index = 0; Index < JointCount; ++Index)
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

// TODO(Justin): I dont like the idea of passing in the mesh here. We could do
// that for all the data that needs joint indices at the very end?
internal void
AnimationInfoGet(memory_arena *Arena, xml_node *Root, mesh *Mesh, animation_info *AnimationInfo, u32 *AnimationInfoIndex)
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
				xml_node AccessorNode = {};
				NodeGet(Node->Parent, &AccessorNode, "accessor");

				xml_attribute AttrCount = NodeAttributeGet(Node, "count");
				xml_attribute AttrStride = NodeAttributeGet(&AccessorNode, "stride");

				u32 Count = U32FromASCII(AttrCount.Value.Data);
				u32 Stride = U32FromASCII(AttrStride.Value.Data);

				animation_info *Info = AnimationInfo + *AnimationInfoIndex;

				Info->TransformCount = Count / Stride;
				Info->AnimationTransforms = PushArray(Arena, Info->TransformCount, mat4);

				ParseF32Array((f32 *)Info->AnimationTransforms, Count, Node->InnerText);
			}
		}
		else if(StringsAreSame(Node->Tag, "channel"))
		{
			// TODO(Justin): Figure out a way to get the joint name
			xml_attribute Attr = NodeAttributeGet(Node, "target");
			char *AtForwardSlash = strstr((char *)Attr.Value.Data, "/");
			string ArmatureName = StringFromRange(Attr.Value.Data, (u8 *)AtForwardSlash);

			animation_info *Info = AnimationInfo + *AnimationInfoIndex;

			
			Info->JointName = StringAllocAndCopy(Arena, ArmatureName);
			//Info->JointIndex = JointIndexGet(Mesh->JointNames, Mesh->JointCount, ArmatureName);

			(*AnimationInfoIndex)++;
		}

		if(*Node->Children)
		{
			AnimationInfoGet(Arena, Node, Mesh, AnimationInfo, AnimationInfoIndex);
		}
	}
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

			s32 Index = JointIndexGet(Mesh->JointNames, Mesh->JointCount, SID.Value);
			s32 ParentIndex = JointIndexGet(Mesh->JointNames, Mesh->JointCount, ParentSID.Value);
			Assert(Index != - 1);
			Assert(ParentIndex != - 1);

			Joint->Index = (u32)Index;
			Joint->ParentIndex = (u32)ParentIndex;

		}
		else if(StringsAreSame(Node->Tag, "matrix"))
		{
			joint *Joint = Joints + *JointIndex;

			// TODO(Justin): Allocate beforehand. We know the size of each
			// transfor 16 f32s and we know how many joints there are as well
			//Joint->PoseTransform = PushArray(Arena, 16, f32);
			Joint->Transform = PushArray(Arena, 1, mat4);
			ParseF32Array((f32 *)Joint->Transform, 16, Node->InnerText);

			(*JointIndex)++;
		}

		if(*Node->Children)
		{
			JointsGet(Arena, Node, Mesh, Joints, JointIndex);
		}
	}
}


internal mesh
MeshInitFromCollada(memory_arena *Arena, loaded_dae DaeFile)
{
	mesh Mesh = {};

	xml_node *Root = DaeFile.Root;

	//
	// NOTE(Justin): Mesh/Skin info
	//

	xml_node Geometry = {};
	xml_node Triangles = {};

	NodeGet(Root, &Geometry, "library_geometries");

	NodeGet(&Geometry, &Triangles, "triangles");
	u32 AttributeCount = Triangles.ChildrenCount - 1;

	xml_node NodeIndex = {};
	xml_node NodePos = {};
	xml_node NodeNormal = {};
	xml_node NodeUV = {};
	xml_node NodeColor = {};

	for(u32 ChildIndex = 0; ChildIndex < AttributeCount; ++ChildIndex)
	{
		xml_node N = {};
		xml_node *Node = Triangles.Children[ChildIndex];

		string Value = NodeAttributeValueGet(Node, "source");
		Value.Data++;

		string Semantic = Node->Attributes[0].Value;
		if(StringsAreSame(Semantic, "VERTEX"))
		{

			NodeGet(&Geometry, &N, "vertices", (char *)Value.Data);
			N = *(N.Children[0]);

			Value = NodeAttributeValueGet(&N, "source");
			Value.Data++;

			N = {};
			NodeGet(&Geometry, &N, "source", (char *)Value.Data);

			NodePos = *(N.Children[0]);
		}
		else if(StringsAreSame(Semantic, "NORMAL"))
		{
			NodeGet(&Geometry, &N, "source", (char *)Value.Data);
			NodeNormal = *(N.Children[0]);
		}
		else if(StringsAreSame(Semantic, "TEXCOORD"))
		{
			NodeGet(&Geometry, &N, "source", (char *)Value.Data);
			NodeUV= *(N.Children[0]);
		}
		else
		{
			NodeGet(&Geometry, &N, "float_array", (char *)Value.Data);
			NodeColor = *(N.Children[0]);
		}
	}

	NodeGet(&Geometry, &NodeIndex, "p");

	u32 TriangleCount = U32FromAttributeValue(NodeIndex.Parent);
	u32 IndicesCount = 3 * 3 * TriangleCount;
	u32 PositionCount = U32FromAttributeValue(&NodePos);
	u32 NormalCount = U32FromAttributeValue(&NodeNormal);
	u32 UVCount = U32FromAttributeValue(&NodeUV);

	u32 *Indices = PushArray(Arena, IndicesCount, u32);
	f32 *Positions = PushArray(Arena, PositionCount, f32);
	f32 *Normals = PushArray(Arena, NormalCount, f32);
	f32 *UV = PushArray(Arena, UVCount, f32);

	ParseU32Array(Indices, IndicesCount, NodeIndex.InnerText);
	ParseF32Array(Positions, PositionCount, NodePos.InnerText);
	ParseF32Array(Normals, NormalCount, NodeNormal.InnerText);
	ParseF32Array(UV, UVCount, NodeUV.InnerText);

	Mesh.IndicesCount = 3 * TriangleCount;
	Mesh.PositionsCount = PositionCount;
	if(NormalCount != 0)
	{
		Mesh.NormalsCount = Mesh.PositionsCount;
	}

	if(UVCount != 0)
	{
		Mesh.UVCount = 2 * (Mesh.PositionsCount / 3);
	}

	Mesh.Positions = PushArray(Arena, Mesh.PositionsCount, f32);
	Mesh.Normals = PushArray(Arena, Mesh.NormalsCount, f32);
	Mesh.UV = PushArray(Arena, Mesh.UVCount, f32);
	Mesh.Indices = PushArray(Arena, Mesh.IndicesCount, u32);

	u32 TriIndex = 0;
	for(u32 Index = 0; Index < IndicesCount; Index += AttributeCount)
	{
		Mesh.Indices[TriIndex++] = Indices[Index];
	}

	//
	// NOTE(Justin): Unify indices
	//

	b32 *UniqueIndexTable = PushArray(Arena, Mesh.PositionsCount/3, b32);
	for(u32 i = 0; i < Mesh.PositionsCount/3; ++i)
	{
		UniqueIndexTable[i] = true;
	}

	// TODO(Justin): Is there a better way to handle different attribute counts?
	u32 StrideP = 3;
	u32 StrideUV = 2;
	for(u32 i = 0; i < IndicesCount; i += AttributeCount)
	{
		u32 IndexP = Indices[i];
		b32 IndexIsUnique = UniqueIndexTable[IndexP];
		if(IndexIsUnique)
		{
			if(AttributeCount == 2)
			{
				u32 IndexN = Indices[i + 1];

				f32 X = Positions[StrideP * IndexP];
				f32 Y = Positions[StrideP * IndexP + 1];
				f32 Z = Positions[StrideP * IndexP + 2];

				f32 Nx = Normals[StrideP * IndexN];
				f32 Ny = Normals[StrideP * IndexN + 1];
				f32 Nz = Normals[StrideP * IndexN + 2];

				Mesh.Positions[StrideP * IndexP] = X;
				Mesh.Positions[StrideP * IndexP + 1] = Y;
				Mesh.Positions[StrideP * IndexP + 2] = Z;

				Mesh.Normals[StrideP * IndexP] = Nx;
				Mesh.Normals[StrideP * IndexP + 1] = Ny;
				Mesh.Normals[StrideP * IndexP + 2] = Nz;

				UniqueIndexTable[IndexP] = false;
			}
			else if(AttributeCount == 3)
			{
				u32 IndexN = Indices[i + 1];
				u32 IndexUV = Indices[i + 2];

				f32 X = Positions[StrideP * IndexP];
				f32 Y = Positions[StrideP * IndexP + 1];
				f32 Z = Positions[StrideP * IndexP + 2];

				f32 Nx = Normals[StrideP * IndexN];
				f32 Ny = Normals[StrideP * IndexN + 1];
				f32 Nz = Normals[StrideP * IndexN + 2];

				f32 U = UV[StrideUV * IndexUV];
				f32 V = UV[StrideUV * IndexUV + 1];

				Mesh.Positions[StrideP * IndexP] = X;
				Mesh.Positions[StrideP * IndexP + 1] = Y;
				Mesh.Positions[StrideP * IndexP + 2] = Z;

				Mesh.Normals[StrideP * IndexP] = Nx;
				Mesh.Normals[StrideP * IndexP + 1] = Ny;
				Mesh.Normals[StrideP * IndexP + 2] = Nz;

				Mesh.UV[StrideUV * IndexP] = U;
				Mesh.UV[StrideUV * IndexP + 1] = V;

				UniqueIndexTable[IndexP] = false;
			}
		}
	}

	//
	// NOTE(Jusitn): Skeletion info
	//
	
	xml_node Controllers = {};
	NodeGet(Root, &Controllers, "library_controllers");
	if(Controllers.ChildrenCount != 0)
	{
		ParseColladaStringArray(Arena, &Controllers, &Mesh.JointNames, &Mesh.JointCount);

		Assert(Mesh.JointNames);
		Assert(Mesh.JointCount != 0);

		Mesh.JointTransforms = PushArray(Arena, Mesh.JointCount, mat4);
		Mesh.ModelSpaceTransforms = PushArray(Arena, Mesh.JointCount, mat4);

		mat4 I = Mat4Identity();
		for(u32 MatIndex = 0; MatIndex < Mesh.JointCount; ++MatIndex)
		{
			Mesh.JointTransforms[MatIndex] = I;
			Mesh.ModelSpaceTransforms[MatIndex] = I;
		}

		ParseColladaFloatArray(Arena, &Controllers,
				&((f32 *)Mesh.InvBindTransforms), &Mesh.JointCount,
				"input", "INV_BIND_MATRIX");

		Assert(Mesh.InvBindTransforms);

		ParseColladaFloatArray(Arena, &Controllers,
				&Mesh.Weights, &Mesh.WeightCount,
				"input", "WEIGHT");

		Assert(Mesh.Weights);
		Assert(Mesh.WeightCount != 0);

		xml_node VertexWeights = {};
		NodeGet(&Controllers, &VertexWeights, "vertex_weights");

		u32 JointInfoCount = U32FromAttributeValue(&VertexWeights);
		u32 *JointCountArray = PushArray(Arena, JointInfoCount, u32);

		ParseColladaU32Array(Arena, &Controllers, &JointCountArray, JointInfoCount, "vcount");

		xml_node NodeJointsAndWeights = {};
		NodeGet(&Controllers, &NodeJointsAndWeights, "v");

		u32 JointsAndWeightsCount = 2 * U32ArraySum(JointCountArray, JointInfoCount);
		u32 *JointsAndWeights = PushArray(Arena, JointsAndWeightsCount, u32);
		ParseU32Array(JointsAndWeights, JointsAndWeightsCount, NodeJointsAndWeights.InnerText);

		Mesh.JointInfoCount = JointInfoCount;
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

		AnimationInfoGet(Arena, AnimRoot, &Mesh, Info, &AnimationInfoIndex);
		Assert(AnimationInfoIndex == Mesh.AnimationInfoCount);
	}

	//
	// NOTE(Justin): Visual Scenes (but only joint tree)
	//

	xml_node LibVisScenes = {};
	NodeGet(Root, &LibVisScenes, "library_visual_scenes");
	if(LibVisScenes.ChildrenCount != 0)
	{
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

		// NOTE(Justin): When processin lib animations the channel stores the
		// armature name not the joint name. We get the joint name of the animation here.

#if 1
		xml_node N = {};
		for(u32 k = 0; k < Mesh.AnimationInfoCount; ++k)
		{
			animation_info *Info = Mesh.AnimationsInfo + k;

			NodeGet(&LibVisScenes, &N, "node", (char *)Info->JointName.Data);
			string SID = NodeAttributeValueGet(&N, "sid");
			Info->JointName = SID;
			Info->JointIndex = JointIndexGet(Mesh.JointNames, Mesh.JointCount, SID);
		}
#endif
	}

	return(Mesh);
}

#if 0
internal void
MeshBindPose(mesh *Mesh)
{
	mat4 RootJointT = *Mesh->Joints[0].Transform;
	mat4 RootInvBind = Mesh->InvBindTransforms[0];
	Mesh->ModelSpaceTransforms[0] = RootJointT * RootInvBind * Bind;
	for(u32 Index = 1; Index < Mesh.JointCount; ++Index)
	{
		joint *Joint = Mesh.Joints + Index;

		mat4 ParentTransform = *Mesh.Joints[Joint->ParentIndex].Transform;
		mat4 JointTransform = *Joint->Transform;

		JointTransform = ParentTransform * JointTransform;
		mat4 InvBind = Mesh.InvBindTransforms[Index];

		// NOTE(Justin): The line below puts the mesh into the articulated pose
		//Mesh.ModelSpaceTransforms[Joint->Index] = *Joint->Transform * InvBind * Bind;


		// NOTE(Justin): The line below puts the mesh into bind/t/rest pose
		Mesh.ModelSpaceTransforms[Index] = JointTransform * InvBind * Bind;

	}
}
#endif
