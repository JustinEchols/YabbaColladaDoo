
// TODO(Justin): String hash!
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

internal animation_info 
AnimationInitFromCollada(memory_arena *Arena, char *DaeFileName)
{
	animation_info Info = {};

	loaded_dae DaeFile = ColladaFileLoad(Arena, DaeFileName);

	xml_node *Root = DaeFile.Root;
	xml_node LibAnimations = {};
	NodeGet(Root, &LibAnimations, "library_animations");

	if(LibAnimations.ChildrenCount != 0)
	{
		u8 Delimeters[] = " \n\r";

		if(LibAnimations.ChildrenCount == 1)
		{
			LibAnimations = *LibAnimations.Children[0];
		}

		u32 JointCount = LibAnimations.ChildrenCount;

		xml_node FloatArray = {};
		NodeGet(&LibAnimations, &FloatArray, "float_array");
		string StrTimeCount = NodeAttributeValueGet(&FloatArray, "count");

		u32 TimeCount = U32FromASCII(StrTimeCount);
		Info.TimeCount = TimeCount;//U32FromASCII(StrTimeCount);
		//f32 *Times = PushArray(Arena, TimeCount, f32);
		Info.Times = PushArray(Arena, Info.TimeCount, f32);
		ParseF32Array(Info.Times, Info.TimeCount, FloatArray.InnerText);

		Info.JointCount = JointCount;
		Info.JointNames = PushArray(Arena, Info.JointCount, string);
		Info.Duration = Info.Times[Info.TimeCount - 1];
		Info.FrameRate = Info.Duration / (f32)TimeCount;

		u32 TransformCount = Info.JointCount;
		mat4 **Transforms = PushArray(Arena, TransformCount, mat4 *);
		Info.KeyFrameCount = Info.TimeCount;
		//Info.KeyFrameCount = TimeCount;
		Info.KeyFrames = PushArray(Arena, Info.KeyFrameCount, key_frame);

		for(u32 Index = 0; Index < TransformCount; ++Index)
		{
			Transforms[Index] = PushArray(Arena, TimeCount, mat4);
		}

		for(u32 Index = 0; Index < Info.KeyFrameCount; ++Index)
		{
			Info.KeyFrames[Index].Transforms = PushArray(Arena, Info.JointCount, mat4);

			Info.KeyFrames[Index].Positions = PushArray(Arena, Info.JointCount, v3);
			Info.KeyFrames[Index].Quaternions = PushArray(Arena, Info.JointCount, quaternion);
			Info.KeyFrames[Index].Scales = PushArray(Arena, Info.JointCount, v3);
		}

		for(s32 ChildIndex = 0; ChildIndex < LibAnimations.ChildrenCount; ++ChildIndex)
		{
			xml_node *Node = LibAnimations.Children[ChildIndex];
			Assert(StringsAreSame(Node->Tag, "animation"));

			string JointName = NodeAttributeValueGet(Node, "name");

			Info.JointNames[ChildIndex] = JointName;

			// TODO(Justin): Why is the index - 2?
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
			//Assert(Info.JointCount == NodeTimeCount);

			N = {};
			NodeGet(Node, &N, "source", (char *)OutputSrcName.Data);
			N = *N.Children[0];

			string_list TransformList = StringSplit(Arena, N.InnerText, (u8 *)Delimeters, ArrayCount(Delimeters));
			string_node *StrNode = TransformList.First;

			mat4 *M = Transforms[ChildIndex];
			f32 *Float = (f32 *)M;
			for(u32 Index = 0; Index < TransformList.Count; ++Index)
			{
				string S = StringAllocAndCopy(Arena, StrNode->String);
				Float[Index] = F32FromASCII(S);
				StrNode = StrNode->Next;
			}
		}

		for(u32 KeyFrameIndex = 0; KeyFrameIndex < Info.KeyFrameCount; ++KeyFrameIndex)
		{
			key_frame *KeyFrame = Info.KeyFrames + KeyFrameIndex;
			for(u32 JointIndex = 0; JointIndex < Info.JointCount; ++JointIndex)
			{
				KeyFrame->Transforms[JointIndex] = Transforms[JointIndex][KeyFrameIndex];
			}
		}

		for(u32 KeyFrameIndex = 0; KeyFrameIndex < Info.KeyFrameCount; ++KeyFrameIndex)
		{
			key_frame *KeyFrame = Info.KeyFrames + KeyFrameIndex;
			for(u32 JointIndex = 0; JointIndex < Info.JointCount; ++JointIndex)
			{
				mat4 *JointTransform = KeyFrame->Transforms + JointIndex;
				affine_decomposition D = Mat4AffineDecomposition(*JointTransform);

				v3 Position = D.P;
				quaternion Q = RotationToQuaternion(D.R);
				v3 Scale = V3(D.Cx, D.Cy, D.Cz);

				v3 *P = KeyFrame->Positions + JointIndex;
				quaternion *Orientation = KeyFrame->Quaternions + JointIndex;
				v3 *S = KeyFrame->Scales + JointIndex;

				*P = Position;
				*Orientation = Q;
				*S = Scale;
			}
		}
	}
	else
	{
		printf("library_animations not found!.");
	}

	return(Info);
}

