
/*
 TODO(Justin):
 [] Use temporary memory when loading models
 [?] Separate skeleton/armature from each mesh. A mesh may be affected by a subset of the entire
	skeleton. We can store the entire skeleton at the model level and update the skeleton each frame
	then the joint transforms / model transforms of each mesh need to be updated.
 [] Fix simple textured quads
 [] opengl error logging/checking!  
 [?] fonts 
 [?] imgui 
 [?] asset system  
 [?] asset file structure 
 [?] Add tangents and BiTangents to asset mesh file
 [] Average tangents and BiTangents when model loading 
 [X] Transform normals via joint transforms!  
 [X] Get the correct normal when using a normal map AND doing skeletal animation
 [] File paths from build directory
 [X] Simple animation file format 
 [X] Test different models.
	 [] Test different models++.
 [X] Instead of storing mat4's for animation, store pos, quat, scale and construct for both key frame and model
 [X] Check UV mapping 
 [X] Phong lighting 
 [?] Diffuse, ambient, specular, and normal textures in file format 
 [] WARNING using String() on a token is bad and can easily result in an access violation. REMOVE THIS
 [?] Remove strtok_s
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

#define CString(String) (char *)String.Data

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
#include "texture.h"
#include "xml.h"
#include "animation.h"
#include "mesh.h"
#include "asset.h"
#include "entity.h"
#include "strings.cpp"
#include "file_io.cpp"
#include "texture.cpp"
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
	model Ninja;
	model Arissa;
	model Maria;
	model Vanguard;
	model Brute;
	model Mannequin;
	model ErikaArcher;
	model BulkyKnight;

	model Models[16];

	model DebugArrow;

	texture Textures[64];
};

enum buttons
{
	Button_W,
	Button_A,
	Button_S,
	Button_D,

	Button_Count
};

struct game_button_state
{
	b32 EndedDown;
};

struct game_input
{
	union
	{
		game_button_state Buttons[Button_Count];
		struct
		{
			game_button_state W;
			game_button_state A;
			game_button_state S;
			game_button_state D;
		};
	};
};

static game_input GameInput[2];

inline b32
IsDown(game_button_state Button)
{
	b32 Result = Button.EndedDown;
	return(Result);
}

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

internal void
AllocateModelTextures(model *Model)
{
	for(u32 TextureIndex = 0; TextureIndex < Model->TextureCount; ++TextureIndex)
	{
		texture *Texture = Model->Textures + TextureIndex;
		OpenGLAllocateTexture(Texture);
	}
}

int main(int Argc, char **Argv)
{
	void *Memory = calloc(Megabyte(1024), sizeof(u8));
	memory_arena Arena_;
	ArenaInitialize(&Arena_, (u8 *)Memory, Megabyte(1024));
	memory_arena *Arena = &Arena_;

	glfwSetErrorCallback(GLFWErrorCallback);
	if(!glfwInit())
	{
		printf("ERROR: GLFW failed to initialize\n");
		return(0);
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

	window Window = {};
	Window.Width = 960;
	Window.Height = 540;

	Window.Handle = glfwCreateWindow(Window.Width, Window.Height, "YabbaColladaDoo", NULL, NULL);
	if(!Window.Handle)
	{
		printf("ERROR: GLFW failed to create a window\n");
		return(0);
	}

	glfwMakeContextCurrent(Window.Handle);
	glfwSetInputMode(Window.Handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	glfwSetKeyCallback(Window.Handle, GLFWKeyCallBack);
	glfwSetFramebufferSizeCallback(Window.Handle, GLFWFrameBufferResizeCallBack);
	glewExperimental = GL_TRUE;
	if(glewInit() != GLEW_OK)
	{
		printf("ERROR: GLEW failed to initialize\n");
		return(0);
	}
				//
				// NOTE(Justin): Transformations
				//

	v3 CameraP = V3(0.0f, 10.0f, 100.0f);
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

	char *ShaderFiles[] =
	{
		"shaders\\main.vs",
		"shaders\\main.fs",
		"shaders\\basic.vs",
		"shaders\\basic.fs",
		"shaders\\quad.vs",
		"shaders\\quad.fs",
	};

	char *XBotAnimations[] =
	{
		"models\\XBot\\animationsXBot_ActionIdle.animation",
	};
	char *PaladinAnimations[] =
	{
		"models\\Paladin\\animations\\Paladin_SwordAndShieldCasting.animation",
	};
	char *ArcherAnimations[] =
	{
		"models\\ErikaArcher\\animations\\Archer_StandingAimRecoil.animation",
	};
	char *BruteAnimations[] =
	{
		"models\\Brute\\animations\\Brute_Idle.animation",
	};
	char *MariaAnimations[] =
	{
		"models\\Maria\\animations\\Maria_Idle.animation",
	};

	game_state GameState_ = {};
	game_state *GameState = &GameState_;

	shader Shaders[3];
	for(u32 ShaderIndex = 0; ShaderIndex < ArrayCount(Shaders); ++ShaderIndex)
	{
		Shaders[ShaderIndex] = ShaderLoad(ShaderFiles[2 * ShaderIndex], ShaderFiles[2 * ShaderIndex + 1]);
		Shaders[ShaderIndex].Handle = GLProgramCreate(Shaders[ShaderIndex].VS, Shaders[ShaderIndex].FS);
	}

	//
	// NOTE(Justin): Models
	//

	GameState->Sphere = ModelLoad(Arena, "models\\Sphere.mesh");

	GameState->Cube = ModelLoad(Arena, "models\\Cube\\Cube.mesh");
	GameState->Cube.Textures[0] = TextureLoad(Arena, "textures\\orange_texture_02.png");
	GameState->Cube.Meshes[0].DiffuseTexture = 0;
	GameState->Cube.Meshes[0].MaterialFlags = MaterialFlag_Diffuse;

	GameState->XBot = ModelLoad(Arena, "models\\XBot\\XBot.mesh");
	model *XBot = &GameState->XBot;
	XBot->Animations.Count = ArrayCount(XBotAnimations);
	XBot->Animations.Info = PushArray(Arena, XBot->Animations.Count, animation_info);
	for(u32 AnimIndex = 0; AnimIndex < ArrayCount(XBotAnimations); ++AnimIndex)
	{
		char *FileName = XBotAnimations[AnimIndex];
		animation_info TestAnimation = AnimationLoad(Arena, FileName);
		animation_info *Info = XBot->Animations.Info + AnimIndex;
		*Info = TestAnimation;
	}

#if 0
	// Collada Initialization

	GameState->Paladin = ModelLoadFromCollada(Arena, "models\\Paladin\\PaladinWithProp.dae");
	model *Paladin = &GameState->Paladin;
#else
	// Custom Initialization

	GameState->Paladin = ModelLoad(Arena, "models\\Paladin\\PaladinWithProp.mesh");
	model *Paladin = &GameState->Paladin;

	Paladin->TextureCount = 3;
	Paladin->Textures[0] = TextureLoad(Arena, "models\\Paladin\\textures\\Paladin_diffuse.png");
	Paladin->Textures[1] = TextureLoad(Arena, "models\\Paladin\\textures\\Paladin_specular.png");
	Paladin->Textures[2] = TextureLoad(Arena, "models\\Paladin\\textures\\Paladin_normal.png");
	for(u32 MeshIndex = 0; MeshIndex < Paladin->MeshCount; ++MeshIndex)
	{
		mesh *Mesh = Paladin->Meshes + MeshIndex;
		Mesh->DiffuseTexture = 0;
		Mesh->SpecularTexture = 1;
		Mesh->NormalTexture = 2;
		Mesh->MaterialFlags = (MaterialFlag_Diffuse | MaterialFlag_Specular | MaterialFlag_Normal);
	}
#endif
	Paladin->Animations.Count = ArrayCount(PaladinAnimations);
	Paladin->Animations.Info = PushArray(Arena, Paladin->Animations.Count, animation_info);
	for(u32 AnimIndex = 0; AnimIndex < ArrayCount(PaladinAnimations); ++AnimIndex)
	{
		char *FileName = PaladinAnimations[AnimIndex];
		animation_info TestAnimation = AnimationLoad(Arena, FileName);
		animation_info *Info = Paladin->Animations.Info + AnimIndex;
		*Info = TestAnimation;
	}


#if 0
	GameState->Maria = ModelLoadFromCollada(Arena, "models\\Maria\\MariaSword.dae");
	model *Maria = &GameState->Maria;
#else
	GameState->Maria = ModelLoad(Arena, "models\\Maria\\MariaSword.mesh");
	model *Maria = &GameState->Maria;

	Maria->TextureCount = 3;
	Maria->Textures[0] = TextureLoad(Arena, "models\\Maria\\textures\\maria_diffuse.png");
	Maria->Textures[1] = TextureLoad(Arena, "models\\Maria\\textures\\maria_specular.png");
	Maria->Textures[2] = TextureLoad(Arena, "models\\Maria\\textures\\maria_normal.png");
	for(u32 MeshIndex = 0; MeshIndex < Maria->MeshCount; ++MeshIndex)
	{
		mesh *Mesh = Maria->Meshes + MeshIndex;
		Mesh->DiffuseTexture = 0;
		Mesh->SpecularTexture = 1;
		Mesh->NormalTexture = 2;
		Mesh->MaterialFlags = (MaterialFlag_Diffuse | MaterialFlag_Specular | MaterialFlag_Normal);
	}
#endif
	Maria->Animations.Count = ArrayCount(MariaAnimations);
	Maria->Animations.Info = PushArray(Arena, Maria->Animations.Count, animation_info);
	for(u32 AnimIndex = 0; AnimIndex < ArrayCount(MariaAnimations); ++AnimIndex)
	{
		char *FileName = MariaAnimations[AnimIndex];
		animation_info TestAnimation = AnimationLoad(Arena, FileName);
		animation_info *Info = Maria->Animations.Info + AnimIndex;
		*Info = TestAnimation;
	}

	GameState->ErikaArcher = ModelLoadFromCollada(Arena, "models\\ErikaArcher\\Erika_ArcherWithBowArrow.dae");
	model *ErikaArcher = &GameState->ErikaArcher;
	ErikaArcher->Animations.Count = ArrayCount(ArcherAnimations);
	ErikaArcher->Animations.Info = PushArray(Arena, ErikaArcher->Animations.Count, animation_info);

	for(u32 AnimIndex = 0; AnimIndex < ArrayCount(ArcherAnimations); ++AnimIndex)
	{
		char *FileName = ArcherAnimations[AnimIndex];
		animation_info TestAnimation = AnimationLoad(Arena, FileName);
		animation_info *Info = GameState->ErikaArcher.Animations.Info + AnimIndex;
		*Info = TestAnimation;
	}

	GameState->Brute = ModelLoadFromCollada(Arena, "models\\Brute\\Brute.dae");
	model *Brute = &GameState->Brute;
	Brute->Animations.Count = ArrayCount(BruteAnimations);
	Brute->Animations.Info = PushArray(Arena, Brute->Animations.Count, animation_info);
	animation_info *BruteAnimation = Brute->Animations.Info;
	*BruteAnimation  = AnimationLoad(Arena, BruteAnimations[0]);

	//
	// GPU allocations
	//

	OpenGLAllocateAnimatedModel(&GameState->XBot, Shaders[0].Handle);
	AllocateModelTextures(&GameState->XBot);

	OpenGLAllocateAnimatedModel(&GameState->Paladin, Shaders[0].Handle);
	AllocateModelTextures(&GameState->Paladin);

	OpenGLAllocateAnimatedModel(&GameState->Vampire, Shaders[0].Handle);
	AllocateModelTextures(&GameState->Vampire);

	OpenGLAllocateAnimatedModel(&GameState->Maria, Shaders[0].Handle);
	AllocateModelTextures(&GameState->Maria);

	OpenGLAllocateAnimatedModel(&GameState->Brute, Shaders[0].Handle);
	AllocateModelTextures(&GameState->Brute);

	OpenGLAllocateAnimatedModel(&GameState->ErikaArcher, Shaders[0].Handle);
	AllocateModelTextures(&GameState->ErikaArcher);

	OpenGLAllocateAnimatedModel(&GameState->Sphere, Shaders[0].Handle);

	OpenGLAllocateModel(&GameState->Cube, Shaders[1].Handle);
	AllocateModelTextures(&GameState->Cube);

	PlayerAdd(GameState);
	VampireAdd(GameState);
	LightAdd(GameState);
	PaladinAdd(GameState);
	MariaAdd(GameState);
	BruteAdd(GameState);
	ErikaArcherAdd(GameState);

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
	UniformMatrixSet(Shaders[1].Handle, "Projection", PerspectiveTransform);
	glUseProgram(Shaders[2].Handle);
	UniformMatrixSet(Shaders[2].Handle, "Projection", PerspectiveTransform);

	v3 LightColor = V3(1.0f);
	v3 Ambient = V3(0.1f);
	while(!glfwWindowShouldClose(Window.Handle))
	{
		game_input *OldInput = &GameInput[0];
		game_input *NewInput = &GameInput[1];
		*NewInput = {};

		for(u32 ButtonIndex = 0; ButtonIndex < ArrayCount(NewInput->Buttons); ++ButtonIndex)
		{
			NewInput->Buttons[ButtonIndex].EndedDown = OldInput->Buttons[ButtonIndex].EndedDown;
		}


		glfwPollEvents();

		v3 ddPForCamera = {};
		if(IsDown(NewInput->W))
		{
			ddPForCamera = V3(0.0f, 0.0f, -1.0f);
		}
		if(IsDown(NewInput->A))
		{
			ddPForCamera = ddPForCamera + V3(-1.0f, 0.0f, 0.0f);
		}
		if(IsDown(NewInput->S))
		{
			ddPForCamera = ddPForCamera + V3(0.0f, 0.0f, 1.0f);
		}
		if(IsDown(NewInput->D))
		{
			ddPForCamera = ddPForCamera + V3(1.0f, 0.0f, 0.0f);
		}

		if(ddPForCamera.x != 0.0f || ddPForCamera.y != 0.0f || ddPForCamera.z != 0.0f)
		{
			ddPForCamera = Normalize(ddPForCamera);
		}

		CameraP = CameraP + 100.0f*DtForFrame*ddPForCamera;
		CameraTransform = Mat4Camera(CameraP, CameraP + Direction);

		Angle += DtForFrame;
		v3 LightP = V3(0.0f);
		for(u32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex)
		{
			entity *Entity = GameState->Entities + EntityIndex;
			switch(Entity->Type)
			{
				case EntityType_Light:
				{
					v3 *P = &Entity->P;
					P->x = 40.0f * cosf((Angle));
					LightP = *P;

				} break;
			}
		}

		//
		// NOTE(Justin): Render.
		//

		glUseProgram(Shaders[0].Handle);
		UniformMatrixSet(Shaders[0].Handle, "View", CameraTransform);
		glUseProgram(Shaders[1].Handle);
		UniformMatrixSet(Shaders[1].Handle, "View", CameraTransform);
		glUseProgram(Shaders[2].Handle);
		UniformMatrixSet(Shaders[2].Handle, "View", CameraTransform);

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
				case EntityType_Maria:
				{
					glUseProgram(Shaders[0].Handle);
					UniformV3Set(Shaders[0].Handle, "Ambient", Ambient);
					UniformV3Set(Shaders[0].Handle, "LightPosition", LightP);
					UniformV3Set(Shaders[0].Handle, "CameraPosition", CameraP);
					UniformV3Set(Shaders[0].Handle, "LightColor", LightColor);
					mat4 Transform = EntityTransform(Entity, 0.1f);
					AnimationUpdate(Maria, DtForFrame);
					OpenGLDrawAnimatedModel(Maria, Shaders[0].Handle, Transform);
				} break;
				case EntityType_Brute:
				{
					glUseProgram(Shaders[0].Handle);
					UniformV3Set(Shaders[0].Handle, "Ambient", Ambient);
					UniformV3Set(Shaders[0].Handle, "LightPosition", LightP);
					UniformV3Set(Shaders[0].Handle, "CameraPosition", CameraP);
					UniformV3Set(Shaders[0].Handle, "LightColor", LightColor);
					mat4 Transform = EntityTransform(Entity, 0.1f);
					AnimationUpdate(&GameState->Brute, DtForFrame);
					OpenGLDrawAnimatedModel(&GameState->Brute, Shaders[0].Handle, Transform);
				} break;
				case EntityType_Mannequin:
				{
					glUseProgram(Shaders[0].Handle);
					UniformV3Set(Shaders[0].Handle, "Ambient", Ambient);
					UniformV3Set(Shaders[0].Handle, "LightPosition", LightP);
					UniformV3Set(Shaders[0].Handle, "CameraPosition", CameraP);
					UniformV3Set(Shaders[0].Handle, "LightColor", LightColor);
					mat4 Transform = EntityTransform(Entity, 0.1f);
					OpenGLDrawAnimatedModel(&GameState->Mannequin, Shaders[0].Handle, Transform);
				} break;
				case EntityType_ErikaArcher:
				{
					glUseProgram(Shaders[0].Handle);
					UniformV3Set(Shaders[0].Handle, "Ambient", Ambient);
					UniformV3Set(Shaders[0].Handle, "LightPosition", LightP);
					UniformV3Set(Shaders[0].Handle, "CameraPosition", CameraP);
					UniformV3Set(Shaders[0].Handle, "LightColor", LightColor);
					mat4 Transform = EntityTransform(Entity, 0.1f);
					AnimationUpdate(&GameState->ErikaArcher, DtForFrame);
					OpenGLDrawAnimatedModel(&GameState->ErikaArcher, Shaders[0].Handle, Transform);
				} break;
				case EntityType_Light:
				{
					glUseProgram(Shaders[1].Handle);
					UniformV3Set(Shaders[1].Handle, "Diffuse", V3(1.0f));
					mat4 Transform = EntityTransform(Entity);
					OpenGLDrawModel(&GameState->Sphere, Shaders[1].Handle, Transform);
				} break;
				case EntityType_Block:
				{
					glUseProgram(Shaders[1].Handle);
					UniformV3Set(Shaders[1].Handle, "LightP", LightP);
					mat4 Transform = EntityTransform(Entity, 3.0f);
					OpenGLDrawModel(&GameState->Cube, Shaders[1].Handle, Transform);
				} break;
			}
		}

		glfwSwapBuffers(Window.Handle);

		EndTime = (f32)glfwGetTime();
		DtForFrame = EndTime - StartTime;
		StartTime = EndTime;

		game_input *Temp = OldInput;
		OldInput = NewInput;
		NewInput = Temp;
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
				void *Memory = calloc(Megabyte(1024), sizeof(u8));
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
