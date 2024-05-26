
/*

 TODO(Justin):
 [] WARNING using String() on a token is bad and can easily result in an access violation. REMOVE THIS
 [] Better buffering of text using memory arenas
 [] Remove crt functions
 [] Change children array from fixed size to allocate on demand
	[] Sparse hash table?
	[] Dynamic list/array
	[] Is there a way to a priori determine the size (then can allocate)?
 [] Mesh initialization
	[X] Parse normals and uvs such that they are in 1-1 correspondence with the vertex positions (not so for collada files) 
		[] Confirm using Phong Shading
	[] Init a mesh with one tree traversal
	[] Free or figure out a way to not have to not have to allocate so much when initalizing the mesh
	[] Traverse the tree once.
	[] Probably just need to traverse each sub tree that is one level down from the COLLADA node
	[] Temporary memory? Or just parse the data directly from the inner text?
	[] Handle more complicated meshes
	[] Handle meshes with materials
 [] Skeleton initialization
	[] Library Controllers done in one tree traversal
	[] Skeletal animation transforms
	[X] Library Animations done in one tree traversal
 [] Write model data to simple file format (mesh/animation/material)
 
*/

#include <GL/glew.h>
#include <GLFW/glfw3.h>

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
#define COLLADA_NODE_CHILDREN_MAX_COUNT 30

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
// NOTE(Justin): Bump allocator
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

#define PushArray(Arena, Count, Type) (Type *)PushSize_(Arena, Count * sizeof(Type))
#define PushStruct(Arena, Type) (Type *)PushSize_(Arena, sizeof(Type))
internal void *
PushSize_(memory_arena *Arena, memory_index Size)
{
	Assert((Arena->Used + Size) <= Arena->Size);

	void *Result = Arena->Base + Arena->Used;
	Arena->Used += Size;

	return(Result);
}

#define ArrayCopy(Count, Src, Dest) MemoryCopy((Count)*sizeof(*(Src)), (Src), (Dest))
internal void *
MemoryCopy(memory_index Size, void *SrcInit, void *DestInit)
{
	u8 *Src = (u8 *)SrcInit;
	u8 *Dest = (u8 *)DestInit;
	while(Size--) {*Dest++ = *Src++;}

	return(DestInit);
}

#include "base_strings.h"
#include "base_strings.cpp"
#include "base_math.h"
#include "xml.h"
#include "xml.cpp"

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

// NOTE(Justin): A vertex is set to be affected by AT MOST 3 joints;
struct joint_info
{
	u32 Count;
	u32 JointIndex[3];
	u32 WeightIndex[3];
};

struct animation_info
{
	string JointName;

	u32 TimeCount;
	f32 *Times;

	u32 JointTransformCount;
	f32 *JointTransforms;
};

// NOTE(Justin): All the data is currently in 1-1 correspondence between the
// attributes and skeleton info. I.e. the data for the first vertex is
//	v0: P[0] P[1] P[2]
//	n0: N[0] N[1] N[2]
//	uv0: UV[0] UV[1]
struct mesh
{
	u32 PositionsCount;
	u32 NormalsCount;
	u32 UVCount;
	u32 IndicesCount;
	u32 WeightCount;
	u32 BindPosCount;

	f32 *Positions;
	f32 *Normals;
	f32 *UV;
	u32 *Indices;
	f32 *Weights;

	// NOTE(Justin): The bind poses are a mat4 and are stored by rows. So the
	// first 16 floats represents a mat4 moreoever this is the inverse bind
	// matrix for the first node in JointNames which is the root. 
	f32 *BindPoses;

	u32 JointNameCount;
	string *JointNames;

	u32 JointInfoCount;
	joint_info *JointsInfo;

	u32 AnimationInfoCount;
	animation_info *AnimationsInfo;
};

internal s32
FileSizeGet(FILE *OpenedFile)
{
	Assert(OpenedFile);
	s32 Result = -1;
	fseek(OpenedFile, 0, SEEK_END);
	Result = ftell(OpenedFile);
	fseek(OpenedFile, 0, SEEK_SET);

	return(Result);
}

internal void
FileReadEntireAndNullTerminate(u8 *Dest, s32 Size, FILE *File)
{
	fread(Dest, 1, (size_t)Size, File);
	Dest[Size] = '\0';
}

internal b32 
FileClose(FILE *File)
{
	b32 Result = (fclose(File) == 0);
	return(Result);
}

