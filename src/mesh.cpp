
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
AnimationInfoGet(memory_arena *Arena, xml_node *Root, animation_info *AnimationInfo)
{
	u32 AnimationInfoIndex = 0;
	for(s32 ChildIndex = 0; ChildIndex < Root->ChildrenCount; ++ChildIndex)
	{
		xml_node *Node = Root->Children[ChildIndex];
		Assert(StringsAreSame(Node->Tag, "animation"));

		string JointName = NodeAttributeValueGet(Node, "name");
		AnimationInfo->JointNames[AnimationInfoIndex] = JointName;

		//
		// NOTE(Justin): I have only made the observation that the second to
		// last node in an animation node is the sampler node. I do not have a
		// complete understanding of the Collada spec to say with certainty that
		// THIS IS ALWAYS TRUE.
		//

		xml_node Sampler = *Node->Children[Node->ChildrenCount - 2];
		xml_node Input = *Sampler.Children[0];
		xml_node Output = *Sampler.Children[1];

		string InputSrcName = NodeAttributeValueGet(&Input, "source");
		string OutputSrcName = NodeAttributeValueGet(&Output, "source");

		InputSrcName.Data++;
		OutputSrcName.Data++;

		InputSrcName.Size--;
		OutputSrcName.Size--;

		xml_node N = {};
		NodeGet(Node, &N, "source", (char *)InputSrcName.Data);
		N = *N.Children[0];

		string Count = NodeAttributeValueGet(&N, "count");
		u32 TimeCount = U32FromASCII(Count.Data);

		if(ChildIndex == 0)
		{
			AnimationInfo->TimeCount = TimeCount;
			AnimationInfo->TransformCount = TimeCount;
		}
		else
		{
			Assert(AnimationInfo->TimeCount == TimeCount);
			Assert(AnimationInfo->TransformCount == TimeCount);
		}

		AnimationInfo->Times[AnimationInfoIndex] = PushArray(Arena, TimeCount, f32);
		ParseF32Array(AnimationInfo->Times[AnimationInfoIndex], TimeCount, N.InnerText);

		N = {};
		NodeGet(Node, &N, "source", (char *)OutputSrcName.Data);
		N = *N.Children[0];

		AnimationInfo->Transforms[AnimationInfoIndex] = PushArray(Arena, TimeCount * 16, mat4);
		ParseF32Array((f32 *)AnimationInfo->Transforms[AnimationInfoIndex], TimeCount * 16, N.InnerText);

		AnimationInfoIndex++;
	}
}

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

			// NOTE(Justin): The two meshes of XBot use slightly different joint
			// hierarchies. The second mesh has one less node than the first. To
			// initilize the second mesh we use the original joint hierarchy
			// that contains all the joints. Since the second mesh does not have
			// all the nodes when we look up the joint index a return value of
			// -1 is possible. This happens when we are at a Node in the joint
			// hierarchy that is NOT part of the second mesh.

			s32 Index = JointIndexGet(JointNames, JointCount, SID.Value);
			if(Index != -1)
			{
				s32 ParentIndex = JointIndexGet(JointNames, JointCount, ParentSID.Value);
				Assert(Index != - 1);
				Assert(ParentIndex != - 1);

				//Joint->Index = (u32)Index;
				Joint->ParentIndex = (u32)ParentIndex;

				xml_node *Child = Node->Children[0];
				if(StringsAreSame(Child->Tag, "matrix"))
				{
					ParseF32Array(Arena, &Joint->Transform.E[0][0], 16, Child->InnerText);
					(*JointIndex)++;
				}
			}
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

	xml_node LibMaterials = {};
	xml_node LibEffects = {};
	xml_node LibGeometry = {};
	xml_node Triangles = {};

	NodeGet(Root, &LibMaterials, "library_materials");
	NodeGet(Root, &LibEffects, "library_effects");
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

		xml_node *Mat = LibMaterials.Children[MeshIndex];
		string EffectNodeName = NodeAttributeValueGet(Mat->Children[0], "url");
		EffectNodeName.Data++;
		EffectNodeName.Size--;

		xml_node Effect = {};
		xml_node Phong = {};
		NodeGet(Root, &Effect, "effect", (char *)EffectNodeName.Data);
		NodeGet(&Effect, &Phong, "phong");

		for(s32 k = 0; k < Phong.ChildrenCount; ++k)
		{
			xml_node *Child = Phong.Children[k];
			if(StringsAreSame(Child->Tag, "ambient"))
			{
				xml_node Color = *Child->Children[0];
				ParseF32Array(&Mesh.MaterialSpec.Ambient.E[0], 4, Color.InnerText);
			}

			if(StringsAreSame(Child->Tag, "diffuse"))
			{
				xml_node Color = *Child->Children[0];
				ParseF32Array(&Mesh.MaterialSpec.Diffuse.E[0], 4, Color.InnerText);
			}

			if(StringsAreSame(Child->Tag, "specular"))
			{
				xml_node Color = *Child->Children[0];
				ParseF32Array(&Mesh.MaterialSpec.Specular.E[0], 4, Color.InnerText);
			}

			if(StringsAreSame(Child->Tag, "shininess"))
			{
				xml_node Color = *Child->Children[0];
				Mesh.MaterialSpec.Shininess = F32FromASCII(Color.InnerText);
			}
		}

		xml_node *Geometry = LibGeometry.Children[MeshIndex];

		Mesh.Name = NodeAttributeValueGet(Geometry, "name");

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

		u32 Stride3 = 3;
		u32 Stride2 = 2;
		if(AttributeCount == 2)
		{
			for(u32 i = 0; i < IndicesCount; i += AttributeCount)
			{
				u32 IndexP = Indices[i];
				b32 IndexIsUnique = UniqueIndexTable[IndexP];
				if(IndexIsUnique)
				{
					u32 IndexN = Indices[i + 1];

					f32 X = Positions[Stride3 * IndexP];
					f32 Y = Positions[Stride3 * IndexP + 1];
					f32 Z = Positions[Stride3 * IndexP + 2];

					f32 Nx = Normals[Stride3 * IndexN];
					f32 Ny = Normals[Stride3 * IndexN + 1];
					f32 Nz = Normals[Stride3 * IndexN + 2];

					Mesh.Positions[Stride3 * IndexP] = X;
					Mesh.Positions[Stride3 * IndexP + 1] = Y;
					Mesh.Positions[Stride3 * IndexP + 2] = Z;

					Mesh.Normals[Stride3 * IndexP] = Nx;
					Mesh.Normals[Stride3 * IndexP + 1] = Ny;
					Mesh.Normals[Stride3 * IndexP + 2] = Nz;

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

					f32 X = Positions[Stride3 * IndexP];
					f32 Y = Positions[Stride3 * IndexP + 1];
					f32 Z = Positions[Stride3 * IndexP + 2];

					f32 Nx = Normals[Stride3 * IndexN];
					f32 Ny = Normals[Stride3 * IndexN + 1];
					f32 Nz = Normals[Stride3 * IndexN + 2];

					f32 U = UV[Stride2 * IndexUV];
					f32 V = UV[Stride2 * IndexUV + 1];

					Mesh.Positions[Stride3 * IndexP] = X;
					Mesh.Positions[Stride3 * IndexP + 1] = Y;
					Mesh.Positions[Stride3 * IndexP + 2] = Z;

					Mesh.Normals[Stride3 * IndexP] = Nx;
					Mesh.Normals[Stride3 * IndexP + 1] = Ny;
					Mesh.Normals[Stride3 * IndexP + 2] = Nz;

					Mesh.UV[Stride2 * IndexP] = U;
					Mesh.UV[Stride2 * IndexP + 1] = V;

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
			xml_node BindShape = {};
			NodeGet(&Controller, &BindShape, "bind_shape_matrix");

			ParseF32Array(&Mesh.BindTransform.E[0][0], 16, BindShape.InnerText);
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

			u32 WeightCount;
			f32 *Weights;
			ParseColladaFloatArray(Arena, &Controller,
					&Weights, &WeightCount,
					"input", "WEIGHT");

			Assert(Weights);
			Assert(WeightCount != 0);

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
					JointInfo->Weights[k] = Weights[WeightIndex];
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

		xml_node LibVisScenes = {};
		NodeGet(Root, &LibVisScenes, "library_visual_scenes");
		if(LibVisScenes.ChildrenCount != 0)
		{
			Mesh.Joints = PushArray(Arena, Mesh.JointCount, joint);

			//
			// NOTE(Justin): The last nodes of visual scene contain the name of the
			// node where the root joint of the joint hierarchy can be found.
			// This is in the <skeleton> node. This approach also assumes they
			// are in mesh order. Meaning the last skeleton is the skeleton of
			// the last mesh. the second to last skeleton is the skeleton of
			// the second to last mesh and so on.
			//

			u32 ChildIndex = (LibVisScenes.Children[0]->ChildrenCount - Model.MeshCount) + MeshIndex;
			xml_node Node = *LibVisScenes.Children[0]->Children[ChildIndex];

			xml_node Skeleton = {};
			NodeGet(&Node, &Skeleton, "skeleton");

			string JointRootName = Skeleton.InnerText;
			JointRootName.Data++;
			JointRootName.Size--;

			xml_node JointRoot = {};
			NodeGet(&LibVisScenes, &JointRoot, "node", (char *)JointRootName.Data);
			if(JointRoot.ChildrenCount != 0)
			{
				joint *Joints = Mesh.Joints;
				Joints->Name = Mesh.JointNames[0];
				Joints->ParentIndex = -1;
				ParseF32Array(Arena, &Joints->Transform.E[0][0], 16, JointRoot.Children[0]->InnerText);

				u32 JointIndex = 1;
				JointsGet(Arena, &JointRoot, Mesh.JointNames, Mesh.JointCount, Joints, &JointIndex);
			}
		}

		Model.Meshes[MeshIndex] = Mesh;
	}

	//
	// NOTE(Justin): Animations
	//
	
	xml_node LibAnimations = {};
	NodeGet(Root, &LibAnimations, "library_animations");
	if(LibAnimations.ChildrenCount != 0)
	{
		Model.AnimationsInfo.JointCount = LibAnimations.ChildrenCount;
		Model.AnimationsInfo.JointNames = PushArray(Arena, Model.AnimationsInfo.JointCount, string); 
		Model.AnimationsInfo.Times = PushArray(Arena, Model.AnimationsInfo.JointCount, f32 *);
		Model.AnimationsInfo.Transforms = PushArray(Arena, Model.AnimationsInfo.JointCount, mat4 *);

		AnimationInfoGet(Arena, &LibAnimations, &Model.AnimationsInfo);
	}

	//
	// NOTE(Justin): Pre-multiply bind transform with each joints inverse bind
	// transform.
	//

	for(u32 MeshIndex = 0; MeshIndex < Model.MeshCount; ++MeshIndex)
	{
		mesh *Mesh = Model.Meshes + MeshIndex;
		mat4 Bind = Mesh->BindTransform;
		for(u32 Index = 0; Index < Mesh->JointCount; ++Index)
		{
			mat4 InvBind = Mesh->InvBindTransforms[Index];
			Mesh->InvBindTransforms[Index] = InvBind * Bind;
		}
	}

	return(Model);
}

internal void
FileWriteHeader(FILE *OutputFile, char *HeaderName, string HeaderData)
{
	fputs(HeaderName, OutputFile);
	fputs(" ", OutputFile);
	fputs((char *)HeaderData.Data, OutputFile);
	fputs("\n", OutputFile);
}

internal void
FileWriteHeader(FILE *OutputFile, char *HeaderName, char *HeaderData)
{
	fputs(HeaderName, OutputFile);
	fputs(" ", OutputFile);
	fputs(HeaderData, OutputFile);
	fputs("\n", OutputFile);
}

internal void
FileWriteStringArrayWithSpaces(FILE *OutputFile, string *Str, u32 Count)
{
	for(u32 Index = 0; Index < Count; ++Index)
	{
		char *Cstr = (char *)Str[Index].Data;
		fputs(Cstr, OutputFile);
		fputs(" ", OutputFile);
	}
	fputs("\n", OutputFile);
}

internal void
FileWriteStringArrayWithNewLines(FILE *OutputFile, string *Str, u32 Count, u32 NewLineStride)
{
	u32 StrideCount = 0;
	for(u32 Index = 0; Index < Count; ++Index)
	{
		char *Cstr = (char *)Str[Index].Data;
		fputs(Cstr, OutputFile);
		if(StrideCount < NewLineStride - 1)
		{
			fputs(" ", OutputFile);
			StrideCount++;
		}
		else
		{
			fputs("\n", OutputFile);
			StrideCount = 0;
		}
	}
	fputs("\n", OutputFile);
}

internal void
FileWriteJoints(FILE *OutputFile, memory_arena *Arena, xml_node *Root, string *JointNames, u32 JointCount)
{
	for(s32 ChildIndex = 0; ChildIndex < Root->ChildrenCount; ++ChildIndex)
	{
		xml_node *Node = Root->Children[ChildIndex];
		Assert(Node);
		if(StringsAreSame(Node->Tag, "node"))
		{
			string SID = NodeAttributeValueGet(Node, "sid");
			s32 Index = JointIndexGet(JointNames, JointCount, SID);
			if(Index != -1)
			{
				string ParentSID = NodeAttributeValueGet(Node->Parent, "sid");

				fputs((char *)SID.Data, OutputFile);
				fputs("\n", OutputFile);

				s32 ParentIndex = JointIndexGet(JointNames, JointCount, ParentSID);
				Assert(ParentIndex != - 1);

				string StrParentIndex = DigitToString(Arena, ParentIndex);
				fputs((char *)StrParentIndex.Data, OutputFile);
				fputs("\n", OutputFile);

				xml_node *Child = Node->Children[0];
				if(StringsAreSame(Child->Tag, "matrix"))
				{

					string StrTransform = Child->InnerText;
					fputs((char *)StrTransform.Data, OutputFile);
					fputs("\n", OutputFile);
				}
			}
		}

		if(*Node->Children)
		{
			FileWriteJoints(OutputFile, Arena, Node, JointNames, JointCount);
		}
	}
}

internal void 
ConvertMeshFormat(memory_arena *Arena, loaded_dae DaeFile, char *OutputFileName)
{
	FILE *OutputFile = fopen(OutputFileName, "w");
	if(OutputFile)
	{
		xml_node *Root = DaeFile.Root;

		xml_node LibMaterials = {};
		xml_node LibEffects = {};
		xml_node LibGeometry = {};
		xml_node Triangles = {};

		NodeGet(Root, &LibMaterials, "library_materials");
		NodeGet(Root, &LibEffects, "library_effects");
		NodeGet(Root, &LibGeometry, "library_geometries");

		//
		// NOTE(Justin): The number of meshes is the number of children that
		// library_geometries has. Each geometry node has AT MOST one mesh child
		// node. The LAST CHILD NODE of a mesh node contains two things. A)
		// Where to find positions, normals, UVs, colors and B) The indices of the
		// mesh.
		//

		u32 MeshCount = LibGeometry.ChildrenCount;
		string StrMeshCount = DigitToString(Arena, MeshCount);
		FileWriteHeader(OutputFile, "MESHES:", StrMeshCount);

		for(u32 MeshIndex = 0; MeshIndex < MeshCount; ++MeshIndex)
		{
			xml_node *Geometry = LibGeometry.Children[MeshIndex];

			string MeshName = NodeAttributeValueGet(Geometry, "name");

			xml_node *Mat = LibMaterials.Children[MeshIndex];
			string EffectNodeName = NodeAttributeValueGet(Mat->Children[0], "url");
			EffectNodeName.Data++;
			EffectNodeName.Size--;

			xml_node Effect = {};
			xml_node Phong = {};
			NodeGet(Root, &Effect, "effect", (char *)EffectNodeName.Data);
			NodeGet(&Effect, &Phong, "phong");

			string_array StrLighting = StringArrayAllocate(Arena, 4);
			for(s32 k = 0; k < Phong.ChildrenCount; ++k)
			{
				xml_node *Child = Phong.Children[k];

				if(StringsAreSame(Child->Tag, "ambient"))
				{
					xml_node Color = *Child->Children[0];
					StrLighting.Strings[0] = Color.InnerText;
				}

				if(StringsAreSame(Child->Tag, "diffuse"))
				{
					xml_node Color = *Child->Children[0];
					StrLighting.Strings[1] = Color.InnerText;
				}

				if(StringsAreSame(Child->Tag, "specular"))
				{
					xml_node Color = *Child->Children[0];
					StrLighting.Strings[2] = Color.InnerText;
				}

				if(StringsAreSame(Child->Tag, "shininess"))
				{
					xml_node Color = *Child->Children[0];
					StrLighting.Strings[3] = Color.InnerText;
				}
			}

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

			char Delimeters[] = " \n\r";
			string_array TempIndices = StringSplitIntoArray(Arena, NodeIndex.InnerText, (u8 *)Delimeters, 3);
			string_array TempPositions = StringSplitIntoArray(Arena, NodePos.InnerText, (u8 *)Delimeters, 3);
			string_array TempNormals = StringSplitIntoArray(Arena, NodeNormal.InnerText, (u8 *)Delimeters, 3);
			string_array TempUVs = StringSplitIntoArray(Arena, NodeUV.InnerText, (u8 *)Delimeters, 3);

			Assert(TempIndices.Count == IndicesCount);
			Assert(TempPositions.Count == PositionCount);
			Assert(TempNormals.Count == NormalCount);
			Assert(TempUVs.Count == UVCount);

			if(NormalCount != 0)
			{
				NormalCount = PositionCount;
			}

			if(UVCount != 0)
			{
				UVCount = 2 * (PositionCount / 3);
			}

			string_array StrIndices = StringArrayAllocate(Arena, 3 * TriangleCount);
			string_array StrPositions = StringArrayAllocate(Arena, PositionCount);
			string_array StrNormals = StringArrayAllocate(Arena, NormalCount);
			string_array StrUVs = StringArrayAllocate(Arena, UVCount);

			u32 TriIndex = 0;
			for(u32 i = 0; i < IndicesCount; i += AttributeCount)
			{
				StrIndices.Strings[TriIndex++] = TempIndices.Strings[i];

			}

			//
			// NOTE(Justin): Unify vertex data
			//

			u32 *Indices = PushArray(Arena, IndicesCount, u32);
			ParseU32Array(Indices, IndicesCount, NodeIndex.InnerText);
			b32 *UniqueIndexTable = PushArray(Arena, PositionCount / 3, b32);
			for(u32 i = 0; i < PositionCount / 3; ++i)
			{
				UniqueIndexTable[i] = true;
			}

			u32 Stride3 = 3;
			u32 Stride2 = 2;
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

						string X = TempPositions.Strings[Stride3 * IndexP];
						string Y = TempPositions.Strings[Stride3 * IndexP + 1];
						string Z = TempPositions.Strings[Stride3 * IndexP + 2];

						string Nx = TempNormals.Strings[Stride3 * IndexN];
						string Ny = TempNormals.Strings[Stride3 * IndexN + 1];
						string Nz = TempNormals.Strings[Stride3 * IndexN + 2];

						string U = TempUVs.Strings[Stride2 * IndexUV];
						string V = TempUVs.Strings[Stride2 * IndexUV + 1];

						StrPositions.Strings[Stride3 * IndexP] = X;
						StrPositions.Strings[Stride3 * IndexP + 1] = Y;
						StrPositions.Strings[Stride3 * IndexP + 2] = Z;

						StrNormals.Strings[Stride3 * IndexP] = Nx;
						StrNormals.Strings[Stride3 * IndexP + 1] = Ny;
						StrNormals.Strings[Stride3 * IndexP + 2] = Nz;

						StrUVs.Strings[Stride2 * IndexP] = U;
						StrUVs.Strings[Stride2 * IndexP + 1] = V;

						UniqueIndexTable[IndexP] = false;
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
				Assert(MeshCount == (u32)LibControllers.ChildrenCount);

				//
				// NOTE(Justin): Controller of the current mesh.
				//

				Controller = *LibControllers.Children[MeshIndex];

				string_array JointNames = {};
				string_array StrBind = {};
				string_array StrInvBind = {};
				string_array JointInfo = {};
				if(Controller.ChildrenCount != 0)
				{
					xml_node NameArray = {};
					NodeGet(&Controller, &NameArray, "Name_array");

					xml_node BindShape = {};
					NodeGet(&Controller, &BindShape, "bind_shape_matrix");
					xml_node InvBind = NodeSourceGet(&Controller, "input", "INV_BIND_MATRIX");


					JointNames = StringSplitIntoArray(Arena, NameArray.InnerText, (u8 *)Delimeters, 3);
					StrBind = StringSplitIntoArray(Arena, BindShape.InnerText, (u8 *)Delimeters, 3);
					StrInvBind = StringSplitIntoArray(Arena, InvBind.InnerText, (u8 *)Delimeters, 3);

					u32 WeightCount;
					f32 *Weights;
					ParseColladaFloatArray(Arena, &Controller, &Weights, &WeightCount, "input", "WEIGHT");

					xml_node VertexWeights = {};
					NodeGet(&Controller, &VertexWeights, "vertex_weights");

					u32 JointInfoCount = U32FromAttributeValue(&VertexWeights);
					u32 *JointCountArray = PushArray(Arena, JointInfoCount, u32);

					JointInfo = StringArrayAllocate(Arena, JointInfoCount);

					ParseColladaU32Array(Arena, &Controller, &JointCountArray, JointInfoCount, "vcount");

					xml_node NodeJointsAndWeights = {};
					NodeGet(&Controller, &NodeJointsAndWeights, "v");

					u32 JointsAndWeightsCount = 2 * U32ArraySum(JointCountArray, JointInfoCount);
					u32 *JointsAndWeights = PushArray(Arena, JointsAndWeightsCount, u32);
					ParseU32Array(JointsAndWeights, JointsAndWeightsCount, NodeJointsAndWeights.InnerText);

					u32 JointsAndWeightsIndex = 0;
					for(u32 JointIndex = 0; JointIndex < JointInfoCount; ++JointIndex)
					{
						u32 JointCountForVertex = JointCountArray[JointIndex];
						u32 JointIndices[3] = {};
						f32 JointWeights[3] = {};
						f32 Sum = 0.0f;

						for(u32 k = 0; k < JointCountForVertex; ++k)
						{
							JointIndices[k] = JointsAndWeights[JointsAndWeightsIndex++];

							u32 WeightIndex = JointsAndWeights[JointsAndWeightsIndex++];
							JointWeights[k] = Weights[WeightIndex];

							Sum += JointWeights[k];
						}

						if(Sum != 0.0f)
						{
							JointWeights[0] /= Sum;
							JointWeights[1] /= Sum;
							JointWeights[2] /= Sum;
						}

						string StrJointCountForVertex = DigitToString(Arena, JointCountForVertex);
						string StrJointIndices[3] = {};
						string StrJointWeights[3] = {};

						StrJointIndices[0] = DigitToString(Arena, JointIndices[0]);
						StrJointIndices[1] = DigitToString(Arena, JointIndices[1]);
						StrJointIndices[2] = DigitToString(Arena, JointIndices[2]);
						StrJointWeights[0] = F32ToString(Arena, JointWeights[0]);
						StrJointWeights[1] = F32ToString(Arena, JointWeights[1]);
						StrJointWeights[2] = F32ToString(Arena, JointWeights[2]);

						char Buff[256] = {};
						u32 i;
						for(i = 0; i < StrJointCountForVertex.Size; ++i)
						{
							Buff[i] = StrJointCountForVertex.Data[i];
						}
						Buff[i++] = '\n';

						for(u32 j = 0; j < 3; ++j)
						{
							string S = StrJointIndices[j];
							for(u32 k = 0; k < S.Size; ++k)
							{
								Buff[i++] = S.Data[k];
							}
							Buff[i++] = ' ';
						}
						Buff[i++] = '\n';

						for(u32 j = 0; j < 3; ++j)
						{
							string S = StrJointWeights[j];
							for(u32 k = 0; k < S.Size; ++k)
							{
								Buff[i++] = S.Data[k];
							}
							Buff[i++] = ' ';
						}
						Buff[i++] = '\n';
						Buff[i] = 0;

						JointInfo.Strings[JointIndex] = StringAllocAndCopy(Arena, Buff);
					}
				}

				string StrIndicesCount = DigitToString(Arena, StrIndices.Count);
				string StrAttributeCount = DigitToString(Arena, AttributeCount);
				string StrPositionsCount = DigitToString(Arena, StrPositions.Count);
				string StrNormalCount = DigitToString(Arena, StrNormals.Count);
				string StrUVCount = DigitToString(Arena, StrUVs.Count);
				string StrJointCount = DigitToString(Arena, JointNames.Count);
				string StrInvBindCount = DigitToString(Arena, StrInvBind.Count);
				string StrJointInfoCount = DigitToString(Arena, JointInfo.Count);

				FileWriteHeader(OutputFile, "MESH:", MeshName);
				FileWriteHeader(OutputFile, "LIGHTING:", "Phong");
				FileWriteStringArrayWithSpaces(OutputFile, StrLighting.Strings, 1);
				FileWriteStringArrayWithSpaces(OutputFile, StrLighting.Strings + 1, 1);
				FileWriteStringArrayWithSpaces(OutputFile, StrLighting.Strings + 2, 1);
				FileWriteStringArrayWithSpaces(OutputFile, StrLighting.Strings + 3, 1);

				FileWriteHeader(OutputFile, "INDICES:", StrIndicesCount);
				FileWriteStringArrayWithSpaces(OutputFile, StrIndices.Strings, StrIndices.Count);
				FileWriteHeader(OutputFile, "VERTEX_ATTRIBUTES:", StrAttributeCount);
				FileWriteHeader(OutputFile, "POSITIONS:", StrPositionsCount);
				FileWriteStringArrayWithSpaces(OutputFile, StrPositions.Strings, StrPositions.Count);
				FileWriteHeader(OutputFile, "NORMALS:", StrNormalCount);
				FileWriteStringArrayWithSpaces(OutputFile, StrNormals.Strings, StrNormals.Count);
				FileWriteHeader(OutputFile, "UVS:", StrUVCount);
				FileWriteStringArrayWithSpaces(OutputFile, StrUVs.Strings, StrUVs.Count);
				FileWriteHeader(OutputFile, "JOINT_INFO:", StrJointInfoCount);
				FileWriteStringArrayWithSpaces(OutputFile, JointInfo.Strings, JointInfo.Count);
				FileWriteHeader(OutputFile, "JOINTS:", StrJointCount);

				xml_node LibVisScenes = {};
				NodeGet(Root, &LibVisScenes, "library_visual_scenes");
				if(LibVisScenes.ChildrenCount != 0)
				{
					u32 ChildIndex = (LibVisScenes.Children[0]->ChildrenCount - MeshCount) + MeshIndex;
					xml_node Node = *LibVisScenes.Children[0]->Children[ChildIndex];

					xml_node Skeleton = {};
					NodeGet(&Node, &Skeleton, "skeleton");

					string JointRootName = Skeleton.InnerText;
					JointRootName.Data++;
					JointRootName.Size--;

					xml_node JointRoot = {};
					NodeGet(&LibVisScenes, &JointRoot, "node", (char *)JointRootName.Data);
					if(JointRoot.ChildrenCount != 0)
					{
						xml_node *FirstChild = JointRoot.Children[0];
						string Transform = FirstChild->InnerText;

						fputs((char *)JointNames.Strings[0].Data, OutputFile);
						fputs("\n", OutputFile);
						fputs("-1", OutputFile);
						fputs("\n", OutputFile);
						fputs((char *)Transform.Data, OutputFile);
						fputs("\n", OutputFile);

						FileWriteJoints(OutputFile, Arena, &JointRoot, JointNames.Strings, JointNames.Count);
					}
				}

				FileWriteHeader(OutputFile, "BIND:", "16");
				FileWriteStringArrayWithSpaces(OutputFile, StrBind.Strings, StrBind.Count);
				FileWriteHeader(OutputFile, "INV_BIND:", StrInvBindCount);
				FileWriteStringArrayWithNewLines(OutputFile, StrInvBind.Strings, StrInvBind.Count, 16);

				fputs("*\n", OutputFile);
			}
		}

		fclose(OutputFile);
	}
	else
	{
		perror("ERROR could not open file");
	}
}

