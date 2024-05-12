
/*

 NOTE(Justin): The indices in a collada file look like

	<p>i0 i1 i2 i3 i4...</p>

 and they are grouped (for cube.dae) as

	P0 N0 UV0 P1 N1 UV1...

 thus the indices that we need to pass to glDrawElements is

	P0 P1 P2 P3...

 The parsing is done in ParseS32Array.

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
 [] Do we need dynamic buffer sizes?
 [] For models we know we only need to read do we parse the entire file?
 [] Better buffering of text (strtok)
 [] Rename ParseS32Array

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

//
// NOTE(Justin): Mesh & maths 
//

union v3
{
	struct
	{
		f32 x, y, z;
	};
	f32 E[3];
};

union v4
{
	struct
	{
		f32 x, y, z, w;
	};
	struct
	{
		union
		{
			v3 xyz;
			struct
			{
				f32 x, y, z;
			};
		};
		f32 w;
	};
	f32 E[4];
};

struct mat4
{
	f32 E[4][4];
};

inline v3
V3(f32 X, f32 Y, f32 Z)
{
	v3 Result;

	Result.x = X;
	Result.y = Y;
	Result.z = Z;

	return(Result);
}

inline v4
V4(v3 V, f32 C)
{
	v4 Result = {};

	Result.xyz = V;
	Result.w = C;

	return(Result);
}

inline v3
V3(f32 C)
{
	v3 Result = V3(C, C, C);
	return(Result);
}

inline v3
XAxis()
{
	v3 Result = V3(1.0f, 0.0f, 0.0f);
	return(Result);
}

inline v3
YAxis()
{
	v3 Result = V3(0.0f, 1.0f, 0.0f);
	return(Result);
}

inline v3
ZAxis()
{
	v3 Result = V3(0.0f, 0.0f, 1.0f);
	return(Result);
}

inline v3
operator +(v3 A, v3 B)
{
	v3 Result;

	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	Result.z = A.z + B.z;

	return(Result);
}

inline v3
operator -(v3 A, v3 B)
{
	v3 Result;

	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	Result.z = A.z - B.z;

	return(Result);
}

inline v3
operator *(f32 C, v3 V)
{
	v3 Result = {};

	Result.x = C * V.x;
	Result.y = C * V.y;
	Result.z = C * V.z;

	return(Result);
}

inline v3
operator *(v3 V, f32 C)
{
	v3 Result = C * V;
	return(Result);
}

inline f32
Dot(v3 A, v3 B)
{
	f32 Result = A.x * B.x + A.y * B.y + A.z * B.z;
	return(Result);
}

inline v3
Cross(v3 A, v3 B)
{
	v3 Result;

	Result.x = A.y * B.z - A.z * B.y;
	Result.y = A.z * B.x - A.x * B.z;
	Result.z = A.x * B.y - A.y * B.x;

	return(Result);
}

inline v3
Normalize(v3 V)
{
	v3 Result = {};

	f32 Length = sqrtf(Dot(V, V));
	if(Length != 0.0f)
	{
		Result = (1.0f / Length) * V;
	}

	return(Result);
}

internal mat4
Mat4Identity()
{
	mat4 R =
	{
		{{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1}}
	};

	return(R);
}

internal mat4
Mat4(v3 X, v3 Y, v3 Z)
{
	mat4 R =
	{
		{{X.x, Y.x, Z.x, 0},
		{X.y, Y.y, Z.y, 0},
		{X.z, Y.z, Z.z, 0},
		{0, 0, 0, 1}}
	};

	return(R);
}

internal mat4
Mat4Scale(f32 C)
{
	mat4 R =
	{
		{{C, 0, 0, 0},
		{0, C, 0, 0},
		{0, 0, C, 0},
		{0, 0, 0, 1}}
	};

	return(R);
}

internal mat4
Mat4Translate(v3 V)
{
	mat4 R =
	{
		{{1, 0, 0, V.x},
		{0, 1, 0, V.y},
		{0, 0, 1, V.z},
		{0, 0, 0, 1}}
	};

	return(R);
}

internal v4
Mat4Transform(mat4 T, v4 V)
{
	v4 Result = {};

	Result.x = T.E[0][0] * V.x + T.E[0][1] * V.y + T.E[0][2] * V.z + T.E[0][3] * V.w;
	Result.y = T.E[1][0] * V.x + T.E[1][1] * V.y + T.E[1][2] * V.z + T.E[1][3] * V.w;
	Result.z = T.E[2][0] * V.x + T.E[2][1] * V.y + T.E[2][2] * V.z + T.E[2][3] * V.w;
	Result.w = T.E[3][0] * V.x + T.E[3][1] * V.y + T.E[3][2] * V.z + T.E[3][3] * V.w;

	return(Result);
}

internal mat4
Mat4TransposeMat3(mat4 T)
{
	mat4 R = T;

	for(s32 i = 0; i < 3; ++i)
	{
		for(s32 j = 0; j < 3; ++j)
		{
			if((i != j) && (i < j))
			{
				f32 Temp =  R.E[j][i];
				R.E[j][i] = R.E[i][j];
				R.E[i][j] = Temp;
			}
		}
	}

	return(R);
}

inline v3
operator*(mat4 T, v3 V)
{
	v3 Result = Mat4Transform(T, V4(V, 1.0)).xyz;
	return(Result);
}

internal mat4
Mat4Multiply(mat4 A, mat4 B)
{
	mat4 R = {};

	for(s32 i = 0; i <= 3; ++i)
	{
		for(s32 j = 0; j <= 3; ++j)
		{
			for(s32 k = 0; k <= 3; ++k)
			{
				R.E[i][j] += A.E[i][k] * B.E[k][j];
			}
		}
	}

	return(R);
}

inline mat4
operator*(mat4 A, mat4 B)
{
	mat4 R = Mat4Multiply(A, B);
	return(R);
}

internal mat4
Mat4Camera(v3 P, v3 Target)
{
	mat4 R;

	v3 Z = Normalize(P - Target);
	v3 X = Normalize(Cross(YAxis(), Z));
	v3 Y = Normalize(Cross(Z, X));

	mat4 CameraFrame = Mat4(X, Y, Z);

	mat4 Translate = Mat4Translate(-1.0f * P);

	R = Mat4TransposeMat3(CameraFrame) * Translate;

	return(R);


}

internal mat4
Mat4Perspective(f32 FOV, f32 AspectRatio, f32 ZNear, f32 ZFar)
{
	f32 HalfFOV = FOV / 2.0f;

	mat4 R =
	{
		{{1.0f / (tanf(HalfFOV) * AspectRatio), 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f / tanf(HalfFOV), 0.0f, 0.0f},
		{0.0f, 0.0f, -1.0f * (ZFar + ZNear) / (ZFar - ZNear), -1.0f},
		{0.0f, 0.0f, -1.0f, 0.0f}}
	};

	return(R);
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

//
// NOTE(Justin): DAE/XML 
//

struct string
{
	u8 *Data;
	u64 Size;
};

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
	b32 Result = SubStringExists((char *)HayStack.Data, Needle);
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

internal void
ParseS32Array(u32 *Dest, u32 DestCount, string Data)
{
	char Delimeter[] = " ";
	char *Token;

	u32 TokenCount = 0;
	Token = strtok((char *)Data.Data, Delimeter);

	u32 Index = 0;
	Dest[Index++] = (u32)atoi(Token);

	while(Token)
	{
		Token = strtok(0, Delimeter);
		TokenCount++;
		if(TokenCount == 3)
		{
			Dest[Index++] = (u32)atoi(Token);
			TokenCount = 0;
			
			if(Index == (DestCount))
			{
				break;
			}
		}
	}
}

internal s32
S32FromASCII(u8 *S)
{
	s32 Result = atoi((char *)S);
	return(Result);
}

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

	Node->Attributes = PushArray(Arena, Node->AttributeCountMax, xml_attribute);
	Node->ChildrenMaxCount = COLLADA_NODE_CHILDREN_MAX_COUNT;

	Node->Children = PushArray(Arena, Node->ChildrenMaxCount, xml_node *);
	Node->Parent = Parent;

	return(Node);
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

internal string
StringFromRange(u8 *First, u8 *Last)
{
	string Result = {First, (u64)(Last - First)};
	return(Result);
}

internal string
String(u8 *Cstr)
{
	u8 *C = Cstr;
	for(; *C; ++C);
	string Result = StringFromRange(Cstr, C);

	return(Result);
}

internal xml_node *
ChildNodeAdd(memory_arena *Arena, xml_node *Parent)
{
	xml_node *LastChild = PushXMLNode(Arena, Parent);

	Parent->Children[Parent->ChildrenCount] = LastChild;
	Parent->ChildrenCount++;

	Assert(Parent->ChildrenCount < COLLADA_NODE_CHILDREN_MAX_COUNT);

	return(LastChild);
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
		fclose(FileHandle);

		char TagStart = '<';
		char TagEnd = '>';
		//char ForwardSlash = '/';

		// NOTE(Justin): Totally making up the size of the buffer.
		char *Buffer = (char *)calloc(Size/2, sizeof(char));
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
		Index -= ((s32)strlen("<COLLADA") + 1);

#if 0
		char TagDelimeters[] = "<>";
		string Token = String((u8 *)strtok((char *)(Content + Index), TagDelimeters));
		while(Token.Data)
		{
			Token = String((u8 *)strtok(0, TagDelimeters));
			if(SubStringExists(Token, " ") && !CurrentNode->Tag.Data)
			{
				CurrentNode->Tag = Token;
			}
			else if(SubStringExists(Token, " ") && CurrentNode->Tag.Data && !CurrentNode->InnerText.Data)
			{
				CurrentNode->InnerText = Token;
			}
			else if(SubStringExists(Token, "-") && !CurrentNode->InnerText.Data)
			{
				CurrentNode->InnerText = Token;
			}
			else if(Token.Data[0] == '/')
			{
				if(CurrentNode->Parent)
				{
					CurrentNode = CurrentNode->Parent;
				}
				else
				{
					break;
				}
			}
			else
			{
				// NOTE(Justin): Child node

				CurrentNode = ChildNodeAdd(Arena, CurrentNode);
				CurrentNode->Tag = Token;
			}

		}
#endif

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
					CurrentNode = ChildNodeAdd(Arena, CurrentNode);
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

		free(Buffer);
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

// TODO(Justin): Traverse the tree once.
internal mesh
MeshInit(memory_arena *Arena, loaded_dae DaeFile)
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
	Mesh.IndicesCount = 3 * S32FromASCII(AttrI.Value.Data);

	Mesh.Positions = PushArray(Arena, Mesh.PositionsCount, f32);
	Mesh.Normals = PushArray(Arena, Mesh.NormalsCount, f32);
	Mesh.UV = PushArray(Arena, Mesh.UVCount, f32);
	Mesh.Indices = PushArray(Arena, Mesh.IndicesCount, u32);

	ParseF32Array(Mesh.Positions, Mesh.PositionsCount, NodePos.InnerText);
	ParseF32Array(Mesh.Normals, Mesh.NormalsCount, NodeNormal.InnerText);
	ParseF32Array(Mesh.UV, Mesh.UVCount, NodeUV.InnerText);
	ParseS32Array(Mesh.Indices, Mesh.IndicesCount, NodeIndex.InnerText);

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
				loaded_dae CubeDae = ColladaFileLoad(Arena, "cube.dae");
				mesh Cube = MeshInit(Arena, CubeDae);

				mat4 ModelTransorm = Mat4Translate(V3(0.0f, 0.0f, -5.0f));

				v3 P = V3(0.0f, 5.0f, 3.0f);
				v3 Direction = V3(0.0f, -0.5f, -1.0f);
				mat4 CameraTransform = Mat4Camera(P, P + Direction);

				f32 FOV = DegreeToRad(45.0f);
				f32 Aspect = (f32)Window.Width / (f32)Window.Height;
				f32 ZNear = 0.1f;
				f32 ZFar = 100.0f;

				mat4 PerspectiveTransform = Mat4Perspective(FOV, Aspect, ZNear, ZFar);

				mat4 MVP = PerspectiveTransform *
						   CameraTransform *
						   ModelTransorm;

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

				uniform mat4 MVP;
				void main()
				{
					gl_Position = MVP * vec4(P, 1.0);
				})";

				char *FsSrc = R"(
				#version 330 core
				out vec4 FragColor;
				uniform vec4 uColor;
				void main()
				{
					FragColor = vec4(1.0, 0, 0, 1.0);
				})";


				u32 ShaderProgram = GLProgramCreate(VsSrc, FsSrc);

				u32 PosVB, PosVA;
				glGenVertexArrays(1, &PosVA);
				glGenBuffers(1, &PosVB);
				glBindVertexArray(PosVA);
				glBindBuffer(GL_ARRAY_BUFFER, PosVB);
				glBufferData(GL_ARRAY_BUFFER, Cube.PositionsCount * sizeof(f32), Cube.Positions, GL_STATIC_DRAW);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
				glEnableVertexAttribArray(0);

				u32 IBO;
				glGenBuffers(1, &IBO);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, Cube.IndicesCount * sizeof(u32), Cube.Indices, GL_STATIC_DRAW);

				glUseProgram(ShaderProgram);
				s32 UniformLocation = glGetUniformLocation(ShaderProgram, "MVP");
				glUniformMatrix4fv(UniformLocation, 1, GL_TRUE, &MVP.E[0][0]);

				while(!glfwWindowShouldClose(Window.Handle))
				{
					glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					glDrawElements(GL_TRIANGLES, Cube.IndicesCount, GL_UNSIGNED_INT, 0);

					glfwSwapBuffers(Window.Handle);
					glfwPollEvents();
				}
			}
		}
	}

	return(0);
}