// TODO(Justin): Debug by testing other collada files.
internal model 
ModelInitFromCollada(memory_arena *Arena, loaded_dae DaeFile)
{
	model Model = {};

	xml_node *Root = DaeFile.Root;

	xml_node LibMaterials = {};
	xml_node LibEffects = {};
	xml_node LibGeometry = {};
	xml_node LibControllers = {};
	xml_node LibVisScenes = {};
	xml_node Triangles = {};

	NodeGet(Root, &LibMaterials, "library_materials");
	NodeGet(Root, &LibEffects, "library_effects");
	NodeGet(Root, &LibGeometry, "library_geometries");
	NodeGet(Root, &LibControllers, "library_controllers");
	NodeGet(Root, &LibVisScenes, "library_visual_scenes");

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

		// TODO(Justin): Need different way of handling different number of attributes/vertex
		Assert(AttributeCount == 3);

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

		Mesh.VertexCount = IndicesCount/3;
		Mesh.Vertices = PushArray(Arena, Mesh.VertexCount, vertex);
		Mesh.IndicesCount = IndicesCount/3;
		Mesh.Indices = PushArray(Arena, Mesh.IndicesCount, u32);

		u32 Stride3 = 3;
		u32 Stride2 = 2;
		for(u32 VertexIndex = 0; VertexIndex < Mesh.VertexCount; ++VertexIndex)
		{
			Mesh.Indices[VertexIndex] = VertexIndex;

			vertex *V = Mesh.Vertices + VertexIndex;

			u32 IndexP = Indices[3 * VertexIndex + 0];
			u32 IndexN = Indices[3 * VertexIndex + 1];
			u32 IndexUV = Indices[3 * VertexIndex + 2];

			V->P.x = Positions[Stride3 * IndexP + 0];
			V->P.y = Positions[Stride3 * IndexP + 1];
			V->P.z = Positions[Stride3 * IndexP + 2];

			V->N.x = Normals[Stride3 * IndexN + 0];
			V->N.y = Normals[Stride3 * IndexN + 1];
			V->N.z = Normals[Stride3 * IndexN + 2];

			V->UV.x = UV[Stride2 * IndexUV + 0];
			V->UV.y = UV[Stride2 * IndexUV + 1];
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


		//
		// NOTE(Justin): Get the controller of the current mesh.
		//

		Assert(Model.MeshCount == (u32)LibControllers.ChildrenCount);
		xml_node Controller = {};
		Controller = *LibControllers.Children[MeshIndex];
		// TODO(Justin): Move this to an appropriate position.
		u32 JointCount = 0;
		string *JointNames;
		if(Controller.ChildrenCount != 0)
		{
			xml_node BindShape = {};
			NodeGet(&Controller, &BindShape, "bind_shape_matrix");

			ParseF32Array(&Mesh.BindTransform.E[0][0], 16, BindShape.InnerText);


			//string *JointNames = PushArray(Arena, Mesh.JointCount, string);
			//ParseColladaStringArray(Arena, &Controller, &Mesh.JointNames, &Mesh.JointCount);
			ParseColladaStringArray(Arena, &Controller, &JointNames, &JointCount);

			Mesh.JointCount = JointCount;

			Assert(JointNames);
			Assert(Mesh.JointCount != 0);

			Mesh.JointTransforms = PushArray(Arena, Mesh.JointCount, mat4);
			Mesh.ModelSpaceTransforms = PushArray(Arena, Mesh.JointCount, mat4);

			mat4 I = Mat4Identity();
			for(u32 MatIndex = 0; MatIndex < Mesh.JointCount; ++MatIndex)
			{
				Mesh.JointTransforms[MatIndex] = I;
				Mesh.ModelSpaceTransforms[MatIndex] = I;
			}

			xml_node SourceNode = NodeSourceGet(&Controller, "input", "INV_BIND_MATRIX");
			Assert(SourceNode.Tag.Size != 0);

			string StrCount = NodeAttributeValueGet(&SourceNode, "count");
			u32 Count = U32FromASCII(StrCount.Data);

			xml_node AccessorNode = {};
			NodeGet(SourceNode.Parent, &AccessorNode, "accessor");

			string StrStride = NodeAttributeValueGet(&AccessorNode, "stride");
			u32 Stride = 0;
			if(StrStride.Size == 0)
			{
				Stride = 1;
			}
			else
			{
				Stride = U32FromASCII(StrStride.Data);
			}

			//*DestCount = Count / Stride;
			//*Dest = PushArray(Arena, Count, f32);

			//u32 DestCount = Count / Stride;
			Mesh.InvBindTransforms = PushArray(Arena, Mesh.JointCount, mat4);
			ParseF32Array(&Mesh.InvBindTransforms->E[0][0], Count, SourceNode.InnerText);
			


			//f32 *M = &Mesh.InvBindTransforms->E[0][0];
			//ParseColladaFloatArray(Arena, &Controller, &M, &Mesh.JointCount, "input", "INV_BIND_MATRIX");

			Assert(Mesh.InvBindTransforms);

			u32 WeightCount;
			f32 *Weights;
			ParseColladaFloatArray(Arena, &Controller, &Weights, &WeightCount, "input", "WEIGHT");

			Assert(Weights);
			Assert(WeightCount != 0);

			// NOTE(Justin): This node contains information for each vertex how many joints affect the vertex, what joints affect the vertex,
			// and how each one of the joints affects the vertex.
			xml_node VertexWeights = {};
			NodeGet(&Controller, &VertexWeights, "vertex_weights");

			u32 JointInfoCount = U32FromAttributeValue(&VertexWeights);
			u32 *JointCountArray = PushArray(Arena, JointInfoCount, u32);
			joint_info *JointInfoArray = PushArray(Arena, JointInfoCount, joint_info);

			Assert(JointInfoCount == (PositionCount/3));
			Assert(JointCountArray);

			ParseColladaU32Array(Arena, &Controller, &JointCountArray, JointInfoCount, "vcount");

			xml_node NodeJointsAndWeights = {};
			NodeGet(&Controller, &NodeJointsAndWeights, "v");

			//
			// NOTE(Justin): The joint count array is a list of #'s for each
			// vertex. The # is how many joints affect the vertex.
			// In order to get the count of all the joint indices and weights add up all the #'s
			// and multiply the sum by 2. 
			//

			u32 JointsAndWeightsCount = 2 * ArraySum(JointCountArray, JointInfoCount);
			u32 *JointsAndWeights = PushArray(Arena, JointsAndWeightsCount, u32);
			ParseU32Array(JointsAndWeights, JointsAndWeightsCount, NodeJointsAndWeights.InnerText);

			u32 JointsAndWeightsIndex = 0;
			for(u32 Index = 0; Index < JointInfoCount; ++Index)
			{
				joint_info *JointInfo = JointInfoArray + Index;
				u32 JointCountForVertex = JointCountArray[Index];
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

			for(u32 VertexIndex = 0; VertexIndex < Mesh.VertexCount; ++VertexIndex)
			{
				vertex *V = Mesh.Vertices + VertexIndex;
				u32 IndexP = Indices[3 * VertexIndex];
				V->JointInfo = JointInfoArray[IndexP];
			}
		}


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
			xml_node SkeletonParentNode = *LibVisScenes.Children[0]->Children[ChildIndex];

			xml_node Skeleton = {};
			NodeGet(&SkeletonParentNode, &Skeleton, "skeleton");

			string JointRootName = Skeleton.InnerText;
			JointRootName.Data++;
			JointRootName.Size--;

			xml_node JointRoot = {};
			NodeGet(&LibVisScenes, &JointRoot, "node", (char *)JointRootName.Data);
			if(JointRoot.ChildrenCount != 0)
			{
				joint *Joints = Mesh.Joints;
				Joints->Name = JointNames[0];
				Joints->ParentIndex = -1;
				ParseF32Array(Arena, &Joints->Transform.E[0][0], 16, JointRoot.Children[0]->InnerText);

				u32 JointIndex = 1;
				JointsGet(Arena, &JointRoot, JointNames, Mesh.JointCount, Joints, &JointIndex);
			}
		}

		Model.Meshes[MeshIndex] = Mesh;
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

	if(LibControllers.ChildrenCount != 0)
	{
		Model.HasSkeleton = true;
	}

	return(Model);
}
