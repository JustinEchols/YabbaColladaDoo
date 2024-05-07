/*

 Begining tags come in two flavors

	<tagname>
	<tagname key1="value1" key2="value2" . . . keyn="valuen">

 Ending tags always have the same name as the begining tag but with a pre-prended forward slash
	
	<tagname></tagname>

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

typedef size_t memory_index;

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

struct xml_attribute_list
{
	s32 AttributeCountMax;
	s32 AttributeCount;
	xml_attribute *Attributes;
};

struct xml_node
{
	char *Tag;
	char *InnerText;

	s32 AttributeCountMax;
	s32 AttributeCount;
	xml_attribute *Attributes;

	xml_node *Parent;

	s32 ChildrenMaxCount;
	s32 ChildrenCount;
	xml_node **Children;
};

internal xml_attribute *
PushXMLAttribute()
{
	xml_attribute *Attribute = (xml_attribute *)calloc(1, sizeof(xml_attribute));
	Attribute->Key = 0;
	Attribute->Value = 0;

	return(Attribute);
}

// TODO(Justin): Consolidate the attributes struct into the xml node struct?
internal xml_node *
PushXMLNode(xml_node *Parent)
{
	xml_node *Node = (xml_node *)calloc(1, sizeof(xml_node));

	Node->Tag = 0;
	Node->InnerText = 0;
	Node->AttributeCount = 0;
	Node->AttributeCountMax = XML_ATTRIBUTE_MAX_COUNT;
	Node->Attributes = (xml_attribute *)calloc(Node->AttributeCountMax, sizeof(xml_attribute));

	Node->ChildrenMaxCount = XML_NODE_CHILDREN_MAX_COUNT;
	Node->ChildrenCount = 0;

	Node->Children = (xml_node **)calloc(Node->ChildrenMaxCount, sizeof(xml_node *));

	Node->Parent = Parent;

	return(Node);
}

internal xml_node *
ChildNodeGet(xml_node *Node, s32 Index)
{
	xml_node *Result = 0;
	if((Index > 0) && (Index < Node->ChildrenCount))
	{
		Result = Node->Children[Index];
		Assert(Result);
	}

	return(Result);
}

internal void
NodePrint(xml_node *Node)
{
	printf("Tag:%s\n", Node->Tag);

	// NOTE(Justin): AttributeCount is really an index currently
	for(s32 Index = 0; Index < Node->AttributeCount; ++Index)
	{
		xml_attribute *Attr = Node->Attributes + Index;
		printf("Key:%s Value:%s\n", Attr->Key, Attr->Value);

	}
	printf("InnerText:%s\n", Node->InnerText);

}

internal void *
NodeAddChild(xml_node *Node, xml_node *Child)
{
	if(Node->ChildrenCount < Node->ChildrenMaxCount)
	{
		xml_node *Last = Node->Children[Node->ChildrenCount++];
		Last = Child;
	}
}

int main(int Argc, char **Argv)
{
	FILE *FileHandle = fopen("test2.xml", "r");
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

		char Buffer[256];
		s32 InnerTextIndex = 0;
		s32 Index = 0;

		xml_node *Root = PushXMLNode(0);
		xml_node *CurrentNode = Root;

		while(Content[Index] != '\0')
		{
			if(!CurrentNode->Tag)
			{
				Index++;
				while(Content[Index] != TagEnd)
				{
					Buffer[InnerTextIndex++] = Content[Index++];

					if(((Content[Index] == ' ') || (Content[Index + 1] == TagEnd)) && (!CurrentNode->Tag))
					{
						// NOTE(Justin): Two types of tags. Tag with key/values
						// and tags without key/values. So, need to handle both.
						if(Content[Index] != ' ')
						{
							Buffer[InnerTextIndex++] = Content[Index];
						}

						Buffer[InnerTextIndex] = '\0';
						CurrentNode->Tag = strdup(Buffer);
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
					}
				}

				Index++;
				InnerTextIndex = 0;
				while(Content[Index] != TagStart)
				{
					Buffer[InnerTextIndex++] = Content[Index++];
				}
				Buffer[InnerTextIndex] = '\0';
				CurrentNode->InnerText = strdup(Buffer);
				InnerTextIndex = 0;
			}
			else
			{
				if(Content[Index] == '<' && Content[Index + 1] == '/')
				{
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

					xml_node *LastChild = CurrentNode->Children[CurrentNode->ChildrenCount++];
					xml_node *NewNode = PushXMLNode(CurrentNode);
					LastChild = NewNode;
					CurrentNode = NewNode;
				}
				else
				{
					// TODO(Justin): Inspect this case
					while(Content[Index] != TagStart)
					{
						Buffer[InnerTextIndex++] = Content[Index++];
					}

					Buffer[InnerTextIndex] = '\0';
					CurrentNode->InnerText = strdup(Buffer);
					InnerTextIndex = 0;
				}



				// NOTE(Justin): If a tag exists assume that the tag has been
				// processed.

				// Note(Justin): Three cases
				// 1) Inner text
				// 2) New node
				// 3) Closing tag

				// NOTE(Justin): At the end of the original node or at the
				// begining of a child node. This depends on wether or not
				// the first character after '<' is a forward slash '/' or
				// not. 

			}
		}

		NodePrint(Root);
		NodePrint(ChildNodeGet(Root, 0));
	}

	return(0);
}
