
/*
 TODO(Justin):
 [] opengl error logging/checking!  
 [] fonts 
 [] imgui 
 [] asset system  
 [] asset file structure 
 [X] Transform normals via joint transforms!  
 [?] How to get the correct normal when using a normal map AND doing skeletal animation?
 [] File paths from build directory
 [X] Simple animation file format 
 [X] Test different models.
 [X] Instead of storing mat4's for animation, store pos, quat, scale and construct for both key frame and model
 [X] Check UV mapping 
 [X] Phong lighting 
 [?] Diffuse, ambient, specular, and normal textures in file format 
 [] WARNING using String() on a token is bad and can easily result in an access violation. REMOVE THIS
 [] Remove strtok_s
 [?] Use temporary memory when initializing mesh
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

typedef uintptr_t umm;

#define internal static
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#define ArrayCount(A) sizeof(A) / sizeof((A)[0])
#define Kilobyte(Count) (1024 * Count)
#define Megabyte(Count) (1024 * Kilobyte(Count))

#define PI32 3.1415926535897f
#define DegreeToRad(Degrees) ((Degrees) * (PI32 / 180.0f))

#define COLLADA_ATTRIBUTE_MAX_COUNT 10
#define COLLADA_NODE_CHILDREN_MAX_COUNT 85

#define PushArray(Arena, Count, Type) (Type *)PushSize_(Arena, Count * sizeof(Type))
#define PushStruct(Arena, Type) (Type *)PushSize_(Arena, sizeof(Type))
#define ArrayCopy(Count, Src, Dest) MemoryCopy((Count)*sizeof(*(Src)), (Src), (Dest))

#define SLLQueuePush_N(First,Last,Node,Next) (((First)==0?\
(First)=(Last)=(Node):\
((Last)->Next=(Node),(Last)=(Node))),\
(Node)->Next=0)
#define SLLQueuePush(First,Last,Node) SLLQueuePush_N(First,Last,Node,Next)

#define OffsetOf(type, Member) (umm)&(((type *)0)->Member)

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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "memory.h"
#include "math_util.h"
#include "strings.h"
#include "xml.h"
#include "animation.h"
#include "mesh.h"
#include "asset.h"
#include "entity.h"
#include "strings.cpp"
#include "file_io.cpp"
#include "xml.cpp"
#include "mesh.cpp"
#include "animation.cpp"
#include "asset.cpp"

struct quad_vertex
{
	v3 P;
	v3 N;
	v3 Tangent;
	v3 BiTangent;
	v2 UV;
	v4 Color;
};

struct quad
{
	quad_vertex Vertices[6];

	u32 VB;
	u32 VA;
	u32 DiffuseTexture;
	u32 NormalTexture;
};

#if RENDER_TEST
#include "opengl.h"
#include "opengl.cpp"
#include "shaders.h"

struct game_state
{
	u32 EntityCount;
	entity Entities[256];

	model Cube;
	model Sphere;
	model XBot;
	model Vampire;
	model Paladin;
	model DebugArrow;

	texture Textures[16];
};

#include "entity.cpp"
#include "glfw.cpp"

internal shader 
ShaderLoad(char *FileNameVS, char *FileNameFS)
{
	shader Result = {};

	FILE *FileVS = fopen(FileNameVS, "r");
	FILE *FileFS = fopen(FileNameFS, "r");
	if(FileVS && FileFS)
	{
		s32 SizeVS = FileSizeGet(FileVS);
		s32 SizeFS = FileSizeGet(FileFS);
		Assert(SizeVS <= ArrayCount(Result.VS));
		Assert(SizeFS <= ArrayCount(Result.FS));

		FileReadEntireAndNullTerminate((u8 *)Result.VS, SizeVS, FileVS);
		FileReadEntireAndNullTerminate((u8 *)Result.FS, SizeFS, FileFS);

		fclose(FileVS);
		fclose(FileFS);
	}

	return(Result);
}

internal quad
QuadDefault(void)
{
	quad Result = {};

	v3 N = V3(0.0f, 0.0f, 1.0f);
	v4 Color = V4(1.0f);
	
	Result.Vertices[0].P = V3(0.0f, 0.0f, 0.0f);
	Result.Vertices[0].N = N;
	Result.Vertices[0].UV = V2(0.0f, 0.0f);
	Result.Vertices[0].Color = Color;

	Result.Vertices[1].P = V3(1.0f, 0.0f, 0.0f);
	Result.Vertices[1].N = N;
	Result.Vertices[1].UV = V2(1.0f, 0.0f);
	Result.Vertices[1].Color = Color;

	Result.Vertices[2].P = V3(1.0f, 1.0f, 0.0f);
	Result.Vertices[2].N = N;
	Result.Vertices[2].UV = V2(1.0f, 1.0f);
	Result.Vertices[2].Color = Color;

	Result.Vertices[3].P = V3(1.0f, 1.0f, 0.0f);
	Result.Vertices[3].N = N;
	Result.Vertices[3].UV = V2(1.0f, 1.0f);
	Result.Vertices[3].Color = Color;

	Result.Vertices[4].P = V3(0.0f, 1.0f, 0.0f);
	Result.Vertices[4].N = N;
	Result.Vertices[4].UV = V2(0.0f, 1.0f);
	Result.Vertices[4].Color = Color;

	Result.Vertices[5].P = V3(0.0f, 0.0f, 0.0f);
	Result.Vertices[5].N = N;
	Result.Vertices[5].UV = V2(0.0f, 0.0f);
	Result.Vertices[5].Color = Color;

	for(u32 TriangleIndex = 0; TriangleIndex < ArrayCount(Result.Vertices) / 3; ++TriangleIndex)
	{
		quad_vertex V0 = Result.Vertices[3 * TriangleIndex + 0];
		quad_vertex V1 = Result.Vertices[3 * TriangleIndex + 1];
		quad_vertex V2 = Result.Vertices[3 * TriangleIndex + 2];

		v3 E0 = V1.P - V0.P;
		v3 E1 = V2.P - V0.P;

		v2 dUV0 = V1.UV - V0.UV;
		v2 dUV1 = V2.UV - V0.UV;

		f32 C = (1.0f / (dUV0.x * dUV1.y - dUV0.y * dUV1.x));

		v3 Tangent = {};
		v3 BiTangent = {};

		Tangent = C * (dUV1.y * E0 - dUV0.y * E1);
		BiTangent = C * (dUV0.x * E1 - dUV1.x * E0);

		Result.Vertices[TriangleIndex + 0].Tangent = Tangent;
		Result.Vertices[TriangleIndex + 1].Tangent = Tangent;
		Result.Vertices[TriangleIndex + 2].Tangent = Tangent;
		Result.Vertices[TriangleIndex + 0].BiTangent = BiTangent;
		Result.Vertices[TriangleIndex + 1].BiTangent = BiTangent;
		Result.Vertices[TriangleIndex + 2].BiTangent = BiTangent;
	}

	return(Result);
}

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
			glfwSetKeyCallback(Window.Handle, GLFWKeyCallBack);
			glfwSetFramebufferSizeCallback(Window.Handle, GLFWFrameBufferResizeCallBack);
			glewExperimental = GL_TRUE;
			if(glewInit() == GLEW_OK)
			{
				//
				// NOTE(Justin): Transformations
				//

				v3 CameraP = V3(0.0f, 5.0f, 30.0f);
				v3 Direction = V3(0.0f, 0.0f, -1.0f);
				mat4 CameraTransform = Mat4Camera(CameraP, CameraP + Direction);

				f32 FOV = DegreeToRad(45.0f);
				f32 Aspect = (f32)Window.Width / (f32)Window.Height;
				f32 ZNear = 0.1f;
				f32 ZFar = 100.0f;
				mat4 PerspectiveTransform = Mat4Perspective(FOV, Aspect, ZNear, ZFar);

				GLenum GLParams[] =
				{
					GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
					GL_MAX_CUBE_MAP_TEXTURE_SIZE,
					GL_MAX_DRAW_BUFFERS,
					GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,
					GL_MAX_TEXTURE_IMAGE_UNITS,
					GL_MAX_TEXTURE_SIZE,
					GL_MAX_VARYING_FLOATS,
					GL_MAX_VERTEX_ATTRIBS,
					GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,
					GL_MAX_VERTEX_UNIFORM_COMPONENTS,
					GL_MAX_VIEWPORT_DIMS,
					GL_STEREO,
				};
				const char* GLParamNames[] =
				{
					"GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS",
					"GL_MAX_CUBE_MAP_TEXTURE_SIZE",
					"GL_MAX_DRAW_BUFFERS",
					"GL_MAX_FRAGMENT_UNIFORM_COMPONENTS",
					"GL_MAX_TEXTURE_IMAGE_UNITS",
					"GL_MAX_TEXTURE_SIZE",
					"GL_MAX_VARYING_FLOATS",
					"GL_MAX_VERTEX_ATTRIBS",
					"GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS",
					"GL_MAX_VERTEX_UNIFORM_COMPONENTS",
					"GL_MAX_VIEWPORT_DIMS",
					"GL_STEREO",
				};

				char *Vendor = (char *)glGetString(GL_VENDOR);
				char *RendererName = (char *)glGetString(GL_RENDERER);
				char *RendererVersion = (char *)glGetString(GL_VERSION);
				char *ShadingLanguageVersion = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
				printf("%s\n", Vendor);
				printf("%s\n", RendererName);
				printf("%s\n", RendererVersion);
				printf("%s\n", ShadingLanguageVersion);
				for(u32 Index = 0; Index < ArrayCount(GLParams); ++Index)
				{
					s32 Param = 0;
					glGetIntegerv(GLParams[Index], &Param);
					printf("%s %i\n", GLParamNames[Index], Param);
				}

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
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				//
				// NOTE(Justin): Assets
				//

				char *TextureFiles[] =
				{
					"textures\\checkerboard.jpg",
					"textures\\awesomeface.png",
					"textures\\brickwall.jpg",
					"textures\\brickwall_normal.jpg",
					"textures\\Vampire_diffuse_transparent.png",
					"textures\\Vampire_specular.png",
					"textures\\Vampire_diffuse.png",
					"textures\\Vampire_emission.png",
					"textures\\Vampire_normal.png",
					"textures\\Paladin_diffuse.png",
					"textures\\Paladin_specular.png",
					"textures\\Paladin_normal.png",
					"textures\\white.bmp"
				};
				char *ShaderFiles[] =
				{
					"shaders\\main.vs",
					"shaders\\main.fs",
					"shaders\\basic.vs",
					"shaders\\basic.fs",
					"shaders\\quad.vs",
					"shaders\\quad.fs",
				};
				char *ModelFiles[] = 
				{
					"XBot.mesh",
					"Sphere.mesh",
					"VampireALusth.mesh",
					"PaladinWithProp.mesh",
				};
				char *XBotAnimations[] =
				{
					"XBot_ActionIdle.animation",
					"XBot_ActionIdleToStandingIdle.animation",
					"XBot_FemaleWalk.animation",
					"XBot_IdleLookAround.animation",
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
				char *VampireAnimations[] =
				{
					"Vampire_Walking.animation",
					"Vampire_Bite.animation",
				};
				char *PaladinAnimations[] =
				{
					"Paladin_SwordAndShieldIdle.animation",
				};

				game_state GameState_ = {};
				game_state *GameState = &GameState_;

				texture *Texture = (texture *)GameState->Textures;
				for(u32 TextureIndex = 0; TextureIndex < ArrayCount(TextureFiles); ++TextureIndex)
				{
					*Texture = TextureLoad(TextureFiles[TextureIndex]);
					OpenGLAllocateTexture(Texture);
					Texture++;
				}

				shader Shaders[3];
				for(u32 ShaderIndex = 0; ShaderIndex < ArrayCount(Shaders); ++ShaderIndex)
				{
					Shaders[ShaderIndex] = ShaderLoad(ShaderFiles[2 * ShaderIndex], ShaderFiles[2 * ShaderIndex + 1]);
					Shaders[ShaderIndex].Handle = GLProgramCreate(Shaders[ShaderIndex].VS, Shaders[ShaderIndex].FS);
				}

				//
				// NOTE(Justin): Models
				//

				GameState->XBot = ModelLoad(Arena, "XBot.mesh");
				model *XBot = &GameState->XBot;
				XBot->Animations.Count = ArrayCount(XBotAnimations);
				XBot->Animations.Info = PushArray(Arena, XBot->Animations.Count, animation_info);

				GameState->Sphere = ModelLoad(Arena, "Sphere.mesh");
				model *Sphere = &GameState->Sphere;

				GameState->Cube = ModelLoad(Arena, "Cube.mesh");
				model *Cube = &GameState->Cube;
				Cube->Meshes[0].DiffuseTexture = GameState->Textures[2].Handle;
				Cube->Meshes[0].NormalTexture = GameState->Textures[3].Handle;
				Cube->Meshes[0].MaterialFlags = (MaterialFlag_Diffuse | MaterialFlag_Normal);

				GameState->Vampire = ModelLoad(Arena, "VampireALusth.mesh");
				model *Vampire = &GameState->Vampire;
				Vampire->Animations.Count = ArrayCount(VampireAnimations);
				Vampire->Animations.Info = PushArray(Arena, Vampire->Animations.Count, animation_info);

				Vampire->Meshes[0].DiffuseTexture = GameState->Textures[4].Handle;
				Vampire->Meshes[0].SpecularTexture = GameState->Textures[5].Handle;
				Vampire->Meshes[0].NormalTexture = GameState->Textures[8].Handle;
				Vampire->Meshes[0].MaterialFlags = (MaterialFlag_Diffuse | MaterialFlag_Specular);

				GameState->Paladin = ModelLoad(Arena, "PaladinWithProp.mesh");
				model *Paladin = &GameState->Paladin;
				Paladin->Animations.Count = ArrayCount(PaladinAnimations);
				Paladin->Animations.Info = PushArray(Arena, Paladin->Animations.Count, animation_info);
				for(u32 MeshIndex = 0; MeshIndex < Paladin->MeshCount; ++MeshIndex)
				{
					mesh *Mesh = Paladin->Meshes + MeshIndex;
					Mesh->DiffuseTexture = GameState->Textures[9].Handle;
					Mesh->SpecularTexture = GameState->Textures[10].Handle;
					Mesh->NormalTexture = GameState->Textures[11].Handle;
					Mesh->MaterialFlags = (MaterialFlag_Diffuse | MaterialFlag_Specular);
				}

				//
				// NOTE(Justin): Animations
				//

				for(u32 AnimIndex = 0; AnimIndex < ArrayCount(XBotAnimations); ++AnimIndex)
				{
					char *FileName = XBotAnimations[AnimIndex];
					animation_info TestAnimation = AnimationLoad(Arena, FileName);
					animation_info *Info = XBot->Animations.Info + AnimIndex;
					*Info = TestAnimation;
				}
				for(u32 AnimIndex = 0; AnimIndex < ArrayCount(VampireAnimations); ++AnimIndex)
				{
					char *FileName = VampireAnimations[AnimIndex];
					animation_info TestAnimation = AnimationLoad(Arena, FileName);
					animation_info *Info = Vampire->Animations.Info + AnimIndex;
					*Info = TestAnimation;
				}
				for(u32 AnimIndex = 0; AnimIndex < ArrayCount(PaladinAnimations); ++AnimIndex)
				{
					char *FileName = PaladinAnimations[AnimIndex];
					animation_info TestAnimation = AnimationLoad(Arena, FileName);
					animation_info *Info = Paladin->Animations.Info + AnimIndex;
					*Info = TestAnimation;
				}

				OpenGLAllocateAnimatedModel(XBot, Shaders[0].Handle);
				OpenGLAllocateAnimatedModel(Vampire, Shaders[0].Handle);
				OpenGLAllocateAnimatedModel(Paladin, Shaders[0].Handle);
				OpenGLAllocateModel(Sphere, Shaders[1].Handle);
				OpenGLAllocateModel(Cube, Shaders[1].Handle);

				quad Quad = QuadDefault();
				Quad.DiffuseTexture = GameState->Textures[2].Handle;
				Quad.NormalTexture = GameState->Textures[3].Handle;
				OpenGLAllocateQuad(&Quad, Shaders[2].Handle);

				PlayerAdd(GameState);
				VampireAdd(GameState);
				LightAdd(GameState);
				PaladinAdd(GameState);
				BlockAdd(GameState);

				f32 StartTime = 0.0f;
				f32 EndTime = 0.0f;
				f32 DtForFrame = 0.0f;
				f32 Angle = 0.0f;
				glfwSetTime(0.0);

				//
				// NOTE(Justin): Set all the uniforms that do not change during runtime.
				//

				glUseProgram(Shaders[0].Handle);
				UniformMatrixSet(Shaders[0].Handle, "View", CameraTransform);
				UniformMatrixSet(Shaders[0].Handle, "Projection", PerspectiveTransform);

				glUseProgram(Shaders[1].Handle);
				UniformMatrixSet(Shaders[1].Handle, "View", CameraTransform);
				UniformMatrixSet(Shaders[1].Handle, "Projection", PerspectiveTransform);

				glUseProgram(Shaders[2].Handle);
				UniformMatrixSet(Shaders[2].Handle, "View", CameraTransform);
				UniformMatrixSet(Shaders[2].Handle, "Projection", PerspectiveTransform);

				v3 LightColor = V3(1.0f);
				v3 Ambient = V3(0.1f);
				while(!glfwWindowShouldClose(Window.Handle))
				{
					Angle += DtForFrame;

					v3 LightP = V3(0.0f);
					for(u32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex)
					{
						entity *Entity = GameState->Entities + EntityIndex;
						switch(Entity->Type)
						{
							case EntityType_Player:
							{
							} break;
							case EntityType_Vampire:
							{
							} break;
							case EntityType_Light:
							{
								v3 *P = &Entity->P;
								P->x = 20.0f * cosf((Angle));
								LightP = *P;

							} break;
							case EntityType_Paladin:
							{
							} break;
							case EntityType_Block:
							{
							} break;
						}
					}

					//
					// NOTE(Justin): Render.
					//

					glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					for(u32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex)
					{
						entity *Entity = GameState->Entities + EntityIndex;
						switch(Entity->Type)
						{
							case EntityType_Player:
							{
								glUseProgram(Shaders[0].Handle);
								UniformV3Set(Shaders[0].Handle, "Ambient", Ambient);
								UniformV3Set(Shaders[0].Handle, "LightPosition", LightP);
								UniformV3Set(Shaders[0].Handle, "CameraPosition", CameraP);
								UniformV3Set(Shaders[0].Handle, "LightColor", LightColor);
								mat4 Transform = EntityTransform(Entity, 0.1f);
								AnimationUpdate(XBot, DtForFrame);
								OpenGLDrawAnimatedModel(XBot, Shaders[0].Handle, Transform);
							} break;
							case EntityType_Vampire:
							{
								glUseProgram(Shaders[0].Handle);
								UniformV3Set(Shaders[0].Handle, "Ambient", Ambient);
								UniformV3Set(Shaders[0].Handle, "LightPosition", LightP);
								UniformV3Set(Shaders[0].Handle, "CameraPosition", CameraP);
								UniformV3Set(Shaders[0].Handle, "LightColor", LightColor);
								mat4 Transform = EntityTransform(Entity, 0.1f);
								AnimationUpdate(Vampire, DtForFrame);
								OpenGLDrawAnimatedModel(Vampire, Shaders[0].Handle, Transform);
							} break;
							case EntityType_Paladin:
							{
								glUseProgram(Shaders[0].Handle);
								UniformV3Set(Shaders[0].Handle, "Ambient", Ambient);
								UniformV3Set(Shaders[0].Handle, "LightPosition", LightP);
								UniformV3Set(Shaders[0].Handle, "CameraPosition", CameraP);
								UniformV3Set(Shaders[0].Handle, "LightColor", LightColor);
								mat4 Transform = EntityTransform(Entity, 0.1f);
								AnimationUpdate(Paladin, DtForFrame);
								OpenGLDrawAnimatedModel(Paladin, Shaders[0].Handle, Transform);
							} break;
							case EntityType_Light:
							{
								glUseProgram(Shaders[1].Handle);
								UniformV3Set(Shaders[1].Handle, "Diffuse", V3(1.0f));
								mat4 Transform = EntityTransform(Entity);
								OpenGLDrawModel(Sphere, Shaders[1].Handle, Transform);
							} break;
							case EntityType_Block:
							{
								mat4 Transform = EntityTransform(Entity, 3.0f);
								glUseProgram(Shaders[1].Handle);
								UniformV3Set(Shaders[1].Handle, "LightP", LightP);
								//OpenGLDrawModel(Cube, Shaders[1].Handle, Transform);
							} break;
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
					ArenaInitialize(&Arena_, (u8 *)Memory, Megabyte(1024));
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