// TODO(Justin): The collada data can be temporary memory?
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

				char TagDelimeters[] = "<>";
				char InnerTagDelimeters[] = "=\"";

				char *Context1 = 0;
				char *Context2 = 0;

				string Token = String((u8 *)strtok_s((char *)(Content + Index), TagDelimeters, &Context1));
				Token = String((u8 *)strtok_s(0, TagDelimeters, &Context1));

				Result.Root = PushXMLNode(Arena, 0);
				xml_node *CurrentNode = Result.Root;

				CurrentNode->Tag = Token;
				Token = String((u8 *)strtok_s(0, TagDelimeters, &Context1));
				CurrentNode->InnerText = Token;

				while(Token.Data)
				{
					Token = String((u8 *)strtok_s(0, TagDelimeters, &Context1));
					if(Token.Data[0] == '/')
					{
						if(CurrentNode->Parent)
						{
							CurrentNode = CurrentNode->Parent;
							Token = String((u8 *)strtok_s(0, TagDelimeters, &Context1));
						}
						else
						{
							break;
						}
					}
					else
					{
						CurrentNode = ChildNodeAdd(Arena, CurrentNode);

						if(NodeHasKeysValues(Token) && StringEndsWith(Token, '/'))
						{
							string AtSpace = String((u8 *)strstr((char *)Token.Data, " "));

							CurrentNode->Tag.Size = (Token.Size - AtSpace.Size);
							CurrentNode->Tag.Data = PushArray(Arena, CurrentNode->Tag.Size + 1, u8);

							ArrayCopy(CurrentNode->Tag.Size, Token.Data, CurrentNode->Tag.Data);
							CurrentNode->Tag.Data[CurrentNode->Tag.Size] = '\0';

							string Temp = {};
							Temp.Size = AtSpace.Size - 1;
							Temp.Data = PushArray(Arena, Temp.Size + 1, u8);
							ArrayCopy(Temp.Size, (AtSpace.Data + 1), Temp.Data);
							Temp.Data[Temp.Size] = '\0';

							NodeProcessKeysValues(Arena, CurrentNode, Temp, Context2, InnerTagDelimeters);

							Token = String((u8 *)strtok_s(0, TagDelimeters, &Context1));
							CurrentNode->InnerText = Token;
							CurrentNode = CurrentNode->Parent;
						}
						else if(NodeHasKeysValues(Token))
						{
							string AtSpace = String((u8 *)strstr((char *)Token.Data, " "));

							CurrentNode->Tag.Size = (Token.Size - AtSpace.Size);
							CurrentNode->Tag.Data = PushArray(Arena, CurrentNode->Tag.Size + 1, u8);

							ArrayCopy(CurrentNode->Tag.Size, Token.Data, CurrentNode->Tag.Data);
							CurrentNode->Tag.Data[CurrentNode->Tag.Size] = '\0';

							string Temp = {};
							Temp.Size = AtSpace.Size - 1;
							Temp.Data = PushArray(Arena, Temp.Size + 1, u8);
							ArrayCopy(Temp.Size, (AtSpace.Data + 1), Temp.Data);
							Temp.Data[Temp.Size] = '\0';

							NodeProcessKeysValues(Arena, CurrentNode, Temp, Context2, InnerTagDelimeters);

							Token = String((u8 *)strtok_s(0, TagDelimeters, &Context1));
							CurrentNode->InnerText = Token;

						}
						else if(StringEndsWith(Token, '/'))
						{
							CurrentNode->Tag = Token;
							Token = String((u8 *)strtok_s(0, TagDelimeters, &Context1));
							CurrentNode->InnerText = Token;
							CurrentNode = CurrentNode->Parent;
						}
						else
						{
							CurrentNode->Tag = Token;
							Token = String((u8 *)strtok_s(0, TagDelimeters, &Context1));
							CurrentNode->InnerText = Token;

						}
					}
				}
			}
		}
	}

	return(Result);
}

//
// NOTE(Justin): GLFW & GL
//

struct window
{
	GLFWwindow *Handle;
	s32 Width, Height;
};

internal void
GLFWErrorCallback(s32 ErrorCode, const char *Message)
{
	printf("GLFW Error: Code %d Message %s", ErrorCode, Message);
}

internal void
GLFWFrameBufferResizeCallBack(GLFWwindow *Window, s32 Width, s32 Height)
{
	glViewport(0, 0, Width, Height);
}

