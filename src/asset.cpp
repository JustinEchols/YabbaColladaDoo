
#if 1
internal void 
ConvertMeshFormat(memory_arena *Arena, char *OutputFileName, char *FileName)
{
	FILE *Out = fopen(OutputFileName, "wb");
	if(!Out)
	{
		printf("ERROR: Could not open file %s for writing\n", OutputFileName);
		return;
	}

	loaded_dae DAEFile = ColladaFileLoad(Arena, FileName);
	model Model = ModelInitFromCollada(Arena, DAEFile);

	asset_model_header Header = {};
	Header.MagicNumber = MODEL_FILE_MAGIC_NUMBER;
	Header.Version = MODEL_FILE_VERSION;
	Header.HasSkeleton = Model.HasSkeleton;
	Header.MeshCount = Model.MeshCount;
	Header.OffsetToMeshInfo = sizeof(Header);

	fwrite(&Header, sizeof(Header), 1, Out);

	asset_manager Manager = {};
	Manager.AssetCount = Header.MeshCount;

	u32 HeaderSize = sizeof(Header);
	u32 TotalSize = HeaderSize + (Manager.AssetCount * sizeof(asset_mesh_info));

	// NOTE(Justin): Skip past the header block and asset array block.
	// Then write out the vertices, joints, and xforms. Also populate the
	// asset array (mesh info) data to then be written out.

	fseek(Out, TotalSize, SEEK_CUR);
	for(u32 MeshIndex = 0; MeshIndex < Model.MeshCount; ++MeshIndex)
	{
		mesh *Mesh = Model.Meshes + MeshIndex;
		asset *Asset = Manager.Assets + MeshIndex;
		asset_mesh_info *MeshInfo = &Asset->MeshInfo;

		MemoryCopy(Mesh->Name.Size, Mesh->Name.Data, MeshInfo->Name);
		MeshInfo->IndicesCount = Mesh->IndicesCount;
		MeshInfo->VertexCount = Mesh->VertexCount;
		MeshInfo->JointCount = Mesh->JointCount;
		MeshInfo->MaterialSpec = Mesh->MaterialSpec;
		MeshInfo->XFormBind = Mesh->BindTransform;

		MeshInfo->OffsetToIndices = ftell(Out);
		for(u32 Index = 0; Index < Mesh->IndicesCount; ++Index)
		{
			fwrite(&Index, sizeof(u32), 1, Out);
		}

		MeshInfo->OffsetToVertices = ftell(Out);
		for(u32 VertexIndex = 0; VertexIndex < Mesh->VertexCount; ++VertexIndex)
		{
			vertex *V = Mesh->Vertices + VertexIndex;
			fwrite(&V->P, sizeof(v3), 1, Out);
			fwrite(&V->N, sizeof(v3), 1, Out);
			fwrite(&V->UV, sizeof(v2), 1, Out);
			fwrite(&V->JointInfo.Count, sizeof(u32), 1, Out);
			fwrite(&V->JointInfo.JointIndex, sizeof(u32), 4, Out);
			fwrite(&V->JointInfo.Weights, sizeof(f32), 4, Out);
		}

		// TODO(Justin): Handle the case when a skeleton does not exist.
		MeshInfo->OffsetToJoints = ftell(Out);
		for(u32 JointIndex = 0; JointIndex < Mesh->JointCount; ++JointIndex)
		{
			joint *Joint = Mesh->Joints + JointIndex;

			asset_joint_info JointInfo = {};
			MemoryCopy(Joint->Name.Size, Joint->Name.Data, JointInfo.Name);
			JointInfo.ParentIndex = Joint->ParentIndex;
			JointInfo.Transform = Joint->Transform;

			fwrite(&JointInfo.Name, sizeof(u8), ArrayCount(JointInfo.Name), Out);
			fwrite(&JointInfo.ParentIndex, sizeof(s32), 1, Out);
			fwrite(&JointInfo.Transform, sizeof(mat4), 1, Out);
		}

		MeshInfo->OffsetToInvBindXForms = ftell(Out);
		for(u32 JointIndex = 0; JointIndex < Mesh->JointCount; ++JointIndex)
		{
			mat4 *XForm = Mesh->InvBindTransforms + JointIndex;
			fwrite(XForm, sizeof(mat4), 1, Out);
		}
	}

	// NOTE(Justin): Seek back to the top of the file, after the header. Then write the asset array
	// (mesh info) out.

	fseek(Out, HeaderSize, SEEK_SET);
	for(u32 AssetIndex = 0; AssetIndex < Manager.AssetCount; ++AssetIndex)
	{
		asset *Asset = Manager.Assets + AssetIndex;
		asset_mesh_info *Info = &Asset->MeshInfo;

		fwrite(&Info->Name, sizeof(u8), ArrayCount(Info->Name), Out);
		fwrite(&Info->IndicesCount, sizeof(u32), 1, Out);
		fwrite(&Info->VertexCount, sizeof(u32), 1, Out);
		fwrite(&Info->JointCount, sizeof(u32), 1, Out);
		fwrite(&Info->MaterialSpec, sizeof(material_spec), 1, Out);
		fwrite(&Info->XFormBind, sizeof(mat4), 1, Out);
		fwrite(&Info->OffsetToIndices, sizeof(u64), 1, Out);
		fwrite(&Info->OffsetToVertices, sizeof(u64), 1, Out);
		fwrite(&Info->OffsetToJoints, sizeof(u64), 1, Out);
		fwrite(&Info->OffsetToInvBindXForms, sizeof(u64), 1, Out);
	}

	fclose(Out);

	FILE *Log = fopen("log.txt", "wb");
	if(!Log)
	{
		return;
	}

	char Buffer[256];
	MemoryZero(&Buffer, sizeof(Buffer));
	for(u32 MeshIndex = 0; MeshIndex < Model.MeshCount; ++MeshIndex)
	{
		mesh *Mesh = Model.Meshes + MeshIndex;
		fwrite(CString(Mesh->Name), Mesh->Name.Size * sizeof(u8), 1, Log);
	}

	fclose(Log);
	printf("Log file: %s\n", "log.txt");

}
#endif

