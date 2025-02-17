
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

#if 1

// TODO(Justin): Clean this up.
internal model 
ModelInitFromCollada(memory_arena *Arena, loaded_dae DaeFile)
{
	model Model = {};

	xml_node *Root = DaeFile.Root;

	xml_node LibImages = {};
	xml_node LibMaterials = {};
	xml_node LibEffects = {};
	xml_node LibGeometry = {};
	xml_node LibControllers = {};
	xml_node LibVisScenes = {};
	xml_node Triangles = {};

	NodeGet(Root, &LibImages, "library_images");
	NodeGet(Root, &LibMaterials, "library_materials");
	NodeGet(Root, &LibEffects, "library_effects");
	NodeGet(Root, &LibGeometry, "library_geometries");
	NodeGet(Root, &LibControllers, "library_controllers");
	NodeGet(Root, &LibVisScenes, "library_visual_scenes");

	// NOTE(Justin): The number of meshes is the number of children that
	// library_geometries has. Each geometry node has AT MOST one mesh child
	// node. The LAST CHILD NODE of a mesh node contains two things. A)
	// Where to find positions, normals, UVs, colors and B) The indices of the
	// mesh.

	// NOTE/TODO(Justin): Parse the file bottom up. The visual scene node seems to have nodes
	// that point to all the data needed to load the model

	// NOTE(Justin): The number of textures used in the model is the number of children of the LibImages
	// node

	// TODO(Justin): Allocate texture array up front. Each mesh can index into this array to pull out the correct
	// material for the mesh.

	Model.FullPath = DaeFile.FullPath;

	Model.MeshCount = LibGeometry.ChildrenCount;
	Model.Meshes = PushArray(Arena, Model.MeshCount, mesh);

	for(u32 MeshIndex = 0; MeshIndex < Model.MeshCount; ++MeshIndex)
	{
		mesh Mesh = {};

		xml_node *Geometry = LibGeometry.Children[MeshIndex];
		Mesh.Name = NodeAttributeValueGet(Geometry, "name");

		Triangles = {};
		xml_node *MeshNode = Geometry->Children[0];
		NodeGet(MeshNode, &Triangles, "polylist");
		if(Triangles.Tag.Size == 0)
		{
			NodeGet(MeshNode, &Triangles, "triangles");
		}
		Assert(Triangles.Tag.Size != 0);

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

		// Models should have at least 3 attributes: positions, normals, and uvs.
		Assert(AttributeCount >= 3);

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
				if(NodeUV.Tag.Size == 0)
				{
					NodeGet(Geometry, &N, "source", (char *)Value.Data);
					NodeUV = *(N.Children[0]);
				}
			}
			else if(StringsAreSame(Semantic, "COLOR"))
			{
				NodeGet(Geometry, &N, "float_array", (char *)Value.Data);
				NodeColor = *(N.Children[0]);
			}
		}

		//NodeGet(Geometry, &NodeIndex, "p");
		NodeGet(&Triangles, &NodeIndex, "p");

		u32 TriangleCount = U32FromAttributeValue(NodeIndex.Parent);
		u32 AttributesPerVertex = AttributeCount;
		u32 VerticesPerTriangle = 3;

		u32 IndicesCount = AttributesPerVertex * VerticesPerTriangle * TriangleCount;
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

		Mesh.VertexCount = IndicesCount/AttributeCount;
		Mesh.Vertices = PushArray(Arena, Mesh.VertexCount, vertex);
		Mesh.IndicesCount = Mesh.VertexCount;
		Mesh.Indices = PushArray(Arena, Mesh.IndicesCount, u32);

		u32 Stride3 = 3;
		u32 Stride2 = 2;
		for(u32 VertexIndex = 0; VertexIndex < Mesh.VertexCount; ++VertexIndex)
		{
			Mesh.Indices[VertexIndex] = VertexIndex;

			vertex *V = Mesh.Vertices + VertexIndex;

			u32 IndexP = Indices[AttributeCount * VertexIndex + 0];
			u32 IndexN = Indices[AttributeCount * VertexIndex + 1];
			u32 IndexUV = Indices[AttributeCount * VertexIndex + 2];

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

		// TODO(Justin): Move this to an appropriate position.
		if(LibControllers.ChildrenCount != 0)
		{
			Assert(Model.MeshCount == (u32)LibControllers.ChildrenCount);
			xml_node Controller = {};
			Controller = *LibControllers.Children[MeshIndex];

			if(Controller.ChildrenCount != 0)
			{
				xml_node BindShape = {};
				NodeGet(&Controller, &BindShape, "bind_shape_matrix");
				ParseF32Array(&Mesh.BindTransform.E[0][0], 16, BindShape.InnerText);

				xml_node NameArray = {};
				NodeGet(&Controller, &NameArray, "Name_array");
				Mesh.JointCount = U32FromAttributeValue(&NameArray);
				Assert(Mesh.JointCount != 0);
				Mesh.JointNames = PushArray(Arena, Mesh.JointCount, string);
				ParseStringArray(Arena, Mesh.JointNames, NameArray.InnerText);

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
				u32 Count = U32FromAttributeValue(&SourceNode);
				Assert(Count != 0);

				Mesh.InvBindTransforms = PushArray(Arena, Mesh.JointCount, mat4);
				ParseF32Array(&Mesh.InvBindTransforms->E[0][0], Count, SourceNode.InnerText);

				xml_node AccessorNode = {};
				NodeGet(SourceNode.Parent, &AccessorNode, "accessor");

				u32 WeightCount;
				f32 *Weights;
				ParseColladaFloatArray(Arena, &Controller, &Weights, &WeightCount, "input", "WEIGHT");
				Assert(WeightCount != 0);

				// NOTE(Justin): This node contains information for each vertex how many joints affect the vertex, what joints affect the vertex,
				// and how each one of the joints affects the vertex.
				xml_node VertexWeights = {};
				NodeGet(&Controller, &VertexWeights, "vertex_weights");

				u32 JointInfoCount = U32FromAttributeValue(&VertexWeights);
				Assert(JointInfoCount != 0);
				Assert(JointInfoCount == (PositionCount/3));
				u32 *JointCountArray = PushArray(Arena, JointInfoCount, u32);
				joint_info *JointInfoArray = PushArray(Arena, JointInfoCount, joint_info);


				// This array is an array of u32s each of which tells how many joints affects the vertex
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
						for(u32 JointIndex = 0; JointIndex < JointInfo->Count; ++JointIndex)
						{
							JointInfo->Weights[JointIndex] = (JointInfo->Weights[JointIndex] / Sum);
						}
					}
				}

				for(u32 VertexIndex = 0; VertexIndex < Mesh.VertexCount; ++VertexIndex)
				{
					vertex *V = Mesh.Vertices + VertexIndex;
					u32 IndexP = Indices[AttributeCount * VertexIndex];
					V->JointInfo = JointInfoArray[IndexP];
				}
			}
		}

