
/*

 TODO(Justin):
 [] Change children array from fixed size to allocate on demand
 [] Clean up logic
 [] Clean up parsing
 [] Handle more complicated meshes
 [] Handle meshes with materials
 [] Init mesh using recursion so only have to traverse tree once? (bad idea??)
 [] Skeletal animation transforms
 [] Remove crt functions
 [] Write collada file
 [] Do we need dynamix buffer sizes?
 [] For models we know we only need to read do we parse the entire file?


*/

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define internal static
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#define ArrayCount(A) sizeof(A) / sizeof((A)[0])
#define Kilobyte(Count) (1024 * Count)
#define Megabyte(Count) (1024 * Kilobyte(Count))

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

struct string
{
	u8 *Data;
	u64 Size;
};

struct xml_attribute
{
	string Key;
	string Value;
};

struct xml_node
{
	string Tag;
	string InnerText;

	s32 AttributeCountMax;
	s32 AttributeCount;
	xml_attribute *Attributes;

	xml_node *Parent;

	s32 ChildrenMaxCount;
	s32 ChildrenCount;
	xml_node **Children;
};

struct loaded_dae 
{
	char *FullPath;
	xml_node *Root;
};


internal xml_node *
PushXMLNode(memory_arena *Arena, xml_node *Parent)
{
	xml_node *Node = PushStruct(Arena, xml_node);

	Node->AttributeCountMax = COLLADA_ATTRIBUTE_MAX_COUNT;

	Node->Attributes = PushArray(Arena, Node->AttributeCountMax, xml_attribute);//(xml_attribute *)calloc(Node->AttributeCountMax, sizeof(xml_attribute));
	Node->ChildrenMaxCount = COLLADA_NODE_CHILDREN_MAX_COUNT;

	Node->Children = PushArray(Arena, Node->ChildrenMaxCount, xml_node *);
	Node->Parent = Parent;

	return(Node);
}

internal b32
StringsAreSame(char *S1, char *S2)
{
	b32 Result = (strcmp(S1, S2) == 0);
	return(Result);
}

internal b32
StringsAreSame(string S1, char *S2)
{
	b32 Result = StringsAreSame((char *)S1.Data, S2);
	return(Result);
}

struct mesh
{
	f32 *Positions;
	f32 *Normals;
	f32 *UV;
	u32 *Indices;

	u32 PositionsCount;
	u32 NormalsCount;
	u32 UVCount;
	u32 IndicesCount;
};

internal b32
SubStringExists(char *HayStack, char *Needle)
{
	b32 Result = false;
	char *S = strstr(HayStack, Needle);
	if(S)
	{
		Result = true;
	}

	return(Result);
}

internal b32
SubStringExists(string HayStack, char *Needle)
{
	b32 Result = false;
	char *S = strstr((char *)HayStack.Data, Needle);
	if(S)
	{
		Result = true;
	}

	return(Result);
}

internal b32
StringEndsWith(string S, char C)
{
	u8 *Test = S.Data + (S.Size - 1);
	b32 Result = (*Test == (u8 )C);

	return(Result);
}

internal void
ParseF32Array(f32 *Dest, u32 DestCount, string Data)
{
	char *Scan = (char *)Data.Data;
	for(u32 Index = 0; Index < DestCount; ++Index)
	{
		Dest[Index] = (f32)atof(Scan);
		while(*Scan != ' ' && *(Scan + 1) != '\0')
		{
			Scan++;
		}

		if(*(Scan + 1) != '\0')
		{
			Scan++;
		}
	}
}

internal b32
IsNumber(char C)
{
	b32 Result = ((C >= '0') && (C <= '9'));
	return(Result);
}


internal void
ParseS32Array(u32 *Dest, u32 DestCount, string Data)
{
	char *Scan = (char *)Data.Data;
	for(u32 Index = 0; Index < DestCount; ++Index)
	{
		Dest[Index] = (u32)atoi(Scan);
		while(*Scan != ' ' && *(Scan + 1) != '\0')
		{
			Scan++;
		}

		if(*(Scan + 1) != '\0')
		{
			Scan++;
		}
	}
}

internal s32
S32FromASCII(u8 *S)
{
	s32 Result = atoi((char *)S);
	return(Result);
}