internal model 
ModelLoad(memory_arena *Arena, char *FileName)
{
	model Model = {};

	FILE *File = fopen(FileName, "rb");
	if(!File)
	{
		printf("ERROR: Could not open file %s\n", FileName);
		Assert(0);
	}


	s32 Size = FileSizeGet(File);
	if(Size == 0)
	{
		printf("ERROR: File %s has size 0\n", FileName);
		Assert(0);
	}


	u8 *Content = (u8 *)PushSize_(Arena, Size);
	FileReadEntire(Content, Size, File);

	asset_model_header *Header = (asset_model_header *)Content;
	Assert(Header->MagicNumber == MODEL_FILE_MAGIC_NUMBER);
	Assert(Header->Version == MODEL_FILE_VERSION);
	Assert(Header->MeshCount != 0);

	Model.MeshCount = Header->MeshCount;
	Model.HasSkeleton = Header->HasSkeleton;
	Model.Meshes = PushArray(Arena, Model.MeshCount, mesh);

	asset_mesh_info *MeshSource = (asset_mesh_info *)(Content + Header->OffsetToMeshInfo);
	for(u32 MeshIndex = 0; MeshIndex < Model.MeshCount; ++MeshIndex)
	{
		mesh *Mesh = Model.Meshes + MeshIndex;
		Mesh->Name = String(MeshSource->Name);
		Mesh->IndicesCount = MeshSource->IndicesCount;
		Mesh->VertexCount = MeshSource->VertexCount;
		Mesh->JointCount = MeshSource->JointCount;
		Mesh->MaterialSpec = MeshSource->MaterialSpec;
		Mesh->BindTransform = MeshSource->XFormBind;

		Mesh->Indices = (u32 *)(Content + MeshSource->OffsetToIndices);
		Mesh->Vertices = (vertex *)(Content + MeshSource->OffsetToVertices);
		Mesh->InvBindTransforms = (mat4 *)(Content + MeshSource->OffsetToInvBindXForms);

		// TODO(Justin): Figure out best way to handle strings and joints
		// Right now we are copying the joint information which we dont want to do..
		//Mesh->Joints = (joint *)(Content + Source->OffsetToJoints);

		Mesh->Joints = PushArray(Arena, Mesh->JointCount, joint);
		Mesh->JointTransforms = PushArray(Arena, Mesh->JointCount, mat4);
		Mesh->ModelSpaceTransforms = PushArray(Arena, Mesh->JointCount, mat4);
		mat4 I = Mat4Identity();

		asset_joint_info *JointSource = (asset_joint_info *)(Content + MeshSource->OffsetToJoints);
		for(u32 JointIndex = 0; JointIndex < Mesh->JointCount; ++JointIndex)
		{
			joint *Dest = Mesh->Joints + JointIndex;
			Dest->Name = String(JointSource->Name);
			Dest->ParentIndex = JointSource->ParentIndex;
			Dest->Transform = JointSource->Transform;

			Mesh->JointTransforms[JointIndex] = I;
			Mesh->ModelSpaceTransforms[JointIndex] = I;

			JointSource++;
		}

		MeshSource++;
	}

	for(u32 MeshIndex = 0; MeshIndex < Model.MeshCount; ++MeshIndex)
	{
		mesh *Mesh = Model.Meshes + MeshIndex;
		Mesh->Tangents = PushArray(Arena, Mesh->VertexCount, v3);
		Mesh->BiTangents = PushArray(Arena, Mesh->VertexCount, v3);
		for(u32 TriangleIndex = 0; TriangleIndex < Mesh->VertexCount; TriangleIndex += 3)
		{
			vertex V0 = Mesh->Vertices[TriangleIndex + 0];
			vertex V1 = Mesh->Vertices[TriangleIndex + 1];
			vertex V2 = Mesh->Vertices[TriangleIndex + 2];

			v3 E0 = V1.P - V0.P;
			v3 E1 = V2.P - V0.P;

			v2 dUV0 = V1.UV - V0.UV;
			v2 dUV1 = V2.UV - V0.UV;

			f32 C = (1.0f / (dUV0.x * dUV1.y - dUV0.y * dUV1.x));
			v3 Tangent = Normalize(C * (dUV1.y * E0 - dUV0.y * E1));
			v3 BiTangent = Normalize(C * (dUV0.x * E1 - dUV1.x * E0));

			Mesh->Tangents[TriangleIndex + 0] = Tangent;
			Mesh->Tangents[TriangleIndex + 1] = Tangent;
			Mesh->Tangents[TriangleIndex + 2] = Tangent;

			Mesh->BiTangents[TriangleIndex + 0] = BiTangent;
			Mesh->BiTangents[TriangleIndex + 1] = BiTangent;
			Mesh->BiTangents[TriangleIndex + 2] = BiTangent;
		}

	}

	return(Model);
}