#if 1
		if((LibVisScenes.ChildrenCount != 0) && (LibControllers.ChildrenCount != 0))
		{
			Mesh.Joints = PushArray(Arena, Mesh.JointCount, joint);

			xml_node FirstInstanceController = {};
			NodeGet(&LibVisScenes, &FirstInstanceController, "instance_controller");
			xml_node *ParentOfParent = FirstInstanceController.Parent->Parent;

			u32 InstanceControllersVisited = 0;
			xml_node SkeletonParentNode = {};
			for(s32 k = 0; k < ParentOfParent->ChildrenCount; ++k)
			{
				xml_node *Node = ParentOfParent->Children[k];
				if(Node->ChildrenCount != 0)
				{

					for(s32 ChildIndex = 0; ChildIndex < Node->ChildrenCount; ++ChildIndex)
					{
						xml_node *Child = Node->Children[ChildIndex];
						if(StringsAreSame(Child->Tag, "instance_controller"))
						{
							if(InstanceControllersVisited == MeshIndex)
							{
								SkeletonParentNode = *Node;
								break;
							}
							else
							{
								InstanceControllersVisited++;
							}
						}
					}
				}

				if(SkeletonParentNode.Tag.Size != 0)
				{
					break;
				}
			}

			xml_node Skeleton = {};
			NodeGet(&SkeletonParentNode, &Skeleton, "skeleton");

			string JointRootName = Skeleton.InnerText;
			JointRootName.Data++;
			JointRootName.Size--;

			xml_node SkeletonRoot = {};
			NodeGet(&LibVisScenes, &SkeletonRoot, "node", CString(JointRootName));

			if(SkeletonRoot.ChildrenCount != 0)
			{
				joint *Joints = Mesh.Joints;
				Joints->Name = Mesh.JointNames[0];
				Joints->ParentIndex = -1;

				// TODO(Justin): Remove overload
				ParseF32Array(Arena, &Joints->Transform.E[0][0], 16, SkeletonRoot.Children[0]->InnerText);

				u32 JointIndex = 1;
				JointsGet(Arena, &SkeletonRoot, Mesh.JointNames, Mesh.JointCount, Joints, &JointIndex);
			}

			//
			// NOTE(Justin): Unfortunately it looks as if the material info for the mesh starts here in
			// the instance_material node.
			//

			xml_node InstanceMaterialNode = {};
			NodeGet(&SkeletonParentNode, &InstanceMaterialNode, "instance_material");
			string InstanceMaterialNodeName = NodeAttributeValueGet(&InstanceMaterialNode, "target");
			Assert(InstanceMaterialNodeName.Size != 0);
			InstanceMaterialNodeName.Data++;
			InstanceMaterialNodeName.Size--;

			xml_node MaterialNode = {};
			NodeGet(&LibMaterials, &MaterialNode, "material", CString(InstanceMaterialNodeName));
			xml_node *InstanceEffectNode = MaterialNode.Children[0];
			string InstanceEffectURL = NodeAttributeValueGet(InstanceEffectNode, "url");
			Assert(InstanceEffectURL.Size != 0);
			InstanceEffectURL.Data++;
			InstanceEffectURL.Size--;

			xml_node EffectNode = {};
			NodeGet(&LibEffects, &EffectNode, "effect", CString(InstanceEffectURL));

			if((LibImages.ChildrenCount == 0) &&
			   (LibMaterials.ChildrenCount != 0))
			{
				xml_node Phong = {};
				NodeGet(&EffectNode, &Phong, "phong");
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
			}

			if((LibImages.ChildrenCount != 0) &&
			   (LibMaterials.ChildrenCount != 0))
			{
				xml_node Node = EffectNode;
				GetTextures(Arena, &LibImages, &EffectNode, &Model, &Mesh);
			}
		}
#endif

		Model.Meshes[MeshIndex] = Mesh;
	}

	if((LibVisScenes.ChildrenCount != 0) && (LibControllers.ChildrenCount != 0))
	{
		Model.HasSkeleton = true;
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

	for(u32 MeshIndex = 0; MeshIndex < Model.MeshCount; ++MeshIndex)
	{
		mesh *Mesh = Model.Meshes + MeshIndex;
		for(u32 VertexIndex = 0; VertexIndex < Mesh->VertexCount; ++VertexIndex)
		{
			vertex *Vertex = Mesh->Vertices + VertexIndex;
			Assert(Vertex->JointInfo.Count <= 4);
		}
	}




	return(Model);
}
#endif

inline b32
FlagIsSet(mesh *Mesh, u32 Flag)
{
	b32 Result = ((Mesh->MaterialFlags & Flag) != 0);
	return(Result);
}
