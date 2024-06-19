
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

internal void
AnimationInfoGet(memory_arena *Arena, xml_node *Root, string *JointNames, u32 JointCount, animation_info *AnimationInfo, u32 *AnimationInfoIndex)
{
	for(s32 ChildIndex = 0; ChildIndex < Root->ChildrenCount; ++ChildIndex)
	{
		xml_node *Node = Root->Children[ChildIndex];
		Assert(Node);
		if(StringsAreSame(Node->Tag, "float_array"))
		{
			if(SubStringExists(Node->Attributes[0].Value, "input"))
			{
				animation_info *Info = AnimationInfo + *AnimationInfoIndex;
				Info->TimeCount = U32FromAttributeValue(Node);
				Info->Times = PushArray(Arena, Info->TimeCount, f32);
				ParseF32Array(Info->Times, Info->TimeCount, Node->InnerText);
			}
			else if(SubStringExists(Node->Attributes[0].Value, "output"))
			{
				xml_node AccessorNode = {};
				NodeGet(Node->Parent, &AccessorNode, "accessor");

				xml_attribute AttrCount = NodeAttributeGet(Node, "count");
				xml_attribute AttrStride = NodeAttributeGet(&AccessorNode, "stride");

				u32 Count = U32FromASCII(AttrCount.Value.Data);
				u32 Stride = U32FromASCII(AttrStride.Value.Data);

				animation_info *Info = AnimationInfo + *AnimationInfoIndex;

				Info->TransformCount = Count / Stride;
				Info->Transforms = PushArray(Arena, Info->TransformCount, mat4);

				ParseF32Array((f32 *)Info->Transforms, Count, Node->InnerText);
			}
		}
		else if(StringsAreSame(Node->Tag, "animation"))
		{
			// TODO(Justin): Figure out a way to get the joint name
			string JointName = NodeAttributeValueGet(Node, "name");

			animation_info *Info = AnimationInfo + *AnimationInfoIndex;

			Info->JointName = StringAllocAndCopy(Arena, JointName);
			Info->JointIndex = JointIndexGet(JointNames, JointCount, Info->JointName);
		}
		else if(StringsAreSame(Node->Tag, "channel"))
		{
			(*AnimationInfoIndex)++;
		}

		if(*Node->Children)
		{
			AnimationInfoGet(Arena, Node, JointNames, JointCount, AnimationInfo, AnimationInfoIndex);
		}
	}
}


// TODO(Justin): How much guarding/checking should there be?
// TODO(Justin): Only the matrix, joint name, and parent index are retrieved
// now. What about angle, quaternion?
internal void
JointsGet(memory_arena *Arena, xml_node *Root, string *JointNames, u32 JointCount, joint *Joints, u32 *JointIndex)
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

			s32 Index = JointIndexGet(JointNames, JointCount, SID.Value);
			s32 ParentIndex = JointIndexGet(JointNames, JointCount, ParentSID.Value);
			Assert(Index != - 1);
			Assert(ParentIndex != - 1);

			Joint->Index = (u32)Index;
			Joint->ParentIndex = (u32)ParentIndex;

		}
		else if(StringsAreSame(Node->Tag, "matrix"))
		{
			joint *Joint = Joints + *JointIndex;
			Joint->Transform = PushArray(Arena, 1, mat4);
			ParseF32Array((f32 *)Joint->Transform, 16, Node->InnerText);

			(*JointIndex)++;
		}

		if(*Node->Children)
		{
			JointsGet(Arena, Node, JointNames, JointCount, Joints, JointIndex);
		}
	}
}