internal void
ConvertAnimationFormat(memory_arena *Arena, loaded_dae DaeFile, char *OutputFileName)
{
	FILE *OutputFile = fopen(OutputFileName, "w");
	if(OutputFile)
	{
		xml_node *Root = DaeFile.Root;
		xml_node LibAnimations = {};
		NodeGet(Root, &LibAnimations, "library_animations");
		if(LibAnimations.ChildrenCount != 0)
		{

			u32 JointCount = LibAnimations.ChildrenCount;
			u32 TimeCount = 0;
			u32 TransformCount = 0;

			xml_node FloatArray = {};
			NodeGet(&LibAnimations, &FloatArray, "float_array");
			string FloatCount = NodeAttributeValueGet(&FloatArray, "count");

			TimeCount = U32FromASCII(FloatCount);
			TransformCount = TimeCount;

			string StrJointCount = DigitToString(Arena, JointCount);
			fputs("JOINTS: ", OutputFile);
			fputs((char *)StrJointCount.Data, OutputFile);
			fputs("\n", OutputFile);

			fputs("TIMES: ", OutputFile);
			fputs((char *)FloatCount.Data, OutputFile);
			fputs("\n", OutputFile);

			fputs("TRANSFORMS: ", OutputFile);
			fputs((char *)FloatCount.Data, OutputFile);
			fputs("\n", OutputFile);

			fputs("*\n", OutputFile);

			string_array JointNames = StringArrayAllocate(Arena, JointCount);

			for(s32 ChildIndex = 0; ChildIndex < LibAnimations.ChildrenCount; ++ChildIndex)
			{
				xml_node *Node = LibAnimations.Children[ChildIndex];
				Assert(StringsAreSame(Node->Tag, "animation"));

				string JointName = NodeAttributeValueGet(Node, "name");
				fputs((char *)JointName.Data, OutputFile);
				fputs("\n", OutputFile);

				xml_node Sampler = *Node->Children[Node->ChildrenCount - 2];
				xml_node Input = *Sampler.Children[0];
				xml_node Output = *Sampler.Children[1];

				string InputSrcName = NodeAttributeValueGet(&Input, "source");
				string OutputSrcName = NodeAttributeValueGet(&Output, "source");

				InputSrcName.Data++;
				OutputSrcName.Data++;

				InputSrcName.Size--;
				OutputSrcName.Size--;

				xml_node N = {};
				NodeGet(Node, &N, "source", (char *)InputSrcName.Data);
				N = *N.Children[0];

				string Count = NodeAttributeValueGet(&N, "count");

				u32 NodeTimeCount = U32FromASCII(Count.Data);
				Assert(TimeCount == NodeTimeCount);
				Assert(TransformCount == NodeTimeCount);

				char Delimeters[] = " \n\r";
				string_list TimeList = StringSplit(Arena, N.InnerText, (u8 *)Delimeters, 3);
				string_node *StrNode = TimeList.First;
				for(u32 Index = 0; Index < TimeList.Count; ++Index)
				{

					string S = StringAllocAndCopy(Arena, StrNode->String);
					char *Cstr = (char *)S.Data;
					fputs(Cstr, OutputFile);
					fputs(" ", OutputFile);
					StrNode = StrNode->Next;
				}
				fputs("\n", OutputFile);

				N = {};
				NodeGet(Node, &N, "source", (char *)OutputSrcName.Data);
				N = *N.Children[0];

				string_list TransformList = StringSplit(Arena, N.InnerText, (u8 *)Delimeters, 3);
				StrNode = TransformList.First;
				for(u32 Index = 0; Index < TransformList.Count; ++Index)
				{
					string S = StringAllocAndCopy(Arena, StrNode->String);
					char *Cstr = (char *)S.Data;
					fputs(Cstr, OutputFile);
					fputs(" ", OutputFile);
					StrNode = StrNode->Next;
				}
				fputs("\n", OutputFile);
				fputs("\n", OutputFile);
			}
		}
	}
	else
	{
		perror("Could not open file");
	}
}