internal xml_attribute
NodeAttributeGet(xml_node *Node, char *AttrName)
{
	xml_attribute Result = {};
	for(s32 Index = 0; Index < Node->AttributeCount; ++Index)
	{
		xml_attribute *Attr = Node->Attributes + Index;
		if(StringsAreSame(Attr->Key, AttrName)) 
		{
			Result = *Attr;
		}
	}

	return(Result);
}


// NOTE(Justin): Something very bad is happening when parsing using this routine

#if 0
internal void 
MeshInit(xml_node *Root, mesh *Mesh)
{
	for(s32 Index = 0; Index < Root->ChildrenCount; ++Index)
	{
		xml_node *Node = Root->Children[Index];
		if(Node)
		{
			if(StringsAreSame(Node->Tag, "float_array"))
			{
				if(SubStringExists(Node->Attributes->Value, "mesh-positions-array"))
				{
					xml_attribute Attr = NodeAttributeGet(Node, "count");

					Mesh->PositionsCount = S32FromASCII(Attr.Value.Data);
					Mesh->Positions = (f32 *)calloc(Mesh->PositionsCount, sizeof(f32));
					ParseF32Array(Mesh->Positions, Mesh->PositionsCount, Node->InnerText);
				}
				else if(SubStringExists(Node->Attributes->Value, "mesh-normals-array"))
				{
					xml_attribute Attr = NodeAttributeGet(Node, "count");

					Mesh->NormalsCount = S32FromASCII(Attr.Value.Data);
					Mesh->Normals = (f32 *)calloc(Mesh->PositionsCount, sizeof(f32));
					ParseF32Array(Mesh->Normals, Mesh->NormalsCount, Node->InnerText);
				}
				else if(SubStringExists(Node->Attributes->Value, "mesh-map-0-array"))
				{
					xml_attribute Attr = NodeAttributeGet(Node, "count");
					Mesh->UVCount = S32FromASCII(Attr.Value.Data);
					Mesh->UV = (f32 *)calloc(Mesh->PositionsCount, sizeof(f32));

					ParseF32Array(Mesh->UV, Mesh->UVCount, Node->InnerText);
				}
			}

			if(StringsAreSame(Node->Tag, "triangles"))
			{
				xml_attribute Attr = NodeAttributeGet(Node, "count");
				Mesh->IndicesCount = 3 * 3 * S32FromASCII(Attr.Value.Data);
				Mesh->Indices = (u32 *)calloc(Mesh->IndicesCount, sizeof(u32));
			}

			if(StringsAreSame(Node->Tag, "p"))
			{
				Assert(Mesh->IndicesCount > 0);
				Assert(Mesh->Indices);
				ParseS32Array(Mesh->Indices, Mesh->IndicesCount, Node->InnerText);
			}

			if(*Node->Children)
			{
				MeshInit(Node, Mesh);
			}
		}
	}
}
#endif



internal void
NodeGet(xml_node *Root, xml_node *N, char *TagName, char *ID = 0)
{
	for(s32 Index = 0; Index < Root->ChildrenCount; ++Index)
	{
		// NOTE(Justin): If the tag size is 0 the node has not been found yet, continue searching OW break.
		if(N->Tag.Size == 0)
		{
			xml_node *Node = Root->Children[Index];
			if(Node)
			{
				if(StringsAreSame(Node->Tag, TagName))
				{

					// NOTE(Justin): Tags either have/do not have an ID.
					// Tags without IDs essentially tell us that we have
					// reached a part of the file containing a new bucket of
					// information, so we can just return the node. OTOH if
					// a tag has an ID then we are in a bucket that contains
					// specific information and we use the ID to ensure we are
					// at the node that contains the info. we are looking for.

					if(ID)
					{
						if(SubStringExists(Node->Attributes->Value, ID))
						{
							*N = *Node;
						}
					}
					else
					{
						*N = *Node;
					}
				}
				else if(*Node->Children)
				{
					NodeGet(Node, N, TagName, ID);
				}
			}
		}
		else
		{
			break;
		}
	}
}