internal model 
ModelInitFromCollada(memory_arena *Arena, loaded_dae DaeFile)
{
	model Model = {};

	xml_node *Root = DaeFile.Root;

	xml_node LibGeometry = {};
	xml_node Triangles = {};

	NodeGet(Root, &LibGeometry, "library_geometries");

	//
	// NOTE(Justin): The number of meshes is the number of children that
	// library_geometries has. Each geometry node has AT MOST one mesh child
	// node. The LAST CHILD NODE of a mesh node contains two things. A)
	// Where to find positions, normals, UVs, colors and B) The indices of the
	// mesh.
	//

	u32 MeshCount = LibGeometry.ChildrenCount;
	Model.MeshCount = MeshCount;
	Model.Meshes = PushArray(Arena, Model.MeshCount, mesh);
	for(u32 MeshIndex = 0; MeshIndex < MeshCount; ++MeshIndex)
	{
		mesh Mesh = {};

		xml_node *Geometry = LibGeometry.Children[MeshIndex];

		string MeshName = NodeAttributeValueGet(Geometry, "name");
		Mesh.Name = StringAllocAndCopy(Arena, MeshName);

		xml_node *MeshNode = Geometry->Children[0];
		Triangles = *MeshNode->Children[MeshNode->ChildrenCount - 1];

		xml_node NodeIndex = {};
		xml_node NodePos = {};
		xml_node NodeNormal = {};
		xml_node NodeUV = {};
		xml_node NodeColor = {};

		s32 AttributeCount = 0;
		for(s32 k = 0; k < Triangles.ChildrenCount; ++k)
		{
			xml_node *Node = Triangles.Children[k];
			if(StringsAreSame(Node->Tag, "input"))
			{
				AttributeCount++;
			}
		}

		for(s32 k = 0; k < AttributeCount; ++k)
		{
			xml_node N = {};
			xml_node *Node = Triangles.Children[k];

			string Semantic = Node->Attributes[0].Value;
			string Value = NodeAttributeValueGet(Node, "source");

			Value.Data++;
			Value.Size--;

			if(StringsAreSame(Semantic, "VERTEX"))
			{
				NodeGet(Geometry, &N, "vertices", (char *)Value.Data);
				N = *(N.Children[0]);

				Value = NodeAttributeValueGet(&N, "source");
				Value.Data++;
				Value.Size--;

				N = {};
				NodeGet(Geometry, &N, "source", (char *)Value.Data);

				NodePos = *(N.Children[0]);
			}
			else if(StringsAreSame(Semantic, "NORMAL"))
			{
				NodeGet(Geometry, &N, "source", (char *)Value.Data);
				NodeNormal = *(N.Children[0]);
			}
			else if(StringsAreSame(Semantic, "TEXCOORD"))
			{
				NodeGet(Geometry, &N, "source", (char *)Value.Data);
				NodeUV = *(N.Children[0]);
			}
			else if(StringsAreSame(Semantic, "COLOR"))
			{
				NodeGet(Geometry, &N, "float_array", (char *)Value.Data);
				NodeColor = *(N.Children[0]);
			}
		}

		NodeGet(Geometry, &NodeIndex, "p");

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

		b32 *UniqueIndexTable = PushArray(Arena, Mesh.PositionsCount / 3, b32);
		for(u32 i = 0; i < Mesh.PositionsCount / 3; ++i)
		{
			UniqueIndexTable[i] = true;
		}

		u32 StrideP = 3;
		u32 StrideUV = 2;
		if(AttributeCount == 2)
		{
			for(u32 i = 0; i < IndicesCount; i += AttributeCount)
			{
				u32 IndexP = Indices[i];
				b32 IndexIsUnique = UniqueIndexTable[IndexP];
				if(IndexIsUnique)
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
			}
		}

		if(AttributeCount == 3)
		{
			for(u32 i = 0; i < IndicesCount; i += AttributeCount)
			{
				u32 IndexP = Indices[i];
				b32 IndexIsUnique = UniqueIndexTable[IndexP];
				if(IndexIsUnique)
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

		//
		// NOTE(Justin): The children of library controllers are controller nodes.
		// IF WE ASSUME THAT EACH MESH HAS A CONTROLLER THEN THE CONTROLLERS ARE IN
		// MESH ORDER. This is the approach that is taken. If this does not work
		// in general (i.e. a mesh may not have a controller) then we will need to
		// get the controller of the mesh by way of libaray_visual_scenes.
		//

		xml_node LibControllers = {};
		xml_node Controller = {};
		NodeGet(Root, &LibControllers, "library_controllers");
		Assert(Model.MeshCount == (u32)LibControllers.ChildrenCount);

		//
		// NOTE(Justin): Get the controller of the current mesh.
		//

		Controller = *LibControllers.Children[MeshIndex];
		if(Controller.ChildrenCount != 0)
		{
			Mesh.BindTransform = PushArray(Arena, 1, mat4);

			xml_node BindShape = {};
			NodeGet(&Controller, &BindShape, "bind_shape_matrix");

			ParseF32Array((f32 *)Mesh.BindTransform, 16, BindShape.InnerText);
			ParseColladaStringArray(Arena, &Controller, &Mesh.JointNames, &Mesh.JointCount);

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

			ParseColladaFloatArray(Arena, &Controller,
					&((f32 *)Mesh.InvBindTransforms), &Mesh.JointCount,
					"input", "INV_BIND_MATRIX");

			Assert(Mesh.InvBindTransforms);

			ParseColladaFloatArray(Arena, &Controller,
					&Mesh.Weights, &Mesh.WeightCount,
					"input", "WEIGHT");

			Assert(Mesh.Weights);
			Assert(Mesh.WeightCount != 0);

			xml_node VertexWeights = {};
			NodeGet(&Controller, &VertexWeights, "vertex_weights");

			u32 JointInfoCount = U32FromAttributeValue(&VertexWeights);
			u32 *JointCountArray = PushArray(Arena, JointInfoCount, u32);

			ParseColladaU32Array(Arena, &Controller, &JointCountArray, JointInfoCount, "vcount");

			xml_node NodeJointsAndWeights = {};
			NodeGet(&Controller, &NodeJointsAndWeights, "v");

			//
			// NOTE(Justin): The joint count array is a list of #'s for each
			// vertex. The # is how many joints affect the vertex.
			// In order to get the count of all the joint indices and weights add up all the #'s
			// and multiply the sum by 2. 
			//

			u32 JointsAndWeightsCount = 2 * U32ArraySum(JointCountArray, JointInfoCount);
			u32 *JointsAndWeights = PushArray(Arena, JointsAndWeightsCount, u32);
			ParseU32Array(JointsAndWeights, JointsAndWeightsCount, NodeJointsAndWeights.InnerText);

			Mesh.JointInfoCount = JointInfoCount;
			Mesh.JointsInfo = PushArray(Arena, Mesh.JointInfoCount, joint_info);

			u32 JointsAndWeightsIndex = 0;
			for(u32 JointIndex = 0; JointIndex < Mesh.JointInfoCount; ++JointIndex)
			{
				u32 JointCountForVertex = JointCountArray[JointIndex];

				joint_info *JointInfo = Mesh.JointsInfo + JointIndex;
				JointInfo->Count = JointCountForVertex;
				f32 Sum = 0.0f;
				for(u32 k = 0; k < JointInfo->Count; ++k)
				{
					JointInfo->JointIndex[k] = JointsAndWeights[JointsAndWeightsIndex++];
					u32 WeightIndex = JointsAndWeights[JointsAndWeightsIndex++];
					JointInfo->Weights[k] = Mesh.Weights[WeightIndex];
					Sum += JointInfo->Weights[k];
				}

				if(Sum != 0.0f)
				{
					JointInfo->Weights[0] /= Sum;
					JointInfo->Weights[1] /= Sum;
					JointInfo->Weights[2] /= Sum;
				}
			}
		}

		Model.Meshes[MeshIndex] = Mesh;
	}

	mesh *Mesh = Model.Meshes;

	//
	// NOTE(Justin): Animations
	//
	
	// NOTE(Justin): The next to last node of an animation node APPEARS to be the
	// sampler node which tells what the input and output nodes are.

	xml_node LibAnimations = {};
	NodeGet(Root, &LibAnimations, "library_animations");
	if(LibAnimations.ChildrenCount != 0)
	{
		Mesh->AnimationInfoCount = LibAnimations.ChildrenCount;
		Mesh->AnimationsInfo = PushArray(Arena, Mesh->AnimationInfoCount, animation_info);

		u32 AnimationInfoIndex = 0;
		animation_info *Info = Mesh->AnimationsInfo;

		AnimationInfoGet(Arena, &LibAnimations, Mesh->JointNames, Mesh->JointCount, Info, &AnimationInfoIndex);
		Assert(AnimationInfoIndex == Mesh->AnimationInfoCount);
	}

	//
	// NOTE(Justin): Visual Scenes (only joint hierarchy)
	//

	xml_node LibVisScenes = {};
	NodeGet(Root, &LibVisScenes, "library_visual_scenes");
	if(LibVisScenes.ChildrenCount != 0)
	{
		Mesh->Joints = PushArray(Arena, Mesh->JointCount, joint);

		xml_node JointRoot = {};
		// TODO(Justin): Better way to get the root joint node in tree.
		FirstNodeWithAttrValue(&LibVisScenes, &JointRoot, "JOINT");
		if(JointRoot.ChildrenCount != 0)
		{
			u32 JointIndex = 0;
			joint *Joints = Mesh->Joints;

			Joints->Name = Mesh->JointNames[0];
			Joints->ParentIndex = -1;

			JointsGet(Arena, &JointRoot, Mesh->JointNames, Mesh->JointCount, Joints, &JointIndex);
		}
	}

	return(Model);
}
