
/*
 TODO(Justin):
 [] Remove dependencies GLFW and GLEW
 [] WARNING using String() on a token is bad and can easily result in an access violation. REMOVE THIS
 [] Remove strtok_s
 [] Mesh initialization
	[X] Parse normals and uvs such that they are in 1-1 correspondence with the vertex positions (not so for collada files) 
	[] Use temporary memory when initializing mesh
	[] Move animation data outside mesh struct
 [] Skeleton initialization
	[] Library Controllers done in one tree traversal
	[X] Skeletal animation transforms
	[X] Library Animations done in one tree traversal
	[X] Normalize weights so that they are a convex combination
	[] Instead of storing mat4's for animation, store pos, quat, scale and construct mat4's
	[] Some meshes may use the same rig some may not, need to handle this
	[] Need a way to organize multiple meshes, controllers, and joint hierarchies.
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

#define MAX_JOINT_COUNT 70
uniform mat4 Transforms[MAX_JOINT_COUNT];

out vec3 N;

void main()
{
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
	N = vec3(transpose(inverse(Model)) * vec4(Normal, 1.0));
})";

char *BasicFsSrc = R"(
#version 430 core

in vec3 N;

uniform vec4 Diffuse;
uniform vec4 Specular;
uniform float Shininess;

uniform vec3 LightDir;
uniform vec3 CameraP;

out vec4 Result;
void main()
{
	vec3 LightColor = vec3(1.0);
	vec3 Normal = normalize(N);
	bool FacingTowards = dot(Normal, LightDir) > 0.0;

	vec3 Diff = vec3(0.0);
	vec3 Spec = vec3(0.0);
	if(FacingTowards)
	{
		float D = max(dot(Normal, LightDir), 0.0);
		Diff = LightColor * D * Diffuse.xyz;

		vec3 ViewDir = normalize(CameraP);
		vec3 R = reflect(LightDir, Normal);
		float S = pow(max(dot(ViewDir, R), 0.0), Shininess);
		Spec = LightColor * S * Specular.xyz;
	}

	Result = vec4(Diff + Spec, 1.0);
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
			glfwSetInputMode(Window.Handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetFramebufferSizeCallback(Window.Handle, GLFWFrameBufferResizeCallBack);
			glewExperimental = GL_TRUE;
			if(glewInit() == GLEW_OK)
			{
				//
				// NOTE(Justin): Model info
				//

				loaded_dae LoadedDAE = {};
				LoadedDAE = ColladaFileLoad(Arena, "..\\data\\IdleShiftWeight.dae");
				model Model = ModelInitFromCollada(Arena, LoadedDAE);

				Model.Basis.O = V3(0.0f, -80.0f, -250.0f);
				Model.Basis.X = XAxis();
				Model.Basis.Y = YAxis();
				Model.Basis.Z = ZAxis();

				//
				// NOTE(Justin): Transformations
				//

				mat4 ModelTransform = Mat4Translate(Model.Basis.O);

				v3 CameraP = V3(0.0f, 5.0f, 3.0f);
				v3 Direction = V3(0.0f, 0.0f, -1.0f);
				mat4 CameraTransform = Mat4Camera(CameraP, CameraP + Direction);

				f32 FOV = DegreeToRad(45.0f);
				f32 Aspect = (f32)Window.Width / (f32)Window.Height;
				f32 ZNear = 0.1f;
				f32 ZFar = 100.0f;
				mat4 PerspectiveTransform = Mat4Perspective(FOV, Aspect, ZNear, ZFar);

				glEnable(GL_DEBUG_OUTPUT);
				glDebugMessageCallback(GLDebugCallback, 0);

				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);

				glFrontFace(GL_CCW);
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);

				glEnable(GL_MULTISAMPLE);
				glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

				//
				// NOTE(Justin): Opengl shader initialization
				//


				u32 ShaderProgram = GLProgramCreate(BasicVsSrc, BasicFsSrc);

				for(u32 MeshIndex = 0; MeshIndex < Model.MeshCount; ++MeshIndex)
				{
					s32 ExpectedAttributeCount = 0;

					mesh *Mesh = Model.Meshes + MeshIndex;

					glGenVertexArrays(1, &Model.VA[MeshIndex]);
					glBindVertexArray(Model.VA[MeshIndex]);

					glGenBuffers(1, &Model.PosVB[MeshIndex]);
					glBindBuffer(GL_ARRAY_BUFFER, Model.PosVB[MeshIndex]);
					glBufferData(GL_ARRAY_BUFFER, Mesh->PositionsCount * sizeof(f32), Mesh->Positions, GL_STATIC_DRAW);
					glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
					glEnableVertexAttribArray(0);
					ExpectedAttributeCount++;

					glGenBuffers(1, &Model.NormVB[MeshIndex]);
					glBindBuffer(GL_ARRAY_BUFFER, Model.NormVB[MeshIndex]);
					glBufferData(GL_ARRAY_BUFFER, Mesh->NormalsCount * sizeof(f32), Mesh->Normals, GL_STATIC_DRAW);
					glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
					glEnableVertexAttribArray(1);
					ExpectedAttributeCount++;

					glGenBuffers(1, &Model.JointInfoVB[MeshIndex]);
					glBindBuffer(GL_ARRAY_BUFFER, Model.JointInfoVB[MeshIndex]);
					glBufferData(GL_ARRAY_BUFFER, Mesh->JointInfoCount * sizeof(joint_info), Mesh->JointsInfo, GL_STATIC_DRAW);

					glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(joint_info), 0);
					glVertexAttribIPointer(3, 3, GL_UNSIGNED_INT, sizeof(joint_info), (void *)(1 * sizeof(u32)));
					glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(joint_info), (void *)(4 * sizeof(u32)));

					glEnableVertexAttribArray(2);
					glEnableVertexAttribArray(3);
					glEnableVertexAttribArray(4);
					ExpectedAttributeCount += 3;

					GLIBOInit(&Model.IBO[MeshIndex], Mesh->Indices, Mesh->IndicesCount);
					glBindVertexArray(0);

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
				}

				glUseProgram(ShaderProgram);
				UniformMatrixSet(ShaderProgram, "Model", ModelTransform);
				UniformMatrixSet(ShaderProgram, "View", CameraTransform);
				UniformMatrixSet(ShaderProgram, "Projection", PerspectiveTransform);
				UniformV3Set(ShaderProgram, "CameraP", CameraP);

				glfwSetTime(0.0);
				f32 StartTime = 0.0f;
				f32 EndTime = 0.0f;
				f32 DtForFrame = 0.0f;
				f32 Angle = 0.0f;

				Model.AnimationsInfo.CurrentTime = 0.0f;
				Model.AnimationsInfo.KeyFrameIndex = 0;
				while(!glfwWindowShouldClose(Window.Handle))
				{
					//
					// NOTE(Justin): Skeletal transformations.
					//

					animation_info *AnimInfo = &Model.AnimationsInfo;
					AnimInfo->CurrentTime += DtForFrame;
					if(AnimInfo->CurrentTime > 0.03f)
					{
						AnimInfo->KeyFrameIndex += 1;
						if(AnimInfo->KeyFrameIndex >= AnimInfo->TimeCount)
						{
							AnimInfo->KeyFrameIndex = 0;
						}

						AnimInfo->CurrentTime = 0.0f;
					}

					for(u32 MeshIndex = 0; MeshIndex < Model.MeshCount; ++MeshIndex)
					{
						mesh Mesh = Model.Meshes[MeshIndex];
						if(Mesh.JointInfoCount != 0)
						{
							mat4 Bind = Mesh.BindTransform;

							joint RootJoint = Mesh.Joints[0];
							mat4 RootJointT = RootJoint.Transform;
							mat4 RootInvBind = Mesh.InvBindTransforms[0];

							u32 JointIndex = JointIndexGet(AnimInfo->JointNames, AnimInfo->JointCount, RootJoint.Name);
							if(JointIndex != -1)
							{
								RootJointT = AnimInfo->Transforms[JointIndex][AnimInfo->KeyFrameIndex];
							}

							Mesh.JointTransforms[0] = RootJointT;
							Mesh.ModelSpaceTransforms[0] = RootJointT * RootInvBind * Bind;
							for(u32 Index = 1; Index < Mesh.JointCount; ++Index)
							{
								joint *Joint = Mesh.Joints + Index;
								mat4 JointTransform = Joint->Transform;

								JointIndex = JointIndexGet(AnimInfo->JointNames, AnimInfo->JointCount, Joint->Name);
								if(JointIndex != -1)
								{
									JointTransform = AnimInfo->Transforms[JointIndex][AnimInfo->KeyFrameIndex];
								}

								mat4 ParentTransform = Mesh.JointTransforms[Joint->ParentIndex];
								JointTransform = ParentTransform * JointTransform;
								mat4 InvBind = Mesh.InvBindTransforms[Index];

								Mesh.JointTransforms[Index] = JointTransform;
								Mesh.ModelSpaceTransforms[Index] = JointTransform * InvBind * Bind;
							}
						}
					}

					//
					// NOTE(Justin): Render.
					//

					glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					glUseProgram(ShaderProgram);

					Angle += DtForFrame;
					UniformV3Set(ShaderProgram, "LightDir", V3(2.0f * cosf(Angle), 0.0f, 2.0f * sinf(Angle)));

					glBindVertexArray(Model.VA[0]);
					UniformMatrixArraySet(ShaderProgram, "Transforms", Model.Meshes[0].ModelSpaceTransforms, Model.Meshes[0].JointCount);
					UniformV4Set(ShaderProgram, "Diffuse", Model.Meshes[0].MaterialSpec.Diffuse);
					UniformV4Set(ShaderProgram, "Specular", Model.Meshes[0].MaterialSpec.Specular);
					UniformF32Set(ShaderProgram, "Shininess", Model.Meshes[0].MaterialSpec.Shininess);
					glDrawElements(GL_TRIANGLES, Model.Meshes[0].IndicesCount, GL_UNSIGNED_INT, 0);
					glBindVertexArray(0);

					glBindVertexArray(Model.VA[1]);
					UniformMatrixArraySet(ShaderProgram, "Transforms", Model.Meshes[1].ModelSpaceTransforms, Model.Meshes[1].JointCount);
					UniformV4Set(ShaderProgram, "Diffuse", Model.Meshes[1].MaterialSpec.Diffuse);
					UniformV4Set(ShaderProgram, "Specular", Model.Meshes[1].MaterialSpec.Specular);
					UniformF32Set(ShaderProgram, "Shininess", Model.Meshes[1].MaterialSpec.Shininess);
					glDrawElements(GL_TRIANGLES, Model.Meshes[1].IndicesCount, GL_UNSIGNED_INT, 0);
					glBindVertexArray(0);

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
