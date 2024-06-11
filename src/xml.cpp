internal xml_node * PushXMLNode(memory_arena *Arena, xml_node *Parent)
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

internal string
NodeAttributeValueGet(xml_node *Node, char *AttrName)
{
	string Result = {};

	xml_attribute Attr = NodeAttributeGet(Node, AttrName);
	Result = Attr.Value;

	return(Result);
}

internal b32 
NodeAttributeValueExists(xml_node *Node, char *AttrValue)
{
	b32 Result = false;
	string Value = String((u8 *)AttrValue);
	for(s32 Index = 0; Index < Node->AttributeCount; ++Index)
	{
		xml_attribute *Attr = Node->Attributes + Index;
		if(StringsAreSame(Attr->Value, Value)) 
		{
			Result = true;
		}
	}

	return(Result);
}

// TODO(Justin): Check for correctness

inline b32
NodeIsValid(xml_node N)
{
	b32 Result = (N.Tag.Size != 0);
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

	N = {};
	NodeGet(Root, &N, "source", (char *)SourceName.Data);

	Result = *(N.Children[0]);

	return(Result);
}

internal void
FirstNodeWithAttrValue(xml_node *Root, xml_node *N, char *AttrValue)
{
	for(s32 Index = 0; Index < Root->ChildrenCount; ++Index)
	{
		if(N->Tag.Size == 0)
		{
			xml_node *Node = Root->Children[Index];
			if(NodeAttributeValueExists(Node, AttrValue))
			{
				*N = *Node;
			}
			else if(*Node->Children)
			{
				FirstNodeWithAttrValue(Node, N, AttrValue);
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

internal void
NodeProcessKeysValues(memory_arena *Arena, xml_node *Node, string Token, char *TokenContext, char Delimeters[])
{
	char *TagToken = strtok_s((char *)Token.Data, Delimeters, &TokenContext);

	xml_attribute *Attr = Node->Attributes + Node->AttributeCount;
	Attr->Key = StringAllocAndCopy(Arena, TagToken);

	TagToken = strtok_s(0, Delimeters, &TokenContext);
	Attr->Value = StringAllocAndCopy(Arena, TagToken);

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
		Attr->Key = StringAllocAndCopy(Arena, (TagToken + 1));

		TagToken = strtok_s(0, Delimeters, &TokenContext);

		Attr->Value = StringAllocAndCopy(Arena, TagToken);

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
	u32 Stride = U32FromASCII(StrStride.Data);

	*DestCount = Count / Stride;
	*Dest = PushArray(Arena, Count, f32);

	ParseF32Array(*Dest, Count, SourceNode.InnerText);
}


// TODO(Justin): Is Name_array the only xml string array in collada files?
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


// NOTE(Justin): The API for this routine is slightly different than the others.
internal void
ParseColladaU32Array(memory_arena *Arena, xml_node *Root, u32 **Dest, u32 DestCount, char *U32ArrayName)
{
	xml_node TargetNode = {};
	NodeGet(Root, &TargetNode, U32ArrayName);

	*Dest = PushArray(Arena, DestCount, u32);

	ParseU32Array(*Dest, DestCount, TargetNode.InnerText);
}

// TODO(Justin): Think about what is needed for this. (more/less ?)
// Is passing a null value a bad idea?
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