internal loaded_dae
ColladaFileLoad(memory_arena *Arena, char *FileName)
{
	loaded_dae Result = {};

	FILE *FileHandle = fopen(FileName, "r");
	if(FileHandle)
	{
		// NOTE(Justin): Get the size of the file
		fseek(FileHandle, 0, SEEK_END);
		s32 Size = ftell(FileHandle);
		fseek(FileHandle, 0, SEEK_SET);


		u8 *Content = (u8 *)calloc(Size + 1, sizeof(u8));
		fread(Content, 1, Size, FileHandle);
		Content[Size] = '\0';

		char TagStart = '<';
		char TagEnd = '>';
		//char ForwardSlash = '/';

		char *Buffer = (char *)calloc(Size/4, sizeof(char));
		s32 InnerTextIndex = 0;
		s32 Index = 0;

		Result.Root = PushXMLNode(Arena, 0);
		xml_node *CurrentNode = Result.Root;

		while(!SubStringExists(Buffer, "COLLADA"))
		{	
			Buffer[InnerTextIndex++] = Content[Index++];
			Buffer[InnerTextIndex] = '\0';
		}
		InnerTextIndex = 0;
		Index -= ((s32)strlen("COLLADA") + 1);

		while(Content[Index] != '\0')
		{
			if(!CurrentNode->Tag.Data)
			{
				Index++;
				while(Content[Index] != TagEnd)
				{
					Buffer[InnerTextIndex++] = Content[Index++];

					if((Content[Index] == TagEnd))
					{
						Buffer[InnerTextIndex] = '\0';
						CurrentNode->Tag.Data = (u8 *)strdup(Buffer);
						CurrentNode->Tag.Size = InnerTextIndex;
						InnerTextIndex = 0;
					}

					if(((Content[Index] == ' ') || (Content[Index + 1] == TagEnd)) && (!CurrentNode->Tag.Data))
					{
						// NOTE(Justin): Two types of tags. Tag with key/values
						// and tags without key/values. So, need to handle both.
						if(Content[Index] != ' ')
						{
							Buffer[InnerTextIndex++] = Content[Index];
						}

						Buffer[InnerTextIndex] = '\0';
						CurrentNode->Tag.Data = (u8 *)strdup(Buffer);
						CurrentNode->Tag.Size = InnerTextIndex;
						InnerTextIndex = 0;
						Index++;
					}

					if((Content[Index] == '='))
					{
						// NOTE(Justin): Buffered key so copy to current attribute
						xml_attribute *Attribute = CurrentNode->Attributes + CurrentNode->AttributeCount;
						Buffer[InnerTextIndex] = '\0';
						Attribute->Key.Data = (u8 *)strdup(Buffer);
						Attribute->Key.Size = InnerTextIndex;
						InnerTextIndex = 0;
						Index++;
					}

					if((Content[Index] == '"'))
					{
						// NOTE(Jusitn): At start of value, buffer & copy to current attribute
						// and increment attribute count
						Index++;
						while(Content[Index] != '"')
						{
							Buffer[InnerTextIndex++] = Content[Index++];
						}
						Buffer[InnerTextIndex] = '\0';

						xml_attribute *Attribute = CurrentNode->Attributes + CurrentNode->AttributeCount;
						Assert(Attribute->Key.Data);
						Attribute->Value.Data = (u8 *)strdup(Buffer);
						Attribute->Value.Size = InnerTextIndex;
						CurrentNode->AttributeCount++;
						Assert(CurrentNode->AttributeCount < COLLADA_ATTRIBUTE_MAX_COUNT);

						InnerTextIndex = 0;
						if(Content[Index + 1] == ' ')
						{
							// NOTE(Justin): Advance index to start buffering next key
							Index += 2;
						}
						else
						{
							// NOTE(Justin): Advance index, next char is either '>' or '/'
							Index++;
						}

						if(Content[Index] == '/')
						{
							// NOTE(Justin): Single tag node ("empty node") with no closing tag
							Index++;
							CurrentNode = CurrentNode->Parent;
						}
					}
				}

				// NOTE(Justin): Done processing tag. Buffer & copy inner text.
				Index++;
				InnerTextIndex = 0;
				while(Content[Index] != TagStart)
				{
					Buffer[InnerTextIndex++] = Content[Index++];
				}
				Buffer[InnerTextIndex] = '\0';
				CurrentNode->InnerText.Data = (u8 *)strdup(Buffer);
				CurrentNode->InnerText.Size = InnerTextIndex;
				InnerTextIndex = 0;
			}
			else
			{
				if(StringEndsWith(CurrentNode->Tag, '/'))
				{
					CurrentNode = CurrentNode->Parent;
				}

				if((Content[Index] == '<') && (Content[Index + 1] == '/'))
				{
					// NOTE(Justin): Closing tag and end of the current node. Advance the index to one past '>'.
					Index += 2;
					while(Content[Index] != '>')
					{
						Index++;
					}
					Index++;

					if(CurrentNode->Parent)
					{
						CurrentNode = CurrentNode->Parent;
					}
					else
					{
						break;
					}
				}

				else if(Content[Index] == '<')
				{
					// NOTE(Justin): Child node exists
					xml_node *LastChild = PushXMLNode(Arena, CurrentNode);
					CurrentNode->Children[CurrentNode->ChildrenCount] = LastChild;
					CurrentNode->ChildrenCount++;

					// TODO(Justin): On-demand allocation of children nodes (currently capped).
					Assert(CurrentNode->ChildrenCount < COLLADA_NODE_CHILDREN_MAX_COUNT);
					CurrentNode = LastChild;
				}
				else
				{
					// Note(Justin): At the end of a child node and another child node exists.
					while(Content[Index] != TagStart)
					{
						Buffer[InnerTextIndex++] = Content[Index++];
					}
					Buffer[InnerTextIndex] = '\0';
					InnerTextIndex = 0;
				}
			}
		}

		fclose(FileHandle);
	}


	return(Result);
}

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
OpenGLDebugCallback(GLenum Source, GLenum Type, GLuint ID, GLenum Severity, GLsizei Length,
		const GLchar *Message, const void *UserParam)
{
	printf("OpenGL Debug Callback: %s\n", Message);
}