internal model 
ModelLoadFromCollada(memory_arena *Arena, char *FileName)
{
	model Model = {};

	loaded_dae DAEFile = ColladaFileLoad(Arena, FileName);
	Model = ModelInitFromCollada(Arena, DAEFile);

	// TODO(Justin): Smoothness
	// Compute tangents/bitangents
	for(u32 MeshIndex = 0; MeshIndex < Model.MeshCount; ++MeshIndex)
	{
		mesh *Mesh = Model.Meshes + MeshIndex;
		Mesh->Tangents = PushArray(Arena, Mesh->VertexCount, v3);
		Mesh->BiTangents = PushArray(Arena, Mesh->VertexCount, v3);
		for(u32 TriangleIndex = 0; TriangleIndex < Mesh->VertexCount; TriangleIndex += 3)
		{
			vertex V0 = Mesh->Vertices[TriangleIndex + 0];
			vertex V1 = Mesh->Vertices[TriangleIndex + 1];
			vertex V2 = Mesh->Vertices[TriangleIndex + 2];

			v3 E0 = V1.P - V0.P;
			v3 E1 = V2.P - V0.P;

			v2 dUV0 = V1.UV - V0.UV;
			v2 dUV1 = V2.UV - V0.UV;

			f32 C = (1.0f / (dUV0.x * dUV1.y - dUV0.y * dUV1.x));
			v3 Tangent = Normalize(C * (dUV1.y * E0 - dUV0.y * E1));
			v3 BiTangent = Normalize(C * (dUV0.x * E1 - dUV1.x * E0));

			Mesh->Tangents[TriangleIndex + 0] = Tangent;
			Mesh->Tangents[TriangleIndex + 1] = Tangent;
			Mesh->Tangents[TriangleIndex + 2] = Tangent;

			Mesh->BiTangents[TriangleIndex + 0] = BiTangent;
			Mesh->BiTangents[TriangleIndex + 1] = BiTangent;
			Mesh->BiTangents[TriangleIndex + 2] = BiTangent;
		}
	}
	
	return(Model);
}