internal void GLAPIENTRY
GLDebugCallback(GLenum Source, GLenum Type, GLuint ID, GLenum Severity, GLsizei Length,
		const GLchar *Message, const void *UserParam)
{
	printf("OpenGL Debug Callback: %s\n", Message);
}

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
			Info->JointName = JointName;

			(*AnimationInfoIndex)++;
		}
		else if(*Node->Children)
		{
			AnimationInfoGet(Arena, Node, AnimationInfo, AnimationInfoIndex);
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
		ParseXMLFloatArray(Arena, &Controllers, &Mesh.BindPoses, &Mesh.BindPosCount, "skin-bind_poses-array");
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
		u32 JointsAndWeightsIndex = 0;
		for(u32 JointIndex = 0; JointIndex < Mesh.JointInfoCount; ++JointIndex)
		{
			u32 JointCountForVertex = JointCountArray[JointIndex];

			joint_info *JointInfo = Mesh.JointsInfo + JointIndex;
			JointInfo->Count = JointCountForVertex;
			for(u32 k = 0; k < JointInfo->Count; ++k)
			{
				JointInfo->JointIndex[k] = JointsAndWeights[JointsAndWeightsIndex++];
				JointInfo->WeightIndex[k] = JointsAndWeights[JointsAndWeightsIndex++];
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
	// NOTE(Justin): Visual Scenes
	//

	xml_node LibVisScenes = {};
	NodeGet(Root, &LibVisScenes, "library_visual_scenes");
	if(LibVisScenes.ChildrenCount != 0)
	{

	}

	int y = 0;

	return(Mesh);
}

internal u32
GLProgramCreate(char *VS, char *FS)
{
	u32 VSHandle;
	u32 FSHandle;

	VSHandle = glCreateShader(GL_VERTEX_SHADER);
	FSHandle = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(VSHandle, 1, &VS, 0);
	glShaderSource(FSHandle, 1, &FS, 0);

	b32 VSIsValid = false;
	b32 FSIsValid = false;
	char Buffer[512];

	glCompileShader(VSHandle);
	glGetShaderiv(VSHandle, GL_COMPILE_STATUS, &VSIsValid);
	if(!VSIsValid)
	{
		glGetShaderInfoLog(VSHandle, 512, 0, Buffer);
		printf("ERROR: Vertex Shader Compile Failed\n %s", Buffer);
	}

	glCompileShader(FSHandle);
	glGetShaderiv(FSHandle, GL_COMPILE_STATUS, &FSIsValid);
	if(!FSIsValid)
	{
		glGetShaderInfoLog(FSHandle, 512, 0, Buffer);
		printf("ERROR: Fragment Shader Compile Failed\n %s", Buffer);
	}

	u32 Program;
	Program = glCreateProgram();
	glAttachShader(Program, VSHandle);
	glAttachShader(Program, FSHandle);
	glLinkProgram(Program);
	glValidateProgram(Program);

	b32 ProgramIsValid = false;
	glGetProgramiv(Program, GL_LINK_STATUS, &ProgramIsValid);
	if(!ProgramIsValid)
	{
		glGetProgramInfoLog(Program, 512, 0, Buffer);
		printf("ERROR: Program link failed\n %s", Buffer);
	}

	glDeleteShader(VSHandle);
	glDeleteShader(FSHandle);

	u32 Result = Program;

	return(Result);
}

internal void
UniformMatrixSet(u32 ShaderProgram, char *UniformName, mat4 M)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	glUniformMatrix4fv(UniformLocation, 1, GL_TRUE, &M.E[0][0]);
}

internal void
UniformV3Set(u32 ShaderProgram, char *UniformName, v3 V)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	glUniform3fv(UniformLocation, 1, &V.E[0]);
}

internal void
AttributesInterleave(f32 *BufferData, mesh *Mesh)
{
	u32 BufferIndex = 0;

	u32 Stride = 3;
	u32 UVStride = 2;
	for(u32 Index = 0; Index < Mesh->PositionsCount/3; ++Index)
	{
		BufferData[BufferIndex++] = Mesh->Positions[Stride * Index];
		BufferData[BufferIndex++] = Mesh->Positions[Stride * Index + 1];
		BufferData[BufferIndex++] = Mesh->Positions[Stride * Index + 2];

		BufferData[BufferIndex++] = Mesh->Normals[Stride * Index];
		BufferData[BufferIndex++] = Mesh->Normals[Stride * Index + 1];
		BufferData[BufferIndex++] = Mesh->Normals[Stride * Index + 2];

		BufferData[BufferIndex++] = Mesh->UV[UVStride * Index];
		BufferData[BufferIndex++] = Mesh->UV[UVStride * Index + 1];
	}
}

