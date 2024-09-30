
/*
 TODO(Justin):
 [] File paths from build directory
 [] Remove GLFW
 [] Simple animation file format 
 [] Test different models.
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

#include "memory.h"

inline u32
ArraySum(u32 *A, u32 Count)
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
#include "file_io.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "math_util.h"
#include "xml.h"
#include "xml.cpp"
#include "animation.h"
#include "mesh.h"
#include "mesh.cpp"
#include "animation.cpp"
#include "asset.h"
#include "asset.cpp"

#if RENDER_TEST
#include "opengl.cpp"
#include "shaders.h"

int main(int Argc, char **Argv)
{
	void *Memory = calloc(Megabyte(1024), sizeof(u8));

	memory_arena Arena_;
	ArenaInitialize(&Arena_, (u8 *)Memory, Megabyte(1024));
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


				char *ModelSourceFiles[] = 
				{
					"models\\XBot.dae",
					"models\\YBot.dae",
				};
				char *ModelDestFiles[] = 
				{
					"XBot.mesh",
					"YBot.mesh",
				};

				model *Models[3] = {};
				for(u32 FileIndex = 0; FileIndex < ArrayCount(ModelDestFiles); ++FileIndex)
				{
					Models[FileIndex] = PushStruct(Arena, model);
					ConvertMeshFormat(Arena, ModelDestFiles[FileIndex], ModelSourceFiles[FileIndex]);
					*Models[FileIndex] = ModelLoad(Arena, ModelDestFiles[FileIndex]);
				}

				//
				// NOTE(Justin): Animation info
				//

				char *AnimSourceFiles[] =
				{
					"animations\\XBot_ActionIdle.dae",
					"animations\\XBot_ActionIdleToStandingIdle.dae",
					"animations\\XBot_FemaleWalk.dae",
					"animations\\XBot_IdleLookAround.dae",
					//"animations\\XBot_IdleShiftWeight.dae",
					"animations\\XBot_IdleToSprint.dae",
					"animations\\XBot_LeftTurn.dae",
					"animations\\XBot_Pushing.dae",
					"animations\\XBot_PushingStart.dae",
					"animations\\XBot_PushingStop.dae",
					"animations\\XBot_RightTurn.dae",
					"animations\\XBot_RunToStop.dae",
					"animations\\XBot_Running.dae",
					"animations\\XBot_Running180.dae",
					"animations\\XBot_RunningChangeDirection.dae",
					"animations\\XBot_RunningToTurn.dae",
					"animations\\XBot_Walking.dae",
					"animations\\XBot_WalkingTurn180.dae"
				};

				char *AnimDestFiles[] =
				{
					"XBot_ActionIdle.animation",
					"XBot_ActionIdleToStandingIdle.animation",
					"XBot_FemaleWalk.animation",
					"XBot_IdleLookAround.animation",
					//"XBot_IdleShiftWeight.animation",
					"XBot_IdleToSprint.animation",
					"XBot_LeftTurn.animation",
					"XBot_Pushing.animation",
					"XBot_PushingStart.animation",
					"XBot_PushingStop.animation",
					"XBot_RightTurn.animation",
					"XBot_RunToStop.animation",
					"XBot_Running.animation",
					"XBot_Running180.animation",
					"XBot_RunningChangeDirection.animation",
					"XBot_RunningToTurn.animation",
					"XBot_Walking.animation",
					"XBot_WalkingTurn180.animation"
				};

				Models[0]->Basis.O = V3(100.0f, -80.0f, -400.0f);
				Models[0]->Basis.X = XAxis();
				Models[0]->Basis.Y = YAxis();
				Models[0]->Basis.Z = ZAxis();
				Models[0]->Animations.Count = ArrayCount(AnimDestFiles);
				Models[0]->Animations.Info = PushArray(Arena, Models[0]->Animations.Count, animation_info);

				Models[1]->Basis.O = V3(-100.0f, -80.0f, -500.0f);
				Models[1]->Basis.X = XAxis();
				Models[1]->Basis.Y = YAxis();
				Models[1]->Basis.Z = ZAxis();
				Models[1]->Animations.Count = 1;
				Models[1]->Animations.Info = PushArray(Arena, Models[0]->Animations.Count, animation_info);
				animation_info *YInfo = Models[1]->Animations.Info;
				//ConvertAnimationFormat(Arena, "YBot_RunningJump.animation", "animations\\YBot_RunningJump.dae");
				*YInfo = AnimationLoad(Arena, "YBot_RunningJump.animation");
				mat4 Scale = Mat4Scale(0.2f);

				for(u32 AnimIndex = 0; AnimIndex < ArrayCount(AnimDestFiles); ++AnimIndex)
				{
#if 0
					char *Source = AnimSourceFiles[AnimIndex];
					char *Dest = AnimDestFiles[AnimIndex];

					ConvertAnimationFormat(Arena, Dest, Source);
					animation_info TestAnimation = AnimationLoad(Arena, Dest);
#else
					char *Dest = AnimDestFiles[AnimIndex];
					animation_info TestAnimation = AnimationLoad(Arena, Dest);
#endif

					animation_info *Info = Models[0]->Animations.Info + AnimIndex;
					*Info = TestAnimation;
				}

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

					//Angle += DtForFrame;
					v3 LightDir = V3(0.0f, 0.0f, 1.0f);
					for(u32 ModelIndex = 0; ModelIndex < ArrayCount(Models); ++ModelIndex)
					{
						model *Model = Models[ModelIndex];
						if(Model)
						{
							if(Model->HasSkeleton)
							{
								glUseProgram(Shaders[0]);
								UniformV3Set(Shaders[0], "LightDir", LightDir);
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
						char *Out = (char *)MeshFileName.Data;
						printf("Converting mesh %s\n", (char *)MeshFileName.Data);
						ConvertMeshFormat(Arena, Out, FileName);
						printf("Done\n");
						printf("File saved as %s\n", Out);
					}

					if(ConvertAnimation)
					{
						char *Out = (char *)AnimFileName.Data;
						printf("Converting animation %s\n", (char *)AnimFileName.Data);
						ConvertAnimationFormat(Arena, (char *)AnimFileName.Data, FileName);
						printf("Done\n");
						printf("File saved as %s\n", Out);
					}
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