internal void
ConvertAnimationFormat(memory_arena *Arena, char *OutputFileName, char *DaeFileName)
{
	FILE *Out = fopen(OutputFileName, "wb");
	if(Out)
	{
		animation_info Info = AnimationInitFromCollada(Arena, DaeFileName);

		asset_animation_header Header = {};

		Header.MagicNumber = ANIMATION_FILE_MAGIC_NUMBER;
		Header.Version = ANIMATION_FILE_VERSION;
		Header.JointCount = Info.JointCount;
		Header.KeyFrameCount = Info.KeyFrameCount;
		Header.TimeCount = Info.TimeCount;
		Header.KeyFrameIndex = Info.KeyFrameIndex;
		Header.CurrentTime = Info.CurrentTime;
		Header.Duration = Info.Duration;
		Header.FrameRate = Info.FrameRate;

		u32 TimesTotalSize = Info.TimeCount * sizeof(f32);
		u32 NamesTotalSize = Info.JointCount * sizeof(asset_joint_name);

		Header.OffsetToTimes = sizeof(Header);
		Header.OffsetToNames = Header.OffsetToTimes + TimesTotalSize;
		Header.OffsetToAnimationInfo = Header.OffsetToNames + NamesTotalSize;

		fwrite(&Header, sizeof(Header), 1, Out);

		for(u32 TimeIndex = 0; TimeIndex < Info.TimeCount; ++TimeIndex)
		{
			fwrite(&Info.Times[TimeIndex], sizeof(f32), 1, Out);
		}

		for(u32 NameIndex = 0; NameIndex < Info.JointCount; ++NameIndex)
		{
			string *Source = Info.JointNames + NameIndex;
			asset_joint_name Dest = {};

			MemoryCopy(Source->Size, Source->Data, Dest.Name);
			fwrite(&Dest.Name, sizeof(u8), ArrayCount(Dest.Name), Out);
		}

		asset_manager Manager = {};
		Manager.AssetCount = Header.KeyFrameCount;

		u32 OffsetToAssets = sizeof(Header) + TimesTotalSize + NamesTotalSize;
		u32 AssetsTotalSize = Manager.AssetCount * sizeof(asset_animation_info);
		u32 OffsetToData = OffsetToAssets + AssetsTotalSize;

		fseek(Out, OffsetToData, SEEK_CUR);
		for(u32 KeyFrameIndex = 0; KeyFrameIndex < Info.KeyFrameCount; ++KeyFrameIndex)
		{
			key_frame *KeyFrame = Info.KeyFrames + KeyFrameIndex; 
			asset *Asset = Manager.Assets + KeyFrameIndex;
			asset_animation_info *AnimationInfo = &Asset->AnimationInfo;

			AnimationInfo->OffsetToPositions = ftell(Out);
			for(u32 JointIndex = 0; JointIndex < Info.JointCount; ++JointIndex)
			{
				v3 *P = KeyFrame->Positions + JointIndex;
				fwrite(&P->x, sizeof(f32), 1, Out);
				fwrite(&P->y, sizeof(f32), 1, Out);
				fwrite(&P->z, sizeof(f32), 1, Out);
			}

			AnimationInfo->OffsetToQuaternions = ftell(Out);
			for(u32 JointIndex = 0; JointIndex < Info.JointCount; ++JointIndex)
			{
				quaternion *Q = KeyFrame->Quaternions + JointIndex;
				fwrite(&Q->x, sizeof(f32), 1, Out);
				fwrite(&Q->y, sizeof(f32), 1, Out);
				fwrite(&Q->z, sizeof(f32), 1, Out);
				fwrite(&Q->w, sizeof(f32), 1, Out);
			}

			AnimationInfo->OffsetToScales = ftell(Out);
			for(u32 JointIndex = 0; JointIndex < Info.JointCount; ++JointIndex)
			{
				v3 *S = KeyFrame->Scales + JointIndex;
				fwrite(&S->x, sizeof(f32), 1, Out);
				fwrite(&S->y, sizeof(f32), 1, Out);
				fwrite(&S->z, sizeof(f32), 1, Out);
			}
		}

		fseek(Out, OffsetToAssets, SEEK_SET);
		for(u32 AssetIndex = 0; AssetIndex < Manager.AssetCount; ++AssetIndex)
		{
			asset *Asset = Manager.Assets + AssetIndex;
			asset_animation_info *AInfo = &Asset->AnimationInfo;
			fwrite(&AInfo->OffsetToPositions, sizeof(u64), 1, Out);
			fwrite(&AInfo->OffsetToQuaternions, sizeof(u64), 1, Out);
			fwrite(&AInfo->OffsetToScales, sizeof(u64), 1, Out);
		}

		fclose(Out);
	}
}

