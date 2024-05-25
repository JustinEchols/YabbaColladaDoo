
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

// TODO(Justin): Check for correctness
internal void
NodeGet(xml_node *Root, xml_node *N, char *TagName, char *ID = 0)
{
	for(s32 Index = 0; Index < Root->ChildrenCount; ++Index)
	{
		if(N->Tag.Size == 0)
		{
			xml_node *Node = Root->Children[Index];
			if(Node)
			{
				if(StringsAreSame(Node->Tag, TagName))
				{
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
					// TODO(Justin): Check for correctness
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

internal xml_node *
ChildNodeAdd(memory_arena *Arena, xml_node *Parent)
{
	xml_node *LastChild = PushXMLNode(Arena, Parent);

	Parent->Children[Parent->ChildrenCount] = LastChild;
	Parent->ChildrenCount++;

	Assert(Parent->ChildrenCount < COLLADA_NODE_CHILDREN_MAX_COUNT);

	return(LastChild);
}

internal b32
NodeHasKeysValues(string Str)
{
	b32 Result = SubStringExists(Str, " ");
	return(Result);
}

internal string
StringAllocAndCopyFromCstr(memory_arena *Arena, char *Cstr)
{
	string Result = {};

	Assert(Cstr);
	string Temp = String((u8 *)Cstr);

	Result.Size = Temp.Size;
	Result.Data = PushArray(Arena, Result.Size + 1, u8);
	ArrayCopy(Result.Size, Temp.Data, Result.Data);
	Result.Data[Result.Size] = '\0';

	return(Result);
}

internal void
NodeProcessKeysValues(memory_arena *Arena, xml_node *Node, string Token, char *TokenContext, char Delimeters[])
{

	char *TagToken = strtok_s((char *)Token.Data, Delimeters, &TokenContext);

	xml_attribute *Attr = Node->Attributes + Node->AttributeCount;
	Attr->Key = StringAllocAndCopyFromCstr(Arena, TagToken);

	TagToken = strtok_s(0, Delimeters, &TokenContext);
	Attr->Value = StringAllocAndCopyFromCstr(Arena, TagToken);

	Node->AttributeCount++;

	TagToken = strtok_s(0, Delimeters, &TokenContext);

	// WARNING(Justin): One must be very cautious when using strtok. If the node
	// only has one key/value pair then TagToken will be null after the above
	// statement. Therefore trying to read it results in an access violation.
	// The condition in the while works because the evaluation of TagToken is
	// done first before strchr executes. This is not great and is in my opinion
	// not very stable.
	while(TagToken && !strchr(TagToken, '/'))
	{
		Attr = Node->Attributes + Node->AttributeCount;

		// NOTE(Justin): Every key after the first one has a space at the start. Ignore it.
		//string Key = String((u8 *)(TagToken + 1));
		Attr->Key = StringAllocAndCopyFromCstr(Arena, (TagToken + 1));

		TagToken = strtok_s(0, Delimeters, &TokenContext);

		Attr->Value = StringAllocAndCopyFromCstr(Arena, TagToken);

		Node->AttributeCount++;
		Assert(Node->AttributeCount < COLLADA_ATTRIBUTE_MAX_COUNT);

		TagToken = strtok_s(0, Delimeters, &TokenContext);
		if(TagToken)
		{
			if(SubStringExists(TagToken, "/"))
			{
				break;
			}
		}
	}
}