#if 1
internal mesh
MeshInit(loaded_dae DaeFile)
{
	mesh Mesh = {};

	xml_node *Root = DaeFile.Root;

	xml_node Geometry;
	xml_node NodePos;
	xml_node NodeNormal;
	xml_node NodeUV;
	xml_node NodeIndex;

	Geometry.Tag.Size = 0; 
	NodePos.Tag.Size = 0;
	NodeNormal.Tag.Size = 0;
	NodeUV.Tag.Size = 0;
	NodeIndex.Tag.Size = 0;

	NodeGet(Root, &Geometry, "library_geometries");
	NodeGet(&Geometry, &NodePos, "float_array", "mesh-positions-array");
	NodeGet(&Geometry, &NodeNormal, "float_array", "mesh-normals-array");
	NodeGet(&Geometry, &NodeUV, "float_array", "mesh-map-0-array");
	NodeGet(&Geometry, &NodeIndex, "p");

	xml_attribute AttrP = NodeAttributeGet(&NodePos, "count");
	Mesh.PositionsCount = S32FromASCII(AttrP.Value.Data);

	xml_attribute AttrN = NodeAttributeGet(&NodeNormal, "count");
	Mesh.NormalsCount = S32FromASCII(AttrN.Value.Data);

	xml_attribute AttrUV = NodeAttributeGet(&NodeUV, "count");
	Mesh.UVCount = S32FromASCII(AttrUV.Value.Data);

	xml_attribute AttrI = NodeAttributeGet(NodeIndex.Parent, "count");
	Mesh.IndicesCount = 3 * 3 * S32FromASCII(AttrI.Value.Data);

	Mesh.Positions = (f32 *)calloc(Mesh.PositionsCount, sizeof(f32));
	Mesh.Normals = (f32 *)calloc(Mesh.NormalsCount, sizeof(f32));
	Mesh.UV = (f32 *)calloc(Mesh.UVCount, sizeof(f32));
	Mesh.Indices = (u32 *)calloc(Mesh.IndicesCount, sizeof(u32));

	ParseF32Array(Mesh.Positions, Mesh.PositionsCount, NodePos.InnerText);
	ParseF32Array(Mesh.Normals, Mesh.NormalsCount, NodeNormal.InnerText);
	ParseF32Array(Mesh.UV, Mesh.UVCount, NodeUV.InnerText);
	ParseS32Array(Mesh.Indices, Mesh.IndicesCount, NodeIndex.InnerText);

	return(Mesh);
}
#endif

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
			//glfwSetCursorPosCallback(Window.Handle, glfw_mouse_callback);
			//glfwSetMouseButtonCallback(Window.Handle, glfw_mouse_button_callback);
			if(glewInit() == GLEW_OK)
			{
				const GLubyte *RendererName = glGetString(GL_RENDERER);
				const GLubyte* RendererVersion = glGetString(GL_VERSION);
				printf("Renderer = %s\n", RendererName);
				printf("Renderer version = %s\n", RendererVersion);

				glEnable(GL_DEBUG_OUTPUT);
				glDebugMessageCallback(OpenGLDebugCallback, 0);

				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				loaded_dae CubeDae = ColladaFileLoad(Arena, "untitled.dae");
				mesh Cube = MeshInit(CubeDae);

				char *VertexShaderSrc = R"(
				#version 330 core
				layout (location = 0) in vec3 P;
				void main()
				{
					gl_Position = vec4(P.x, P.y, P.z, 1.0f);
				})";

				char *FragmentShaderSrc = R"(
				#version 330 core
				out vec4 FragColor;
				void main()
				{
					FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
				})";

				u32 VertexShaderHandle;
				u32 FragmentShaderHandle;

				VertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
				FragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);

				glShaderSource(VertexShaderHandle, 1, &VertexShaderSrc, 0);
				glShaderSource(FragmentShaderHandle, 1, &FragmentShaderSrc, 0);

				b32 VertexShaderIsValid = false;
				b32 FragmentShaderIsValid = false;

				char Buffer[512];

				glCompileShader(VertexShaderHandle);
				glGetShaderiv(VertexShaderHandle, GL_COMPILE_STATUS, &VertexShaderIsValid);
				if(!VertexShaderIsValid)
				{
					glGetShaderInfoLog(VertexShaderHandle, 512, 0, Buffer);
					printf("ERROR: Vertex Shader Compile Failed\n %s", Buffer);
					return(-1);
				}

				glCompileShader(FragmentShaderHandle);
				glGetShaderiv(FragmentShaderHandle, GL_COMPILE_STATUS, &FragmentShaderIsValid);
				if(!FragmentShaderIsValid)
				{
					glGetShaderInfoLog(FragmentShaderHandle, 512, 0, Buffer);
					printf("ERROR: Fragment Shader Compile Failed\n %s", Buffer);
					return(-1);
				}

				u32 ShaderProgram;
				ShaderProgram = glCreateProgram();
				glAttachShader(ShaderProgram, VertexShaderHandle);
				glAttachShader(ShaderProgram, FragmentShaderHandle);
				glLinkProgram(ShaderProgram);

				b32 ProgramIsValid = false;
				glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &ProgramIsValid);
				if(!ProgramIsValid)
				{
					glGetProgramInfoLog(ShaderProgram, 512, 0, Buffer);
					printf("ERROR: Program link failed\n %s", Buffer);
					return(-1);
				}

				glDeleteShader(VertexShaderHandle);
				glDeleteShader(FragmentShaderHandle);

				f32 Positions[] = {
					-0.5f, -0.5f, 0.0f,
					0.5f, -0.5f, 0.0f,
					0.0f,  0.5f, 0.0f
				}; 

				u32 VBO, VAO;
				glGenVertexArrays(1, &VAO);
				glGenBuffers(1, &VBO);
				glBindVertexArray(VAO);

				glBindBuffer(GL_ARRAY_BUFFER, VBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(Positions), Positions, GL_STATIC_DRAW);

				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (void *)0);
				glEnableVertexAttribArray(0);

				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				while(!glfwWindowShouldClose(Window.Handle))
				{
					glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT);

					glUseProgram(ShaderProgram);
					glBindVertexArray(VAO);
					glDrawArrays(GL_TRIANGLES, 0, 3);

					glfwSwapBuffers(Window.Handle);
					glfwPollEvents();
				}
			}
		}
	}

	return(0);
}
