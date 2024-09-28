
internal texture
TextureLoad(char *FileName)
{
	texture Texture = {};
	Texture.Memory = stbi_load(FileName, &Texture.Width, &Texture.Height, &Texture.ChannelCount, 4);
	return(Texture);
}

enum animation_header
{
	AnimationHeader_Invalid,
	AnimationHeader_Joints,
	AnimationHeader_Times,
	AnimationHeader_KeyFrame,
};

inline animation_header
AnimationHeaderGet(u8 *Buff)
{
	animation_header Result = AnimationHeader_Invalid;
	if(StringsAreSame(Buff, "JOINTS:"))
	{
		Result = AnimationHeader_Joints;
	}
	if(StringsAreSame(Buff, "TIMES:"))
	{
		Result = AnimationHeader_Times;
	}
	if(StringsAreSame(Buff, "KEY_FRAME:"))
	{
		Result = AnimationHeader_KeyFrame;
	}

	return(Result);
}

internal animation_info
AnimationInfoLoad(memory_arena *Arena, char *FileName)
{
	animation_info Info = {};
	FILE *File = fopen(FileName, "r");
	if(File)
	{
		s32 Size = FileSizeGet(File);
		if(Size != -1)
		{
			u8 *Content = (u8 *)calloc(Size + 1, sizeof(u8));
			FileReadEntireAndNullTerminate(Content, Size, File);
			if(FileClose(File) && Content)
			{
				string Data = String(Content);
				u8 Delimeters[] = "\r\n";
				string_list Lines = StringSplit(Arena, Data, Delimeters, ArrayCount(Delimeters));

				u8 Buff[512];
				for(string_node *Line = Lines.First; Line && Line->Next; Line = Line->Next)
				{
					if(Info.KeyFrameCount != 0)
					{
						// TODO(Justin): Figure out why we continue to have data
						// to process!!?!
						break;
					}
				
					u8 *C = Line->String.Data;
					BufferNextWord(&C, Buff);
					animation_header Header = AnimationHeaderGet(Buff);
					switch(Header)
					{
						case AnimationHeader_Joints:
						{
							EatSpaces(&C);
							BufferNextWord(&C, Buff);
							Info.JointCount = U32FromASCII(Buff);
							Info.JointNames = PushArray(Arena, Info.JointCount, string);

							for(u32 NameIndex = 0; NameIndex < Info.JointCount; ++NameIndex)
							{
								Line = Line->Next;
								C = Line->String.Data;
								BufferNextWord(&C, Buff);
								Info.JointNames[NameIndex] = StringAllocAndCopy(Arena, Buff);
							}

						} break;

						case AnimationHeader_Times:
						{
							EatSpaces(&C);
							BufferNextWord(&C, Buff);
							Info.TimeCount = U32FromASCII(Buff);
							Info.Times = PushArray(Arena, Info.TimeCount, f32);

							Line = Line->Next;
							C = Line->String.Data;
							for(u32 TimeIndex = 0; TimeIndex < Info.TimeCount; ++TimeIndex)
							{
								BufferNextWord(&C, Buff);
								Info.Times[TimeIndex] = F32FromASCII(Buff);
								EatSpaces(&C);
							}
						} break;

						case AnimationHeader_KeyFrame:
						{
							Info.KeyFrameCount = Info.TimeCount;
							Info.KeyFrames = PushArray(Arena, Info.KeyFrameCount, key_frame);
							for(u32 KeyFrameIndex = 0; KeyFrameIndex < Info.KeyFrameCount; ++KeyFrameIndex)
							{
								////Info.KeyFrames[KeyFrameIndex].Transforms = PushArray(Arena, Info.JointCount, mat4);
								Info.KeyFrames[KeyFrameIndex].Positions = PushArray(Arena, Info.JointCount, v3);
								Info.KeyFrames[KeyFrameIndex].Quaternions = PushArray(Arena, Info.JointCount, quaternion);
								Info.KeyFrames[KeyFrameIndex].Scales = PushArray(Arena, Info.JointCount, v3);
							}

							for(u32 KeyFrameIndex = 0; KeyFrameIndex < Info.KeyFrameCount; ++KeyFrameIndex)
							{
								key_frame *KeyFrame = Info.KeyFrames + KeyFrameIndex;
								for(u32 JointIndex = 0; JointIndex < Info.JointCount; ++JointIndex)
								{
									v3 *P = KeyFrame->Positions + JointIndex;
									quaternion *Q = KeyFrame->Quaternions + JointIndex;
									v3 *Scale = KeyFrame->Scales + JointIndex;

									Line = Line->Next;
									C = Line->String.Data;
									f32 *F32 = (f32 *)P;
									for(u32 FloatIndex = 0; FloatIndex < 3; ++FloatIndex)
									{
										BufferNextWord(&C, Buff);
										F32[FloatIndex] = F32FromASCII(Buff);
										EatSpaces(&C);
									}

									Line = Line->Next;
									C = Line->String.Data;
									F32 = (f32 *)Q;
									for(u32 FloatIndex = 0; FloatIndex < 4; ++FloatIndex)
									{
										BufferNextWord(&C, Buff);
										F32[FloatIndex] = F32FromASCII(Buff);
										EatSpaces(&C);
									}

									Line = Line->Next;
									C = Line->String.Data;
									F32 = (f32 *)Scale;
									for(u32 FloatIndex = 0; FloatIndex < 3; ++FloatIndex)
									{
										BufferNextWord(&C, Buff);
										F32[FloatIndex] = F32FromASCII(Buff);
										EatSpaces(&C);
									}
								}

								Line = Line->Next;
							}
						} break;
					}
				}
			}
			else
			{
				printf("Error file size is %d\n", Size);
			}
		}
	}
	else
	{
		printf("Error could not read %s\n", FileName);
		perror("");
	}

	return(Info);
}

