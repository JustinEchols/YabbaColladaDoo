9/28/2024

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
9/28/2024


#if 0
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
				string Header = {};
				string Count = {};

				u8 Delimeters[] = "\n";
				u8 LineDelimeters[] = " \n\r";

				string_list Lines = StringSplit(Arena, Data, Delimeters, 1);
				string_node *Line = Lines.First;

				string_list LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
				string_node *Node = LineData.First;
				Header = Node->String;
				Header.Data[Header.Size] = 0;
				Assert(StringsAreSame(Header, "JOINTS:"));

				Node = Node->Next;
				Count = Node->String;
				Count.Data[Count.Size] = 0;

				Info.JointCount = U32FromASCII(Count);

				Line = Line->Next;
				LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
				Node = LineData.First;
				Header = Node->String;
				Header.Data[Header.Size] = 0;
				Assert(StringsAreSame(Header, "TIMES:"));

				Node = Node->Next;
				Count = Node->String;
				Count.Data[Count.Size] = 0;

				Info.TimeCount = U32FromASCII(Count);

				Line = Line->Next;
				LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
				Node = LineData.First;
				Header = Node->String;
				Header.Data[Header.Size] = 0;
				Assert(StringsAreSame(Header, "TRANSFORMS:"));

				Node = Node->Next;
				Count = Node->String;
				Count.Data[Count.Size] = 0;

				Info.TransformCount = U32FromASCII(Count);
				Assert(Info.TimeCount == Info.TransformCount);

				Line = Line->Next;
				Assert(Line->String.Data[0] == '*');

				Info.JointNames = PushArray(Arena, Info.JointCount, string);
				Info.Times = PushArray(Arena, Info.JointCount, f32 *);
				Info.Transforms = PushArray(Arena, Info.JointCount, mat4 *);

				for(u32 JointIndex = 0; JointIndex < Info.JointCount; ++JointIndex)
				{
					Info.Times[JointIndex] = PushArray(Arena, Info.TimeCount, f32);
					Info.Transforms[JointIndex] = PushArray(Arena, Info.TimeCount, mat4);

					Line = Line->Next;
					string JointName = Line->String;
					JointName.Data[JointName.Size] = 0;
					Info.JointNames[JointIndex] = JointName;

					Line = Line->Next;
					string_array StrTimes = StringSplitIntoArray(Arena, Line->String, LineDelimeters, 3);

					Line = Line->Next;
					string_array StrTransforms = StringSplitIntoArray(Arena, Line->String, LineDelimeters, 3);

					for(u32 TimeIndex = 0; TimeIndex < Info.TimeCount; ++TimeIndex)
					{
						string Time = StrTimes.Strings[TimeIndex];
						Info.Times[JointIndex][TimeIndex] = F32FromASCII(Time);
					}

					for(u32 MatrixIndex = 0; MatrixIndex < Info.TransformCount; ++MatrixIndex)
					{
						mat4 *M = &Info.Transforms[JointIndex][MatrixIndex];
						f32 *Float = &M->E[0][0];
						for(u32 FloatIndex = 0; FloatIndex < 16; ++FloatIndex)
						{
							string StrFloat = StrTransforms.Strings[16 * MatrixIndex + FloatIndex];
							Float[FloatIndex] = F32FromASCII(StrFloat);
						}
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
#endif