internal animation_info 
AnimationLoad(memory_arena *Arena, char *FileName)
{
	animation_info Info = {};

	FILE *File = fopen(FileName, "rb");
	if(!File)
	{
		printf("Error opening file: %s", FileName);
		return(Info);
	}

	s32 Size = FileSizeGet(File);
	if(Size == 0)
	{
		printf("Error opening file: %s", FileName);
		return(Info);
	}

	u8 *Content = (u8 *)PushSize_(Arena, Size);
	FileReadEntire(Content, Size, File);

	asset_animation_header *Header = (asset_animation_header *)Content;
	Assert(Header->MagicNumber == ANIMATION_FILE_MAGIC_NUMBER);
	Assert(Header->Version == ANIMATION_FILE_VERSION);
	Assert(Header->JointCount != 0);

	Info.JointCount = Header->JointCount;
	Info.KeyFrameCount = Header->KeyFrameCount;
	Info.TimeCount = Header->TimeCount;
	Info.KeyFrameIndex = Header->KeyFrameIndex;
	Info.CurrentTime = Header->CurrentTime;
	Info.Duration = Header->Duration;
	Info.FrameRate = Header->FrameRate;

	Info.Times = (f32 *)(Content + Header->OffsetToTimes);

	// TODO(Justin): Should not have to copy the data!!
	//Info.JointNames = (string *)(Content + Header->OffsetToNames);
	Info.JointNames = PushArray(Arena, Info.JointCount, string);
	asset_joint_name *JointNameSource = (asset_joint_name *)(Content + Header->OffsetToNames);
	for(u32 JointIndex = 0; JointIndex < Info.JointCount; ++JointIndex)
	{
		Info.JointNames[JointIndex] = String(JointNameSource->Name);
		JointNameSource++;
	}

	Info.KeyFrames = PushArray(Arena, Info.TimeCount, key_frame);
	asset_animation_info *AnimationSource = (asset_animation_info *)(Content + Header->OffsetToAnimationInfo);
	for(u32 KeyFrameIndex = 0; KeyFrameIndex < Info.KeyFrameCount; ++KeyFrameIndex)
	{
		key_frame *KeyFrame = Info.KeyFrames + KeyFrameIndex;
		KeyFrame->Positions = (v3 *)(Content + AnimationSource->OffsetToPositions);
		KeyFrame->Quaternions = (quaternion *)(Content + AnimationSource->OffsetToQuaternions);
		KeyFrame->Scales = (v3 *)(Content + AnimationSource->OffsetToScales);
		AnimationSource++;
	}

	return(Info);
}
