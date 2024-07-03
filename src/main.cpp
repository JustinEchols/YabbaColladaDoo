
/*
 TODO(Justin):
 [] WARNING using String() on a token is bad and can easily result in an access violation.
 [] Remove strtok_s
 [] Use temporary memory when initializing mesh
 [] Instead of storing mat4's for animation, store pos, quat, scale and construct mat4's
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define internal static
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#define ArrayCount(A) sizeof(A) / sizeof((A)[0])
#define Kilobyte(Count) (1024 * Count)
#define Megabyte(Count) (1024 * Kilobyte(Count))

#define PI32 3.1415926535897f
#define DegreeToRad(Degrees) ((Degrees) * (PI32 / 180.0f))

#define COLLADA_ATTRIBUTE_MAX_COUNT 10
#define COLLADA_NODE_CHILDREN_MAX_COUNT 75

#define PushArray(Arena, Count, Type) (Type *)PushSize_(Arena, Count * sizeof(Type))
#define PushStruct(Arena, Type) (Type *)PushSize_(Arena, sizeof(Type))
#define ArrayCopy(Count, Src, Dest) MemoryCopy((Count)*sizeof(*(Src)), (Src), (Dest))

#define SLLQueuePush_N(First,Last,Node,Next) (((First)==0?\
(First)=(Last)=(Node):\
((Last)->Next=(Node),(Last)=(Node))),\
(Node)->Next=0)
#define SLLQueuePush(First,Last,Node) SLLQueuePush_N(First,Last,Node,Next)

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef s32 b32;
typedef float f32;
typedef size_t memory_index;

//
// NOTE(Justin): Arena allocator
//

struct memory_arena
{
	u8 *Base;
	memory_index Size;
	memory_index Used;
};

internal void
ArenaInitialize(memory_arena *Arena, u8 *Base, memory_index Size)
{
	Arena->Base = Base;
	Arena->Size = Size;
	Arena->Used = 0;
}

internal void *
PushSize_(memory_arena *Arena, memory_index Size)
{
	Assert((Arena->Used + Size) <= Arena->Size);

	void *Result = Arena->Base + Arena->Used;
	Arena->Used += Size;

	return(Result);
}

internal void *
MemoryCopy(memory_index Size, void *SrcInit, void *DestInit)
{
	u8 *Src = (u8 *)SrcInit;
	u8 *Dest = (u8 *)DestInit;
	while(Size--) {*Dest++ = *Src++;}

	return(DestInit);
}

internal void
MemoryZero(memory_index Size, void *Src)
{
	u8 *P = (u8 *)Src;
	while(Size--)
	{
		*P++ = 0;
	}
}

internal u32
U32ArraySum(u32 *A, u32 Count)
{
	u32 Result = 0;
	for(u32 Index = 0; Index < Count; ++Index)
	{
		Result += A[Index];
	}
	return(Result);
}

#include "strings.h"
#include "strings.cpp"
#include "math_util.h"
#include "xml.h"
#include "xml.cpp"
#include "mesh.h"
#include "mesh.cpp"

internal s32
FileSizeGet(FILE *OpenFile)
{
	Assert(OpenFile);
	s32 Result = -1;
	fseek(OpenFile, 0, SEEK_END);
	Result = ftell(OpenFile);
	fseek(OpenFile, 0, SEEK_SET);

	return(Result);
}

internal void
FileReadEntireAndNullTerminate(u8 *Dest, s32 Size, FILE *OpenFile)
{
	fread(Dest, 1, (size_t)Size, OpenFile);
	Dest[Size] = '\0';
}

internal b32 
FileClose(FILE *OpenFile)
{
	b32 Result = (fclose(OpenFile) == 0);
	return(Result);
}

internal loaded_dae
ColladaFileLoad(memory_arena *Arena, char *FileName)
{
	loaded_dae Result = {};

	FILE *FileHandle = fopen(FileName, "r");
	if(FileHandle)
	{
		s32 Size = FileSizeGet(FileHandle);
		if(Size != -1)
		{
			u8 *Content = (u8 *)calloc(Size + 1, sizeof(u8));
			FileReadEntireAndNullTerminate(Content, Size, FileHandle);
			if(FileClose(FileHandle))
			{
				Result.FullPath = StringAllocAndCopy(Arena, FileName);

				//
				// NOTE(Justin): Skip to the <COLLADA> node.
				//

				char Buffer[512];
				s32 InnerTextIndex = 0;
				s32 Index = 0;
				while(!SubStringExists(Buffer, "COLLADA"))
				{	
					Buffer[InnerTextIndex++] = Content[Index++];
					Buffer[InnerTextIndex] = '\0';
				}

				InnerTextIndex = 0;
				Index -= ((s32)strlen("<COLLADA") + 1);

				u32 DelimeterCount = 2;
				char TagDelimeters[] = "<>";
				char InnerTagDelimeters[] = "=\"";

				string Data = String((u8 *)(Content + Index));
				string_list List = StringSplit(Arena, Data, (u8 *)TagDelimeters, DelimeterCount);

				string_node *Token = List.First;
				Token = Token->Next;

				Result.Root = PushXMLNode(Arena, 0);
				xml_node *CurrentNode = Result.Root;

				CurrentNode->Tag = StringAllocAndCopy(Arena, Token->String);
				Token = Token->Next;
				CurrentNode->InnerText = StringAllocAndCopy(Arena, Token->String);

				while(Token->Next)
				{
					Token = Token->Next;
					if(Token->String.Data[0] == '/')
					{
						if(CurrentNode->Parent)
						{
							CurrentNode = CurrentNode->Parent;
							Token = Token->Next;
						}
						else
						{
							break;
						}
					}
					else
					{
						CurrentNode = ChildNodeAdd(Arena, CurrentNode);

						if(NodeHasKeysValues(Token->String) && StringEndsWith(Token->String, '/'))
						{
							string AtSpace = StringSearchFor(Token->String, ' ');
							u64 CopySize = (Token->String.Size - AtSpace.Size);
							string ToCopy = StringFromRange(Token->String.Data, Token->String.Data + CopySize);
							CurrentNode->Tag = StringAllocAndCopy(Arena, ToCopy);

							string Temp = AtSpace;
							Temp.Data++;
							Temp.Size--;

							NodeProcessKeysValues(Arena, CurrentNode, Temp, InnerTagDelimeters, DelimeterCount);

							Token = Token->Next;
							CurrentNode->InnerText = StringAllocAndCopy(Arena, Token->String);
							CurrentNode = CurrentNode->Parent;
						}
						else if(NodeHasKeysValues(Token->String))
						{
							string AtSpace = StringSearchFor(Token->String, ' ');
							u64 CopySize = (Token->String.Size - AtSpace.Size);
							string ToCopy = StringFromRange(Token->String.Data, Token->String.Data + CopySize);
							CurrentNode->Tag = StringAllocAndCopy(Arena, ToCopy);

							string Temp = AtSpace;
							Temp.Data++;
							Temp.Size--;

							NodeProcessKeysValues(Arena, CurrentNode, Temp, InnerTagDelimeters, DelimeterCount);

							Token = Token->Next;
							CurrentNode->InnerText = StringAllocAndCopy(Arena, Token->String);
						}
						else if(StringEndsWith(Token->String, '/'))
						{
							CurrentNode->Tag = StringAllocAndCopy(Arena, Token->String);
							Token = Token->Next;
							CurrentNode->InnerText = StringAllocAndCopy(Arena, Token->String);
							CurrentNode = CurrentNode->Parent;
						}
						else
						{
							CurrentNode->Tag = StringAllocAndCopy(Arena, Token->String);
							Token = Token->Next;
							CurrentNode->InnerText = StringAllocAndCopy(Arena, Token->String);
						}
					}
				}
			}
		}
		else
		{
			printf("Error file %s has size %d", FileName, Size);
		}
	}
	else
	{
		printf("Error opening file %s", FileName);
		perror("");
	}

	return(Result);
}

internal u32
MeshHeaderNumberGet(memory_arena *Arena, string_node *Line, u8 *LineDelimeters, u32 DelimCount)
{
	u32 Result = 0;

	string_list LineData = StringSplit(Arena, Line->String, LineDelimeters, DelimCount);
	string_node *Node = LineData.First;
	Node = Node->Next;
	string Value = Node->String;

	Result = U32FromASCII(Value.Data);

	return(Result);
}

internal void
ParseMeshU32Array(memory_arena *Arena, u32 **U32, u32 *Count, string_node *Line, u8 *Delimeters, u32 DelimCount,
		char *HeaderName)
{
	string_list LineData = StringSplit(Arena, Line->String, Delimeters, DelimCount);
	string_node *Node = LineData.First;
	string Header = Node->String;
	Header.Data[Header.Size] = 0;
	Assert(StringsAreSame(Header, HeaderName));
	Node = Node->Next;
	string StrCount = Node->String;
	StrCount.Data[StrCount.Size] = 0;

	*Count  = U32FromASCII(StrCount.Data);
	*U32 = PushArray(Arena, *Count, u32);

	Line = Line->Next;
	LineData = StringSplit(Arena, Line->String, Delimeters, DelimCount);
	Assert(LineData.Count == *Count);
	Node = LineData.First;

	u32 *A = *U32;
	for(u32 Index = 0; Index < *Count; ++Index)
	{
		string Value = Node->String;
		Value.Data[Value.Size] = 0;
		A[Index] = U32FromASCII(Value);
		Node = Node->Next;
	}
}

internal void
ParseMeshF32Array(memory_arena *Arena, f32 **F32, u32 *Count, string_node *Line, u8 *Delimeters, u32 DelimCount,
		char *HeaderName)
{
	string_list LineData = StringSplit(Arena, Line->String, Delimeters, DelimCount);
	string_node *Node = LineData.First;
	string Header = Node->String;
	Header.Data[Header.Size] = 0;
	Assert(StringsAreSame(Header, HeaderName));
	Node = Node->Next;
	string StrCount = Node->String;
	StrCount.Data[StrCount.Size] = 0;

	*Count  = U32FromASCII(StrCount.Data);
	*F32 = PushArray(Arena, *Count, f32);

	Line = Line->Next;
	LineData = StringSplit(Arena, Line->String, Delimeters, DelimCount);
	Assert(LineData.Count == *Count);
	Node = LineData.First;

	f32 *A = *F32;
	for(u32 Index = 0; Index < *Count; ++Index)
	{
		string Value = Node->String;
		Value.Data[Value.Size] = 0;
		A[Index] = F32FromASCII(Value);
		Node = Node->Next;
	}
}

internal b32
DoneProcessingMesh(string_node *Line)
{
	b32 Result = (Line->String.Data[0] == '*');
	return(Result);
}

internal model
ModelLoad(memory_arena *Arena, char *FileName)
{
	model Model = {};

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

				u8 Delimeters[] = "\n";
				u8 LineDelimeters[] = " \n\r";
				string Count = {};

				string_list Lines = StringSplit(Arena, Data, Delimeters, 1);
				string_node *Line = Lines.First;

				Model.MeshCount = MeshHeaderNumberGet(Arena, Line, LineDelimeters, 3);
				Model.Meshes = PushArray(Arena, Model.MeshCount, mesh);

				for(u32 MeshIndex = 0; MeshIndex < Model.MeshCount; ++MeshIndex)
				{
					mesh *Mesh = Model.Meshes + MeshIndex;

					Line = Line->Next;
					string_list LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
					string_node *Node = LineData.First;
					string Header = Node->String;
					Header.Data[Header.Size] = 0;
					Assert(StringsAreSame(Header, "MESH:"));

					Node = Node->Next;
					string MeshName = Node->String;
					MeshName.Data[MeshName.Size] = 0;
					Mesh->Name = MeshName;

					Line = Line->Next;
					LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
					Node = LineData.First;
					Header = Node->String;
					Header.Data[Header.Size] = 0;
					Assert(StringsAreSame(Header, "LIGHTING:"));

					Line = Line->Next;
					LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
					Assert(LineData.Count == 4);
					Node = LineData.First;

					for(u32 Index = 0; Index < 4; ++Index)
					{
						string Float = Node->String;
						Float.Data[Float.Size] = 0;
						Mesh->MaterialSpec.Ambient.E[Index] = F32FromASCII(Float);
						Node = Node->Next;
					}

					Line = Line->Next;
					LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
					Assert(LineData.Count == 4);
					Node = LineData.First;

					for(u32 Index = 0; Index < 4; ++Index)
					{
						string Float = Node->String;
						Float.Data[Float.Size] = 0;
						Mesh->MaterialSpec.Diffuse.E[Index] = F32FromASCII(Float);
						Node = Node->Next;
					}

					Line = Line->Next;
					LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
					Assert(LineData.Count == 4);
					Node = LineData.First;

					for(u32 Index = 0; Index < 4; ++Index)
					{
						string Float = Node->String;
						Float.Data[Float.Size] = 0;
						Mesh->MaterialSpec.Specular.E[Index] = F32FromASCII(Float);
						Node = Node->Next;
					}

					Line = Line->Next;
					LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
					Assert(LineData.Count == 1);
					Node = LineData.First;
					string Float = Node->String;
					Float.Data[Float.Size] = 0;
					Mesh->MaterialSpec.Shininess = F32FromASCII(Float);

					Line = Line->Next;
					ParseMeshU32Array(Arena, &Mesh->Indices, &Mesh->IndicesCount, Line, LineDelimeters, 3, "INDICES:");

					Line = Line->Next;

					Line = Line->Next;
					Line = Line->Next;
					ParseMeshF32Array(Arena, &Mesh->Positions, &Mesh->PositionsCount, Line, LineDelimeters, 3, "POSITIONS:");

					Line = Line->Next;
					Line = Line->Next;
					ParseMeshF32Array(Arena, &Mesh->Normals, &Mesh->NormalsCount, Line, LineDelimeters, 3, "NORMALS:");

					Line = Line->Next;
					Line = Line->Next;
					ParseMeshF32Array(Arena, &Mesh->UV, &Mesh->UVCount, Line, LineDelimeters, 3, "UVS:");

					Line = Line->Next;
					Line = Line->Next;
					LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
					Node = LineData.First;
					Header = Node->String;
					Header.Data[Header.Size] = 0;
					Assert(StringsAreSame(Header, "JOINT_INFO:"));

					Node = Node->Next;
					Count = Node->String;
					Count.Data[Count.Size] = 0;
					
					Mesh->JointInfoCount = U32FromASCII(Count);
					Mesh->JointsInfo = PushArray(Arena, Mesh->JointInfoCount, joint_info);
					for(u32 Index = 0; Index < Mesh->JointInfoCount; ++Index)
					{
						joint_info *Info = Mesh->JointsInfo + Index;

						Line = Line->Next;

						Count = Line->String;
						Count.Data[Count.Size] = 0;
						
						Info->Count = U32FromASCII(Count);

						Line = Line->Next;
						string_list IndicesList = StringSplit(Arena, Line->String, LineDelimeters, 3);

						Node = IndicesList.First;
						string StrIndex = Node->String;
						StrIndex.Data[StrIndex.Size] = 0;
						Info->JointIndex[0] = U32FromASCII(StrIndex);

						Node = Node->Next;
						StrIndex = Node->String;
						StrIndex.Data[StrIndex.Size] = 0;
						Info->JointIndex[1] = U32FromASCII(StrIndex);

						Node = Node->Next;
						StrIndex = Node->String;
						StrIndex.Data[StrIndex.Size] = 0;
						Info->JointIndex[2] = U32FromASCII(StrIndex);

						Line = Line->Next;
						string_list WeightsList = StringSplit(Arena, Line->String, LineDelimeters, 3);

						Node = WeightsList.First;
						string Weight = Node->String;
						Weight.Data[Weight.Size] = 0;
						Info->Weights[0] = F32FromASCII(Weight);

						Node = Node->Next;
						Weight = Node->String;
						Weight.Data[Weight.Size] = 0;
						Info->Weights[1] = F32FromASCII(Weight);

						Node = Node->Next;
						Weight = Node->String;
						Weight.Data[Weight.Size] = 0;
						Info->Weights[2] = F32FromASCII(Weight);
					}

					Line = Line->Next;
					Line = Line->Next;
					LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
					Node = LineData.First;
					Header = Node->String;
					Header.Data[Header.Size] = 0;
					Assert(StringsAreSame(Header, "JOINTS:"));

					Node = Node->Next;
					Count = Node->String;
					Count.Data[Count.Size] = 0;

					Mesh->JointCount = U32FromASCII(Count);
					Mesh->Joints = PushArray(Arena, Mesh->JointCount, joint);
					Mesh->JointNames = PushArray(Arena, Mesh->JointCount, string);
					Mesh->JointTransforms = PushArray(Arena, Mesh->JointCount, mat4);
					Mesh->ModelSpaceTransforms = PushArray(Arena, Mesh->JointCount, mat4);

					Line = Line->Next;
					string RootJointName = Line->String;
					RootJointName.Data[RootJointName.Size] = 0;

					Mesh->Joints[0].Name = RootJointName;
					Mesh->Joints[0].ParentIndex = -1;
					
					Line = Line->Next;
					Line = Line->Next;
					LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
					Node = LineData.First;
					f32 *T = &Mesh->Joints[0].Transform.E[0][0];
					for(u32 Index = 0; Index < 16; Index++)
					{
						Float = Node->String;
						Float.Data[Float.Size] = 0;
						T[Index] = F32FromASCII(Float);
						Node = Node->Next;
					}

					for(u32 Index = 1; Index < Mesh->JointCount; ++Index)
					{
						Line = Line->Next;

						joint *Joint = Mesh->Joints + Index;

						string JointName = Line->String;
						JointName.Data[JointName.Size] = 0;
						Joint->Name = JointName;

						Line = Line->Next;
						string ParentIndex = Line->String;
						ParentIndex.Data[ParentIndex.Size] = 0;
						Joint->ParentIndex = U32FromASCII(ParentIndex);

						Line = Line->Next;
						LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
						Node = LineData.First;
						T = &Joint->Transform.E[0][0];
						for(u32 k = 0; k < 16; k++)
						{
							Float = Node->String;
							Float.Data[Float.Size] = 0;
							T[k] = F32FromASCII(Float);
							Node = Node->Next;
						}
					}

					Line = Line->Next;
					LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
					Node = LineData.First;
					Header = Node->String;
					Header.Data[Header.Size] = 0;
					Assert(StringsAreSame(Header, "BIND:"));

					Line = Line->Next;
					LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
					Node = LineData.First;
					f32 *BindT = &Mesh->BindTransform.E[0][0];
					for(u32 Index = 0; Index < 16; Index++)
					{
						Float = Node->String;
						Float.Data[Float.Size] = 0;
						BindT[Index] = F32FromASCII(Float);
						Node = Node->Next;
					}

					Line = Line->Next;
					LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
					Node = LineData.First;
					Header = Node->String;
					Header.Data[Header.Size] = 0;
					Assert(StringsAreSame(Header, "INV_BIND:"));

					Node = Node->Next;
					Count = Node->String;
					Count.Data[Count.Size] = 0;
					u32 InvBindTCount = U32FromASCII(Count);
					Assert(InvBindTCount == (Mesh->JointCount * 16));

					Mesh->InvBindTransforms = PushArray(Arena, Mesh->JointCount, mat4);
					for(u32 Index = 0; Index < Mesh->JointCount; ++Index)
					{
						Line = Line->Next;
						LineData = StringSplit(Arena, Line->String, LineDelimeters, 3);
						Node = LineData.First;

						mat4 *InvBindT = Mesh->InvBindTransforms + Index;
						T = &InvBindT->E[0][0];

						for(u32 k = 0; k < 16; k++)
						{
							Float = Node->String;
							Float.Data[Float.Size] = 0;
							T[k] = F32FromASCII(Float);
							Node = Node->Next;
						}
					}

					Line = Line->Next;
					Assert(DoneProcessingMesh(Line));

					mat4 I = Mat4Identity();
					for(u32 Index = 0; Index < Mesh->JointCount; ++Index)
					{
						joint *Joint = Mesh->Joints + Index;

						Mesh->JointNames[Index] = Joint->Name;
						Mesh->JointTransforms[Index] = I;
						Mesh->ModelSpaceTransforms[Index] = I;
					}
				}
			}
		}
	}

	return(Model);
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

internal void
ColladaPrint(xml_node *Node)
{
	for(s32 ChildIndex = 0; ChildIndex < Node->ChildrenCount; ++ChildIndex)
	{
		xml_node *N = Node->Children[ChildIndex];
		printf("Tag:%s\nInnerText:%s\n\n", (char *)N->Tag.Data, (char *)N->InnerText.Data);

		if(*N->Children)
		{
			ColladaPrint(N);
		}
	}
}

internal void
FileWriteStringAndNewLine(FILE *OpenFile, string S)
{
	Assert(OpenFile);
	fputs((char *)S.Data, OpenFile);
	fputs("\n", OpenFile);
}

internal void
FileWriteStringAndNewLine(FILE *OpenFile, char *S)
{
	Assert(OpenFile);
	fputs(S, OpenFile);
	fputs("\n", OpenFile);
}

int main(int Argc, char **Argv)
{
	char *ConversionType = *(Argv + 1);
	char *FileName = *(Argv + 2);
	if(ConversionType && FileName)
	{
		b32 ConvertMesh = false;
		b32 ConvertAnimation = false;

		if(StringsAreSame(ConversionType, "-a"))
		{
			ConvertAnimation = true;
		}

		if(StringsAreSame(ConversionType, "-m"))
		{
			ConvertMesh = true;
		}

		if(ConvertMesh || ConvertAnimation)
		{
			u32 Length = StringLength(FileName);
			char *AtExtension = 0;
			for(char *C = (FileName + Length - 1); *C--;)
			{
				if(*C == '.')
				{
					AtExtension = C;
					break;
				}
			}

			string FileExt = StringFromRange((u8 *)AtExtension, (u8 *)(FileName + Length));
			if(StringsAreSame(FileExt, ".dae"))
			{
				void *Memory = calloc(Megabyte(256), sizeof(u8));
				if(Memory)
				{
					memory_arena Arena_;
					ArenaInitialize(&Arena_, (u8 *)Memory, Megabyte(256));
					memory_arena *Arena = &Arena_;

					string Temp = StringFromRange((u8 *)FileName, (u8 *)AtExtension);
					string PathToFile = StringAllocAndCopy(Arena, Temp);

					u8 MeshExt[] = ".mesh";
					u8 AnimExt[] = ".animation";

					string MeshFileName = StringAllocAndCopy(Arena, PathToFile);
					string AnimFileName = StringAllocAndCopy(Arena, PathToFile);

					MeshFileName = StringCat(Arena, PathToFile, String(MeshExt));
					AnimFileName = StringCat(Arena, PathToFile, String(AnimExt));

					loaded_dae LoadedDAE = {};
					printf("Loading %s\n", FileName);
					LoadedDAE = ColladaFileLoad(Arena, FileName);

					if(ConvertMesh)
					{
						printf("Converting mesh %s\n", (char *)MeshFileName.Data);
						ConvertMeshFormat(Arena, LoadedDAE, (char *)MeshFileName.Data);
					}

					if(ConvertAnimation)
					{
						printf("Converting animation %s\n", (char *)AnimFileName.Data);
						ConvertAnimationFormat(Arena, LoadedDAE, (char *)AnimFileName.Data);
					}

					printf("Done\n");

					//model Model = ModelLoad(Arena, (char *)MeshFileName.Data);
					//Model.AnimationsInfo = AnimationInfoLoad(Arena, (char *)AnimFileName.Data);
				}
			}
			else
			{
				printf("Error invalid file: %s\n", FileName);
				printf("Collada files(.dae) are only accepted\n");
			}
		}
		else
		{
			printf("Error invalid conversion arguement: %s\n", ConversionType);
			printf("Valid arguments: -m, -a\n");
		}
	}
	else
	{
		if(!ConversionType)
		{
			printf("Error invalid conversion argument: %s\n", ConversionType);
			printf("Valid arguments: -m, -a\n");
		}

		if(!FileName)
		{
			printf("Error inalid file name: %s\n", FileName);
		}
	}

	return(0);
}
