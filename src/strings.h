#if !defined(STRINGS_H)

struct string
{
	u8 *Data;
	u64 Size;
};

struct string_node
{
	string_node *Next;
	string String;
};

struct string_list
{
	string_node *First;
	string_node *Last;
	u64 Count;
	u64 Size;
};

struct string_array
{
	u32 Count;
	string *Strings;
};

#define STRINGS_H
#endif
