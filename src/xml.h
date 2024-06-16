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

	s32 AttributeCount;
	s32 AttributeCountMax;
	xml_attribute *Attributes;

	xml_node *Parent;

	s32 ChildrenCount;
	s32 ChildrenMaxCount;
	xml_node **Children;
};

struct loaded_dae 
{
	string FullPath;
	xml_node *Root;
};

#define XML_H
#endif
