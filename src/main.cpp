
/*

 TODO(Justin):
 [] WARNING using String() on a token is bad and can easily result in an access violation. REMOVE THIS
 [] Better buffering of text using memory arenas
 [] Remove crt functions
 [] Change children array from fixed size to allocate on demand
	[] Is there a way to a priori determine the size (then can allocate)?
	[] Sparse hash table?
	[] Dynamic list/array
 [] Mesh initialization
	[X] Parse normals and uvs such that they are in 1-1 correspondence with the vertex positions (not so for collada files) 
		[] Confirm using Phong lighting
	[] Init a mesh with one tree traversal
	[] Free or figure out a way to not have to not have to allocate so much when initalizing the mesh
	[] Traverse the tree once.
	[] Probably just need to traverse each sub tree that is one level down from the COLLADA node
	[] Temporary memory? Or just parse the data directly from the inner text?
	[] Handle more complicated meshes
	[] Handle meshes with materials
 [] Skeleton initialization
	[] Library Controllers done in one tree traversal
	[X] Skeletal animation transforms
	[X] Library Animations done in one tree traversal
	[] Normalize weights so that they are a convex combination
	[] Instead of storing mat4's for animation, store pos, quat, scale and construct mat4's
	[] Lerp between key frames using pos, quat, and scale.
 [] Write model data to simple file format (mesh/animation/material)
 [] How to determine the skeleton? Instance controller and skeleton node however there can exist more than one
	skeleton in a collada file.
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
#define COLLADA_NODE_CHILDREN_MAX_COUNT 75

#define COMPONENT_COUNT_V3 3
#define COMPONENT_COUNT_N 3
#define COMPONENT_COUNT_UV 2

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
#include "gl_util.cpp"

// WARNING(Justin): The max loop count must be a CONSTANT we cannot pass the
// joint count in and use it to cap the number of iterations of a loop.

// WARNING(Justin): If the compiler and linker determines that an attribute is
// not used IT WILL OPTIMIZE IT OUT OF THE PIPELINE AND CHAOS ENSUES.
// To detect this, everytime an attribute is initialized a counter can be
// incremented. After initializing all attributes use glGetProgramiv with
// GL_ACTIVE_ATTRIBUTES and ASSERT that the return value is equal to the
// counter.

char *BasicVsSrc = R"(
#version 430 core
layout (location = 0) in vec3 P;
layout (location = 1) in vec3 Normal;
layout (location = 2) in uint JointCount;
layout (location = 3) in uvec3 JointTransformIndices;
layout (location = 4) in vec3 Weights;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

#define MAX_JOINT_COUNT 5
uniform mat4 Transforms[20];

out vec4 WeightPaint;
out vec3 N;

void main()
{

	WeightPaint = vec4(1 - Weights[0], 1 - Weights[1], 1 - Weights[2], 1.0);
	
	vec4 Pos = vec4(0.0);
	for(uint i = 0; i < 3; ++i)
	{
		if(i < JointCount)
		{
			uint JIndex = JointTransformIndices[i];
			float W = Weights[i];
			mat4 T = Transforms[JIndex];
			Pos += W * T * vec4(P, 1.0);
		}
	}

	gl_Position = Projection * View * Model * Pos;
	N = Normal;
})";

char *BasicFsSrc = R"(
#version 430 core

in vec4 WeightPaint;
in vec3 N;

uniform vec3 Color;

out vec4 Result;
void main()
{
	Result = WeightPaint + 0.1f * vec4(N, 1.0);
})";

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

				string Data = String((u8 *)(Content + Index));
				char *Delimeters = "<>";
				string_list List = StringSplit(Arena, Data, (u8 *)Delimeters, 2);

				string_node *T = List.First;
				T = T->Next;

				Result.Root = PushXMLNode(Arena, 0);
				xml_node *CurrentNode = Result.Root;

				CurrentNode->Tag = T->String;
				T = T->Next;
				//Token = String((u8 *)strtok_s(0, TagDelimeters, &Context1));
				CurrentNode->InnerText = T->String;
#if 1
				string Token = String((u8 *)strtok_s((char *)(Content + Index), TagDelimeters, &Context1));
				Token = String((u8 *)strtok_s(0, TagDelimeters, &Context1));



				CurrentNode->Tag = Token;
				Token = String((u8 *)strtok_s(0, TagDelimeters, &Context1));
				CurrentNode->InnerText = Token;
#endif


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


int main(int Argc, char **Argv)
{
	void *Memory = calloc(Megabyte(256), sizeof(u8));

	memory_arena Arena_;
	ArenaInitialize(&Arena_, (u8 *)Memory, Megabyte(256));
	memory_arena *Arena = &Arena_;

	glfwSetErrorCallback(GLFWErrorCallback);
	if(glfwInit())
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
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
			glewExperimental = GL_TRUE;
			if(glewInit() == GLEW_OK)
			{
				//
				// NOTE(Justin): Model info
				//

				loaded_dae MeshDae = ColladaFileLoad(Arena, "..\\data\\thingamajig.dae");

				mesh Mesh = MeshInitFromCollada(Arena, MeshDae);

				model Model = {};
				Model.Meshes = PushStruct(Arena, mesh);
				Model.Meshes[0] = Mesh;

				Model.Basis.O = V3(0.0f, 0.0f, -5.0f);
				Model.Basis.X = XAxis();
				Model.Basis.Y = YAxis();
				Model.Basis.Z = ZAxis();

				v3 Color = V3(1.0f, 0.5f, 0.31f);

				//
				// NOTE(Justin): Transformations
				//

				mat4 ModelTransform = Mat4Translate(Model.Basis.O) * Mat4YRotation(-30.0f);

				v3 CameraP = V3(0.0f, 5.0f, 3.0f);
				v3 Direction = V3(0.0f, -0.5f, -1.0f);
				mat4 CameraTransform = Mat4Camera(CameraP, CameraP + Direction);

				f32 FOV = DegreeToRad(45.0f);
				f32 Aspect = (f32)Window.Width / (f32)Window.Height;
				f32 ZNear = 0.1f;
				f32 ZFar = 100.0f;
				mat4 PerspectiveTransform = Mat4Perspective(FOV, Aspect, ZNear, ZFar);

				glEnable(GL_DEBUG_OUTPUT);
				glDebugMessageCallback(GLDebugCallback, 0);

				glFrontFace(GL_CCW);
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				//
				// NOTE(Justin): Opengl shader initialization
				//

				u32 ShaderProgram = GLProgramCreate(BasicVsSrc, BasicFsSrc);

				u32 VA;
				u32 PosVB, NormVB, TexVB, JointInfoVB;
				u32 IBO;
				s32 ExpectedAttributeCount = 0;

				glGenVertexArrays(1, &VA);
				glBindVertexArray(VA);

				glGenBuffers(1, &PosVB);
				glBindBuffer(GL_ARRAY_BUFFER, PosVB);
				glBufferData(GL_ARRAY_BUFFER, Mesh.PositionsCount * sizeof(f32), Mesh.Positions, GL_STATIC_DRAW);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
				glEnableVertexAttribArray(0);
				ExpectedAttributeCount++;

				glGenBuffers(1, &NormVB);
				glBindBuffer(GL_ARRAY_BUFFER, NormVB);
				glBufferData(GL_ARRAY_BUFFER, Mesh.NormalsCount * sizeof(f32), Mesh.Normals, GL_STATIC_DRAW);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
				glEnableVertexAttribArray(1);
				ExpectedAttributeCount++;

				glGenBuffers(1, &JointInfoVB);
				glBindBuffer(GL_ARRAY_BUFFER, JointInfoVB);
				glBufferData(GL_ARRAY_BUFFER, Mesh.JointInfoCount * sizeof(joint_info), Mesh.JointsInfo, GL_STATIC_DRAW);

				glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(joint_info), (void *)0);
				glVertexAttribIPointer(3, 3, GL_UNSIGNED_INT, sizeof(joint_info), (void *)(1 * sizeof(u32)));
				glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(joint_info), (void *)(4 * sizeof(u32)));

				glEnableVertexAttribArray(2);
				glEnableVertexAttribArray(3);
				glEnableVertexAttribArray(4);
				ExpectedAttributeCount += 3;

				GLIBOInit(&IBO, Mesh.Indices, Mesh.IndicesCount);

				s32 AttrCount;
				glGetProgramiv(ShaderProgram, GL_ACTIVE_ATTRIBUTES, &AttrCount);
				Assert(ExpectedAttributeCount == AttrCount);

				char Name[256];
				s32 Size = 0;
				GLsizei Length = 0;
				GLenum Type;
				for(s32 i = 0; i < AttrCount; ++i)
				{
					glGetActiveAttrib(ShaderProgram, i, sizeof(Name), &Length, &Size, &Type, Name);
					printf("Attribute:%d\nName:%s\nSize:%d\n\n", i, Name, Size);
				}

				glUseProgram(ShaderProgram);
				UniformMatrixSet(ShaderProgram, "Model", ModelTransform);
				UniformMatrixSet(ShaderProgram, "View", CameraTransform);
				UniformMatrixSet(ShaderProgram, "Projection", PerspectiveTransform);
				UniformV3Set(ShaderProgram, "Color", Color);


				// NOTE(Justin): Test ONE animation first

				animation_info *AnimInfo = Mesh.AnimationsInfo + 4;

				u32 KeyFrameIndex = 0;
				f32 AnimationCurrentTime = 0.0f;

				glfwSetTime(0.0);

				f32 StartTime = 0.0f;
				f32 EndTime = 0.0f;
				f32 DtForFrame = 0.0f;
				f32 Angle = 0.0f;
				while(!glfwWindowShouldClose(Window.Handle))
				{
					// NOTE(Justin): Update Animation time

					AnimationCurrentTime += DtForFrame;
					if(AnimationCurrentTime > AnimInfo->Times[KeyFrameIndex + 1])
					{
						KeyFrameIndex++;
						if(KeyFrameIndex == AnimInfo->TimeCount)
						{
							KeyFrameIndex = 0;
							AnimationCurrentTime = 0.0f;
						}
					}

					if(Mesh.JointInfoCount != 0)
					{
						mat4 Bind = *Mesh.BindTransform;

						mat4 RootJointT = *Mesh.Joints[0].Transform;
						mat4 RootInvBind = Mesh.InvBindTransforms[0];

						Mesh.JointTransforms[0] = RootJointT;
						Mesh.ModelSpaceTransforms[0] = RootJointT * RootInvBind * Bind;
						mat4 JointTransform = Mat4Identity();
						for(u32 Index = 1; Index < Mesh.JointCount; ++Index)
						{
							joint *Joint = Mesh.Joints + Index;

							mat4 ParentTransform = Mesh.JointTransforms[Joint->ParentIndex];

							if((s32)Index == AnimInfo->JointIndex)
							{
								JointTransform = AnimInfo->AnimationTransforms[KeyFrameIndex];
							}
							else
							{
								JointTransform = *Joint->Transform;
							}

							JointTransform = ParentTransform * JointTransform;
							mat4 InvBind = Mesh.InvBindTransforms[Index];

							Mesh.JointTransforms[Index] = JointTransform;
							Mesh.ModelSpaceTransforms[Index] = JointTransform * InvBind * Bind;
						}
					}

					glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					glUseProgram(ShaderProgram);
					UniformMatrixArraySet(ShaderProgram, "Transforms", Mesh.ModelSpaceTransforms, Mesh.JointCount);
					glDrawElements(GL_TRIANGLES, Mesh.IndicesCount, GL_UNSIGNED_INT, 0);

					glfwSwapBuffers(Window.Handle);

					EndTime = (f32)glfwGetTime();
					DtForFrame = EndTime - StartTime;
					StartTime = EndTime;

					glfwPollEvents();
				}
			}
		}
	}

	return(0);
}
