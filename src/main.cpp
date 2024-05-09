/*

 Begining tags come in two flavors

	<tagname>
	<tagname key1="value1" key2="value2" . . . keyn="valuen">

 Ending tags always have the same name as the begining tag but with a pre-prended forward slash
	
	<tagname></tagname>

	// TODO(Justin): Length based strings
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define internal static
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#define ArrayCount(A) sizeof(A) / sizeof((A)[0])

// TODO(Justin): Is this large enough?
#define XML_ATTRIBUTE_MAX_COUNT 10
#define XML_NODE_CHILDREN_MAX_COUNT 10

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

struct string
{
	u8 *Data;
	u64 Size;
};

struct xml_doc
{
	char *FullPath;
	u8 *Content;
};

struct xml_attribute
{
	char *Key;
	char *Value;
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

internal xml_node *
PushXMLNode(xml_node *Parent)
{
	xml_node *Node = (xml_node *)calloc(1, sizeof(xml_node));

	//Node->Tag = 0;
	//Node->InnerText = 0;
	//Node->AttributeCount = 0;
	Node->AttributeCountMax = XML_ATTRIBUTE_MAX_COUNT;
	Node->Attributes = (xml_attribute *)calloc(Node->AttributeCountMax, sizeof(xml_attribute));

	Node->ChildrenMaxCount = XML_NODE_CHILDREN_MAX_COUNT;
	//Node->ChildrenCount = 0;

	Node->Children = (xml_node **)calloc(Node->ChildrenMaxCount, sizeof(xml_node *));

	Node->Parent = Parent;

	return(Node);
}

internal char *DepthStrings[7] = {"", "\t", "\t\t", "\t\t\t", "\t\t\t\t", "\t\t\t\t\t", "\t\t\t\t\t\t"};

internal void
NaryTreePrint(xml_node *Root, int *Depth)
{
	for(s32 Index = 0; Index < Root->ChildrenCount; ++Index)
	{
		xml_node *Node = Root->Children[Index];
		if(Node)
		{
			(*Depth)++;
			char *DepthString = DepthStrings[*Depth];
			printf("%sTag:%s\n", DepthString, Node->Tag);
			for(s32 AttrIndex = 0; AttrIndex < Node->AttributeCount; ++AttrIndex)
			{
				xml_attribute *Attribute = Node->Attributes + AttrIndex;

				printf("%sKey:%s Value:%s\n", DepthString, Attribute->Key, Attribute->Value);
			}
			printf("%sInnerText:%s\n", DepthString, Node->InnerText);
			NaryTreePrint(Node, Depth);
		}
		(*Depth)--;
	}
}

internal b32
StringsAreSame(char *S1, char *S2)
{
	b32 Result = (strcmp(S1, S2) == 0);
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
	u32 UVsCount;
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
StringEndsWith(string S, char C)
{
	u8 *Test = S.Data + (S.Size - 1);
	b32 Result = (*Test == (u8 )C);

	return(Result);
}

#if 0
internal void
ParseFloatArray(f32 *Dest, u32 DestCount, char *Data)
{
	char *Scan = Data;
	for(u32 Index = 0; Index < DestCount; ++Index)
	{
		Dest[Index] = (f32)atof(Scan);
		while(*Scan != ' ' && *Scan != '\0')
		{
			Scan++;
		}
		Scan++;

	}
}

internal void
ParseIndexArray(u32 *Dest, u32 DestCount, char *Data)
{
	char *Scan = Data;
	for(u32 Index = 0; Index < DestCount; ++Index)
	{
		Dest[Index] = (u32)atoi(Scan);
		while(*Scan != ' ' && *Scan != '\0')
		{
			Scan++;
		}
		Scan++;
	}
}

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
				if(SubStringExists(Node->Attributes->Value, "mesh-positions"))
				{
					for(s32 AttrIndex = 0; AttrIndex < Node->AttributeCount; ++AttrIndex)
					{
						xml_attribute *Attribute = Node->Attributes + AttrIndex;
						if(StringsAreSame(Attribute->Key, "count"))
						{
							Mesh->PositionsCount = atoi(Attribute->Value);
							Mesh->Positions = (f32 *)calloc(Mesh->PositionsCount, sizeof(f32));

							ParseFloatArray(Mesh->Positions, Mesh->PositionsCount, Node->InnerText);
						}
					}
				}
				else if(SubStringExists(Node->Attributes->Value, "mesh-normals"))
				{
					for(s32 AttrIndex = 0; AttrIndex < Node->AttributeCount; ++AttrIndex)
					{
						xml_attribute *Attribute = Node->Attributes + AttrIndex;
						if(StringsAreSame(Attribute->Key, "count"))
						{
							Mesh->NormalsCount = atoi(Attribute->Value);
							Mesh->Normals = (f32 *)calloc(Mesh->PositionsCount, sizeof(f32));

							ParseFloatArray(Mesh->Normals, Mesh->NormalsCount, Node->InnerText);
						}
					}
				}
				else if(SubStringExists(Node->Attributes->Value, "mesh-map-0"))
				{
					for(s32 AttrIndex = 0; AttrIndex < Node->AttributeCount; ++AttrIndex)
					{
						xml_attribute *Attribute = Node->Attributes + AttrIndex;
						if(StringsAreSame(Attribute->Key, "count"))
						{
							Mesh->UVsCount = atoi(Attribute->Value);
							Mesh->UV = (f32 *)calloc(Mesh->PositionsCount, sizeof(f32));

							ParseFloatArray(Mesh->UV, Mesh->UVsCount, Node->InnerText);
						}
					}
				}
			}

			if(StringsAreSame(Node->Tag, "triangles"))
			{
				for(s32 AttrIndex = 0; AttrIndex < Node->AttributeCount; ++AttrIndex)
				{
					xml_attribute *Attribute = Node->Attributes + AttrIndex;
					if(StringsAreSame(Attribute->Key, "count"))
					{
						Mesh->IndicesCount = 3 * 3 * atoi(Attribute->Value);
						Mesh->Indices = (u32 *)calloc(Mesh->IndicesCount, sizeof(u32));
					}
				}

			}

			if(StringsAreSame(Node->Tag, "p"))
			{
				Assert(Mesh->IndicesCount > 0);
				Assert(Mesh->Indices);

				ParseIndexArray(Mesh->Indices, Mesh->IndicesCount, Node->InnerText);
			}



			if(*Node->Children)
			{
				MeshInit(Node, Mesh);
			}
		}
	}
}
#endif

int main(int Argc, char **Argv)
{
	FILE *FileHandle = fopen("untitled.dae", "r");
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
		char ForwardSlash = '/';

		char Buffer[4096];
		s32 InnerTextIndex = 0;
		s32 Index = 0;

		xml_node *Root = PushXMLNode(0);
		xml_node *CurrentNode = Root;

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
						xml_attribute *Attribute = CurrentNode->Attributes + CurrentNode->AttributeCount;
						Buffer[InnerTextIndex] = '\0';
						Attribute->Key = strdup(Buffer);
						InnerTextIndex = 0;
						Index++;
					}

					if((Content[Index] == '"'))
					{
						Index++;
						while(Content[Index] != '"')
						{
							Buffer[InnerTextIndex++] = Content[Index++];
						}
						Buffer[InnerTextIndex] = '\0';

						xml_attribute *Attribute = CurrentNode->Attributes + CurrentNode->AttributeCount;
						Assert(Attribute->Key);
						Attribute->Value = strdup(Buffer);
						CurrentNode->AttributeCount++;
						Assert(CurrentNode->AttributeCount < XML_ATTRIBUTE_MAX_COUNT);

						// NOTE(Jusitn): One of two situations can happen
						// after processsing a value either we see a space
						// ' ' after the quote or a bracket '>'. If it is a
						// space eat it in order to start buffering the next
						// key. If it is a bracket advance by 1 which will
						// set the current character to '>' which results in
						// the loop terminating.
						InnerTextIndex = 0;
						if(Content[Index + 1] == ' ')
						{
							Index += 2;
						}
						else
						{
							Index++;
						}

						if(Content[Index] == '/')
						{
							Index++;
							CurrentNode = CurrentNode->Parent;
						}
					}
				}

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
					// NOTE(Justin): Closing tag and end of the current node.
					// Advance the index to one past '>'.
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
					// NOTE(Justin): Child node
					xml_node *LastChild = PushXMLNode(CurrentNode);
					CurrentNode->Children[CurrentNode->ChildrenCount] = LastChild;
					CurrentNode->ChildrenCount++;
					CurrentNode = LastChild;
				}
				else
				{
					// TODO(Justin): At the end of a child node and another
					// child node exists.
					while(Content[Index] != TagStart)
					{
						Buffer[InnerTextIndex++] = Content[Index++];
					}
					Buffer[InnerTextIndex] = '\0';
					InnerTextIndex = 0;

					// NOTE(Justin): I do not think this is required....
					if(!CurrentNode->InnerText.Data)
					{
						CurrentNode->InnerText.Data = (u8 *)strdup(Buffer);
						CurrentNode->InnerText.Size = InnerTextIndex;
					}
				}
			}
		}

		mesh *Cube = (mesh *)calloc(1, sizeof(mesh));
		if(Cube)
		{
		//	MeshInit(Root, Cube);
		}
	}

	return(0);
}