int main(int Argc, char **Argv)
{
	void *Memory = calloc(Megabyte(1), sizeof(u8));

	memory_arena Arena_;
	ArenaInitialize(&Arena_, (u8 *)Memory, Megabyte(1));
	memory_arena *Arena = &Arena_;

	glfwSetErrorCallback(GLFWErrorCallback);
	if(glfwInit())
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

		window Window = {};
		Window.Width = 960;
		Window.Height = 540;
		Window.Handle = glfwCreateWindow(Window.Width, Window.Height, "YabbaColladaDoo", NULL, NULL);

		if(Window.Handle)
		{
			glfwMakeContextCurrent(Window.Handle);
			glfwSetInputMode(Window.Handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwSetFramebufferSizeCallback(Window.Handle, GLFWFrameBufferResizeCallBack);
			if(glewInit() == GLEW_OK)
			{
				loaded_dae CubeDae = ColladaFileLoad(Arena, "..\\data\\thingamajig.dae");

				mesh Cube = MeshInit(Arena, CubeDae);

				mat4 ModelTransform = Mat4Translate(V3(0.0f, 0.0f, -5.0f));

				v3 CameraP = V3(0.0f, 5.0f, 3.0f);
				v3 Direction = V3(0.0f, -0.5f, -1.0f);
				mat4 CameraTransform = Mat4Camera(CameraP, CameraP + Direction);

				f32 FOV = DegreeToRad(45.0f);
				f32 Aspect = (f32)Window.Width / (f32)Window.Height;
				f32 ZNear = 0.1f;
				f32 ZFar = 100.0f;
				mat4 PerspectiveTransform = Mat4Perspective(FOV, Aspect, ZNear, ZFar);

				mat4 MVP = PerspectiveTransform *
						   CameraTransform *
						   ModelTransform;


				v3 LightP = V3(0.0f, 5.0f, -0.5f);
				v3 LightC = V3(1.0f);
				v3 Color = V3(1.0f, 0.5f, 0.31f);

				glEnable(GL_DEBUG_OUTPUT);
				glDebugMessageCallback(GLDebugCallback, 0);

				glFrontFace(GL_CCW);
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				char *VsSrc = R"(
				#version 330 core
				layout (location = 0) in vec3 P;
				layout (location = 1) in vec3 N;
				layout (location = 2) in vec2 UV;

				uniform mat4 Model;
				uniform mat4 View;
				uniform mat4 Projection;

				out vec3 Normal;
				out vec3 FragP;
				void main()
				{
					FragP = vec3(Model * vec4(P, 1.0));
					Normal = mat3(transpose(inverse(Model))) * N;

					gl_Position = Projection * View * vec4(FragP, 1.0);

				})";

				char *FsSrc = R"(
				#version 330 core

				in vec3 Normal;
				in vec3 FragP;

				uniform vec3 LightP;
				uniform vec3 LightC;
				uniform vec3 CameraP;
				uniform vec3 Color;

				out vec4 FragColor;
				void main()
				{
					vec3 Ambient = 0.1f * LightC;

					vec3 FragToLight = normalize(LightP - FragP);
					vec3 Norm = normalize(Normal);
					vec3 Diffuse = max(dot(FragToLight, Norm), 0.0) * LightC;

					FragColor = vec4((Ambient + Diffuse) * Color, 1.0f);
				})";


				u32 ShaderProgram = GLProgramCreate(VsSrc, FsSrc);

				u32 TotalCount = Cube.PositionsCount + Cube.NormalsCount + Cube.UVCount;
				f32 *BufferData = (f32 *)calloc(TotalCount, sizeof(f32));

				AttributesInterleave(BufferData, &Cube);

				u32 VB, VA;
				glGenVertexArrays(1, &VA);
				glGenBuffers(1, &VB);
				glBindVertexArray(VA);
				glBindBuffer(GL_ARRAY_BUFFER, VB);
				glBufferData(GL_ARRAY_BUFFER, TotalCount * sizeof(f32), BufferData, GL_STATIC_DRAW);

				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void *)0);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void *)(3 * sizeof(f32)));
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void *)(6 * sizeof(f32)));

				glEnableVertexAttribArray(0);
				glEnableVertexAttribArray(1);
				glEnableVertexAttribArray(2);

				u32 IBO;
				glGenBuffers(1, &IBO);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, Cube.IndicesCount * sizeof(u32), Cube.Indices, GL_STATIC_DRAW);

				glUseProgram(ShaderProgram);
				UniformMatrixSet(ShaderProgram, "Model", ModelTransform);
				UniformMatrixSet(ShaderProgram, "View", CameraTransform);
				UniformMatrixSet(ShaderProgram, "Projection", PerspectiveTransform);

				UniformV3Set(ShaderProgram, "LightP", LightP);
				UniformV3Set(ShaderProgram, "LightC", LightC);
				UniformV3Set(ShaderProgram, "CameraP", CameraP);
				UniformV3Set(ShaderProgram, "Color", Color);


				f32 DtForFrame = 0.0f;
				f32 StartTime = 0.0f;
				f32 EndTime = 0.0f;
				while(!glfwWindowShouldClose(Window.Handle))
				{
					StartTime = (f32)glfwGetTime();

					glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					glUseProgram(ShaderProgram);
					glDrawElements(GL_TRIANGLES, Cube.IndicesCount, GL_UNSIGNED_INT, 0);

					glfwSwapBuffers(Window.Handle);

					EndTime = (f32)glfwGetTime();
					DtForFrame = EndTime - StartTime;

					glfwPollEvents();
				}
			}
		}
	}

	return(0);
}