internal void 
ConvertMeshFormat(memory_arena *Arena, char *OutputFileName, char *FileName)
{
	FILE *Out = fopen(OutputFileName, "wb");
	if(Out)
	{
		loaded_dae DAEFile = ColladaFileLoad(Arena, FileName);
		model Model = ModelInitFromCollada(Arena, DAEFile);

		asset_model_header Header = {};
		Header.MagicNumber = MODEL_FILE_MAGIC_NUMBER;
		Header.Version = MODEL_FILE_VERSION;
		Header.HasSkeleton = Model.HasSkeleton;
		Header.MeshCount = Model.MeshCount;

		fwrite(&Header, sizeof(Header), 1, Out);

		asset_manager Manager = {};
		Manager.AssetCount = Header.MeshCount;

		u32 HeaderSize = sizeof(Header);
		u32 TotalSize = HeaderSize + (Manager.AssetCount * sizeof(asset));

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
				fwrite(&V->JointInfo.JointIndex, sizeof(u32), 3, Out);
				fwrite(&V->JointInfo.Weights, sizeof(f32), 3, Out);
			}

			MeshInfo->OffsetToJoints = ftell(Out);
			for(u32 JointIndex = 0; JointIndex < Mesh->JointCount; ++JointIndex)
			{
				joint *Joint = Mesh->Joints + JointIndex;

				asset_joint_info JointInfo = {};
				MemoryCopy(Joint->Name.Size, Joint->Name.Data, JointInfo.Name);
				JointInfo.ParentIndex = Joint->ParentIndex;
				JointInfo.Transform = Joint->Transform;

				//fwrite(&Joint->Name.Data, sizeof(u8), Joint->Name.Size, Out);
				//fwrite(&Joint->Name.Size, sizeof(u64), 1, Out);
				//fwrite(&Joint->ParentIndex, sizeof(s32), 1, Out);
				//fwrite(&Joint->Transform, sizeof(mat4), 1, Out);

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
	}
}

internal model 
ModelLoad(memory_arena *Arena, char *FileName)
{
	model Model = {};

	FILE *File = fopen(FileName, "rb");
	s32 Size = FileSizeGet(File);
	if(Size != 0)
	{
		u8 *Content = (u8 *)PushSize_(Arena, Size);
		FileReadEntire(Content, Size, File);

		asset_model_header *Header = (asset_model_header *)Content;
		Assert(Header->MagicNumber == MODEL_FILE_MAGIC_NUMBER);
		Assert(Header->Version == MODEL_FILE_VERSION);
		Assert(Header->MeshCount != 0);

		Model.MeshCount = Header->MeshCount;
		Model.HasSkeleton = Header->HasSkeleton;

		Model.Meshes = PushArray(Arena, Model.MeshCount, mesh);

		asset_mesh_info *MeshSource = (asset_mesh_info *)(Content + sizeof(asset_model_header));
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
			Mesh->JointNames = PushArray(Arena, Mesh->JointCount, string);
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

				Mesh->JointNames[JointIndex] = Dest->Name;
				Mesh->JointTransforms[JointIndex] = I;
				Mesh->ModelSpaceTransforms[JointIndex] = I;

				JointSource++;
			}

			MeshSource++;
		}
	}

	return(Model);
}
