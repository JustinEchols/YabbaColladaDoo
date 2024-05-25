#if !defined(XML_H)

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

#define XML_H
#endif
