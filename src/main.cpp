
/*
 TODO(Justin):
 [] Instead of storing mat4's for animation, store pos, quat, scale and construct for both key frame and model
 [] Different model loading based on whether or not a normal map is available...
 [] Diffuse, ambient, specular, and normal textures in file format 
 [] WARNING using String() on a token is bad and can easily result in an access violation. REMOVE THIS
 [] Remove strtok_s
 [] Mesh initialization
 [] Use temporary memory when initializing mesh
 [] -f command line option to read in a list of files from a txt file and do the conversion on multiple dae files
*/

#if RENDER_TEST
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#endif

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

#define INDICES_PER_TRIANGLE 3
#define POSITIONS_PER_TRIANGLE 3
#define NORMALS_PER_TRIANGLE 3
#define UVS_PER_TRIANGLE 3

#define FLOATS_PER_POSITION 3
#define FLOATS_PER_NORMAL 3
#define FLOATS_PER_UV 2

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

internal void
FileReadEntire(u8 *Dest, s32 Size, FILE *OpenFile)
{
	fread(Dest, 1, (size_t)Size, OpenFile);
}

internal b32 
FileClose(FILE *OpenFile)
{
	b32 Result = (fclose(OpenFile) == 0);
	return(Result);
}

struct texture
{
	int Width;
	int Height;
	int ChannelCount;
	u32 Handle;
	u8 *Memory;
};

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "math_util.h"
#include "xml.h"
#include "xml.cpp"
#include "mesh.h"
#include "mesh.cpp"
#include "animation.cpp"
#include "asset.h"
#include "asset.cpp"
//#include "serialize.cpp"

#if RENDER_TEST
#include "opengl.cpp"
#include "shaders.h"


