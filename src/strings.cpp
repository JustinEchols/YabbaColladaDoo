
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

internal b32
StringEndsWith(string S, char C)
{
	u8 *Test = S.Data + (S.Size - 1);
	b32 Result = (*Test == (u8 )C);

	return(Result);
}

internal b32
StringsAreSame(string S1, string S2)
{
	b32 Result = (S1.Size == S2.Size);
	if(Result)
	{
		for(u64 Index = 0; Index < S1.Size; ++Index)
		{
			if(S1.Data[Index] != S2.Data[Index])
			{
				Result = false;
				break;
			}
		}
	}

	return(Result);
}

internal b32
StringsAreSame(char *Str1, char *Str2)
{
	string S1 = String((u8 *)Str1);
	string S2 = String((u8 *)Str2);

	b32 Result = StringsAreSame(S1, S2);

	return(Result);
}

internal b32
StringsAreSame(string S1, char *Str2)
{
	string S2 = String((u8 *)Str2);

	b32 Result = StringsAreSame(S1, S2);

	return(Result);
}

internal b32
SubStringExists(char *HayStack, char *Needle)
{
	b32 Result = false;
	if(HayStack && Needle)
	{
		char *S = strstr(HayStack, Needle);
		if(S)
		{
			Result = true;
		}
	}

	return(Result);
}

internal b32
SubStringExists(string HayStack, char *Needle)
{
	b32 Result = SubStringExists((char *)HayStack.Data, Needle);
	return(Result);
}

inline s32
S32FromASCII(u8 *S)
{
	s32 Result = atoi((char *)S);
	return(Result);
}

inline u32
U32FromASCII(u8 *S)
{
	u32 Result = (u32 )atoi((char *)S);
	return(Result);
}

inline f32
F32FromASCII(u8 *S)
{
	f32 Result = (f32)atof((char *)S);
	return(Result);
}

internal void
ParseU32Array(u32 *Dest, u32 DestCount, string Str)
{
	Assert(Dest);

	char *Context;
	char *Tok = strtok_s((char *)Str.Data, " ", &Context);
	Dest[0] = U32FromASCII((u8 *)Tok);
	for(u32 Index = 1; Index < DestCount; ++Index)
	{
		Tok = strtok_s(0, " ", &Context);
		if(Tok)
		{
			Dest[Index] = U32FromASCII((u8 *)Tok);
		}
	}
}

internal void
ParseF32Array(f32 *Dest, u32 DestCount, string Str)
{
	Assert(Dest);

	char *Context;
	char *Tok = strtok_s((char *)Str.Data, " ", &Context);
	Dest[0] = F32FromASCII((u8 *)Tok);
	for(u32 Index = 1; Index < DestCount; ++Index)
	{
		Tok = strtok_s(0, " ", &Context);
		if(Tok)
		{
			Dest[Index] = F32FromASCII((u8 *)Tok);
		}
	}
}


internal string
StringAllocAndCopy(memory_arena *Arena, string Str)
{
	Assert(Str.Data);

	string Result = {};
	
	Result.Size = Str.Size;
	Result.Data = PushArray(Arena, Result.Size + 1, u8);

	ArrayCopy(Result.Size, Str.Data, Result.Data);
	Result.Data[Result.Size] = '\0';

	return(Result);
}

internal string
StringAllocAndCopy(memory_arena *Arena, char *Cstr)
{
	string Result = StringAllocAndCopy(Arena, String((u8 *)Cstr));
	return(Result);
}

// NOTE(Justin): This relies on the fact that the strings are separated by spaces ' '
// Is this parse sentence?
internal void
ParseStringArray(memory_arena *Arena, string *Dest, u32 DestCount, string Str)
{
	char *Context;
	char *Tok = strtok_s((char *)Str.Data, " ", &Context);

	u32 Index = 0;
	Dest[Index++] = StringAllocAndCopy(Arena, Tok);

	while(Tok)
	{
		Tok = strtok_s(0, " ", &Context);
		if(Tok)
		{
			Dest[Index++] = StringAllocAndCopy(Arena, Tok);
		}

	}
}