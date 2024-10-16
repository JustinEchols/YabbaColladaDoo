internal xml_node * 
PushXMLNode(memory_arena *Arena, xml_node *Parent)
{
	xml_node *Node = PushStruct(Arena, xml_node);

	Node->AttributeCountMax = COLLADA_ATTRIBUTE_MAX_COUNT;
	Node->Attributes = PushArray(Arena, Node->AttributeCountMax, xml_attribute);

	Node->Parent = Parent;

	Node->ChildrenMaxCount = COLLADA_NODE_CHILDREN_MAX_COUNT;
	Node->Children = PushArray(Arena, Node->ChildrenMaxCount, xml_node *);

	return(Node);
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

internal xml_attribute
NodeAttributeGet(xml_node *Node, char *AttrName)
{
	xml_attribute Result = {};
	string Name = String((u8 *)AttrName);
	for(s32 Index = 0; Index < Node->AttributeCount; ++Index)
	{
		xml_attribute *Attr = Node->Attributes + Index;
		if(StringsAreSame(Attr->Key, Name)) 
		{
			Result = *Attr;
		}
	}

	return(Result);
}

internal string
NodeAttributeValueGet(xml_node *Node, char *AttrName)
{
	string Result = {};

	xml_attribute Attr = NodeAttributeGet(Node, AttrName);
	Result = Attr.Value;

	return(Result);
}

internal void
NodeGet(xml_node *Root, xml_node *N, char *TagName, char *ID = 0)
{
	for(s32 Index = 0; Index < Root->ChildrenCount; ++Index)
	{
		if(N->Tag.Size == 0)
		{
			xml_node *Node = Root->Children[Index];
			Assert(Node);
			if(StringsAreSame(Node->Tag, TagName))
			{
				if(ID)
				{
					if(StringsAreSame(Node->Attributes->Value, ID))
					{
						*N = *Node;
					}
				}
				else
				{
					*N = *Node;
				}
			}

			if(*Node->Children)
			{
				NodeGet(Node, N, TagName, ID);
			}
		}
		else
		{
			break;
		}
	}
}

internal xml_node
NodeSourceGet(xml_node *Root, char *TagName, char *ID)
{
	xml_node Result = {};

	xml_node N = {};
	NodeGet(Root, &N, TagName, ID);

	string SourceName = NodeAttributeValueGet(&N, "source");
	Assert(SourceName.Size != 0);
	SourceName.Data++;
	SourceName.Size--;

	N = {};
	NodeGet(Root, &N, "source", (char *)SourceName.Data);

	Result = *(N.Children[0]);

	return(Result);
}

internal b32
NodeHasKeysValues(string Str)
{
	b32 Result = false;
	u8 *C = Str.Data;
	for(u32 Index = 0; Index < Str.Size; ++Index)
	{
		if(*C++ == ' ')
		{
			Result = true;
			break;
		}
	}

	return(Result);
}

internal void
NodeProcessKeysValues(memory_arena *Arena, xml_node *Node, string Token, char Delimeters[], u32 DelimCount)
{
	string_list List = StringSplit(Arena, Token, (u8 *)Delimeters, DelimCount);

	string_node *T = List.First;

	xml_attribute *Attr = Node->Attributes + Node->AttributeCount;

	Attr->Key = StringAllocAndCopy(Arena, T->String);
	T = T->Next;
	Attr->Value = StringAllocAndCopy(Arena, T->String);
	Node->AttributeCount++;

	T = T->Next;
	while(T && !StringEndsWith(T->String, '/'))
	{
		Attr = Node->Attributes + Node->AttributeCount;

		// NOTE(Justin): The split is done such that every key after the first
		// one has a space at the very begining. Ignore it.

		T->String.Data++;
		T->String.Size--;

		Attr->Key = StringAllocAndCopy(Arena, T->String);
		T = T->Next;
		Attr->Value = StringAllocAndCopy(Arena, T->String);
		Node->AttributeCount++;
		Assert(Node->AttributeCount < COLLADA_ATTRIBUTE_MAX_COUNT);

		T = T->Next;
		if(T)
		{
			if(StringEndsWith(T->String, '/'))
			{
				break;
			}
		}
	}
}

internal void
ParseColladaFloatArray(memory_arena *Arena, xml_node *Root, f32 **Dest, u32 *DestCount, char *TagName, char *ID)
{
	xml_node SourceNode = NodeSourceGet(Root, TagName, ID);
	Assert(SourceNode.Tag.Size != 0);

	string StrCount = NodeAttributeValueGet(&SourceNode, "count");
	u32 Count = U32FromASCII(StrCount.Data);

	xml_node AccessorNode = {};
	NodeGet(SourceNode.Parent, &AccessorNode, "accessor");

	string StrStride = NodeAttributeValueGet(&AccessorNode, "stride");
	u32 Stride = 0;
	if(StrStride.Size == 0)
	{
		Stride = 1;
	}
	else
	{
		Stride = U32FromASCII(StrStride.Data);
	}

	*DestCount = Count / Stride;
	*Dest = PushArray(Arena, Count, f32);

	ParseF32Array(*Dest, Count, SourceNode.InnerText);
}


// TODO(Justin): Is Name_array the only string array in collada files?
internal void
ParseColladaStringArray(memory_arena *Arena, xml_node *Root, string **Dest, u32 *DestCount, char *StringArrayName = 0)
{
	xml_node TargetNode = {};
	NodeGet(Root, &TargetNode, "Name_array", StringArrayName);

	string Count = NodeAttributeValueGet(&TargetNode, "count");

	*DestCount  = U32FromASCII(Count.Data);
	*Dest = PushArray(Arena, *DestCount, string);

	ParseStringArray(Arena, *Dest, *DestCount, TargetNode.InnerText);
}

internal void
ParseColladaU32Array(memory_arena *Arena, xml_node *Root, u32 **Dest, u32 DestCount, char *U32ArrayName)
{
	xml_node TargetNode = {};
	NodeGet(Root, &TargetNode, U32ArrayName);

	*Dest = PushArray(Arena, DestCount, u32);

	ParseU32Array(*Dest, DestCount, TargetNode.InnerText);
}

internal u32
U32FromAttributeValue(xml_node *Node)
{
	u32 Result = 0;

	xml_attribute AttrCount = NodeAttributeGet(Node, "count");
	if(AttrCount.Key.Size != 0)
	{
		Result = U32FromASCII(AttrCount.Value.Data);
	}

	return(Result);
}

internal void
ColladaPrint(xml_node *Node)
{
	for(s32 ChildIndex = 0; ChildIndex < Node->ChildrenCount; ++ChildIndex)
	{
		xml_node *N = Node->Children[ChildIndex];
		printf("Tag:%s\nInnerText:%s\n\n", (char *)N->Tag.Data, (char *)N->InnerText.Data);

		if(*N->Children)
		{
			ColladaPrint(N);
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


				char TagDelimeters[] = "<>";
				char InnerTagDelimeters[] = "=\"";
				u32 DelimeterCount = ArrayCount(TagDelimeters);

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
		else
		{
			printf("Error file %s has size %d\n", FileName, Size);
		}

		fclose(FileHandle);
	}
	else
	{
		printf("Error opening file %s\n", FileName);
		perror("");
	}

	return(Result);
}