int main(int Argc, char **Argv)
{
	void *Memory = calloc(Megabyte(512), sizeof(u8));

	memory_arena Arena_;
	ArenaInitialize(&Arena_, (u8 *)Memory, Megabyte(512));
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
				// NOTE(Justin): Transformations
				//

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
				glEnable(GL_SAMPLE_ALPHA_TO_ONE);
				glEnable(GL_BLEND);

				stbi_set_flip_vertically_on_load(true);

				texture Textures[3] = {};
				char *TextureFiles[] =
				{
					"..\\data\\textures\\white_with_border.bmp",
					"..\\data\\textures\\red.bmp",
				};

				for(u32 TextureIndex = 0; TextureIndex < ArrayCount(TextureFiles); ++TextureIndex)
				{
					OpenGLAllocateTexture(TextureFiles[TextureIndex], &Textures[TextureIndex]);
				}

				//
				// NOTE(Justin): Model info
				//

				model *Models[3] = {};

				Models[0] = PushStruct(Arena, model);
				ConvertMeshFormat(Arena, "test2.mesh", "XBot\\XBot.dae");
				*Models[0] = ModelLoad(Arena, "test2.mesh");

				char *AnimationFiles[] =
				{
					"..\\data\\XBot_PushComplete.animation",
					"..\\data\\XBot_ActionIdle.animation",
					"..\\data\\XBot_RightTurn.animation",
					"..\\data\\XBot_LeftTurn.animation",
					"..\\data\\XBot_RunningToTurn.animation",
					"..\\data\\XBot_RunningChangeDirection.animation",
					"..\\data\\XBot_IdleToSprint.animation",
					"..\\data\\XBot_ActionIdleToStandingIdle.animation",
					"..\\data\\XBot_IdleLookAround.animation",
					"..\\data\\XBot_FemaleWalk.animation",
					"..\\data\\XBot_PushingStart.animation",
					"..\\data\\XBot_Pushing.animation",
					"..\\data\\XBot_PushingStop.animation",
				};

				Models[0]->Basis.O = V3(0.0f, -80.0f, -300.0f);
				Models[0]->Basis.X = XAxis();
				Models[0]->Basis.Y = YAxis();
				Models[0]->Basis.Z = ZAxis();
				Models[0]->Animations.Count = ArrayCount(AnimationFiles);
				Models[0]->Animations.Info = PushArray(Arena, Models[0]->Animations.Count, animation_info);

				for(u32 AnimIndex = 0; AnimIndex < ArrayCount(AnimationFiles); ++AnimIndex)
				{
					animation_info *Info = Models[0]->Animations.Info + AnimIndex;
					*Info = AnimationInfoLoad(Arena, AnimationFiles[AnimIndex]);
				}

				//Models[1] = PushStruct(Arena, model);
				//*Models[1] = ModelLoad(Arena, "..\\data\\Arrow.mesh");
				//Models[1]->Basis.O = V3(1.0f, 5.0f, -2.0f);
				//Models[1]->Basis.X = XAxis();
				//Models[1]->Basis.Y = YAxis();
				//Models[1]->Basis.Z = -1.0f * ZAxis();
				//Models[1]->Meshes[0].TextureHandle = Textures[0].Handle;
				mat4 Scale = Mat4Scale(0.2f);

				//
				// NOTE(Justin): Opengl shader initialization
				//

				u32 Shaders[2];
				Shaders[0] = GLProgramCreate(BasicVsSrc, BasicFsSrc);
				Shaders[1] = GLProgramCreate(CubeVS, CubeFS);

				for(u32 ModelIndex = 0; ModelIndex < ArrayCount(Models); ++ModelIndex)
				{
					model *Model = Models[ModelIndex];
					if(Model)
					{
						if(Model->HasSkeleton)
						{
							OpenGLAllocateAnimatedModel(Models[ModelIndex], Shaders[0]);
							glUseProgram(Shaders[0]);
							UniformMatrixSet(Shaders[0], "View", CameraTransform);
							UniformMatrixSet(Shaders[0], "Projection", PerspectiveTransform);
							UniformV3Set(Shaders[0], "CameraP", CameraP);

						}
						else
						{
							OpenGLAllocateModel(Models[ModelIndex], Shaders[1]);
							glUseProgram(Shaders[1]);
							UniformMatrixSet(Shaders[1], "View", CameraTransform);
							UniformMatrixSet(Shaders[1], "Projection", PerspectiveTransform);
						}
					}
				}

				glfwSetTime(0.0);
				f32 StartTime = 0.0f;
				f32 EndTime = 0.0f;
				f32 DtForFrame = 0.0f;
				f32 Angle = 0.0f;

				while(!glfwWindowShouldClose(Window.Handle))
				{
					for(u32 ModelIndex = 0; ModelIndex < ArrayCount(Models); ++ModelIndex)
					{
						model *Model = Models[ModelIndex];
						if(Model)
						{
							if(Model->HasSkeleton)
							{
								AnimationUpdate(Model, DtForFrame);
							}
						}
					}

					//
					// NOTE(Justin): Render.
					//

					glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					Angle += DtForFrame;
					for(u32 ModelIndex = 0; ModelIndex < ArrayCount(Models); ++ModelIndex)
					{
						model *Model = Models[ModelIndex];
						if(Model)
						{
							if(Model->HasSkeleton)
							{
								glUseProgram(Shaders[0]);
								UniformV3Set(Shaders[0], "LightDir", V3(2.0f * cosf(Angle), 0.0f, 2.0f * sinf(Angle)));
								OpenGLDrawAnimatedModel(Models[ModelIndex], Shaders[0]);
							}
							else
							{
								glUseProgram(Shaders[1]);
								f32 A = 20.0f * Angle;
								mat4 R = Mat4YRotation(DegreeToRad(A));
								UniformMatrixSet(Shaders[1], "Model", Mat4Translate(Models[ModelIndex]->Basis.O) * R * Scale);
								OpenGLDrawModel(Models[ModelIndex], Shaders[1]);
							}
						}
					}

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
#else

int main(int Argc, char **Argv)
{
	char *ConversionType = *(Argv + 1);
	char *FileName = *(Argv + 2);
	if(ConversionType && FileName)
	{
		b32 ConvertMesh = false;
		b32 ConvertAnimation = false;
		b32 ShouldSerialize = false;

		if(StringsAreSame(ConversionType, "-a"))
		{
			ConvertAnimation = true;
		}

		if(StringsAreSame(ConversionType, "-m"))
		{
			ConvertMesh = true;
		}

		if(StringsAreSame(ConversionType, "-ab"))
		{
			ConvertAnimation = true;
			ShouldSerialize = true;
		}

		if(StringsAreSame(ConversionType, "-mb"))
		{
			ConvertMesh = true;
			ShouldSerialize = true;
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

					if(ConvertMesh && ShouldSerialize)
					{
						model Model = ModelLoad(Arena, (char *)MeshFileName.Data);
						Serialize(&Model);
					}

					printf("Done\n");

					//model Model = ModelLoad(Arena, (char *)MeshFileName.Data);
					//animation_info Info = AnimationInfoLoad2(Arena, (char *)AnimFileName.Data);
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

#endif
