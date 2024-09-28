
inline b32
IsNumber(u8 C)
{
	b32 Result = ((C >= '0') && (C < '9'));
	return(Result);
}

inline b32 
IsNewLine(char *S)
{
	b32 Result = ((*S == '\n') || (*S == '\r'));
	return(Result);
}

inline b32 
IsNewLine(u8 *S)
{
	b32 Result = ((*S == '\n') || (*S == '\r'));
	return(Result);
}

inline b32 
IsSpace(u8 *S)
{
	b32 Result = ((*S == ' '));
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
	u32 Result = (u32)atoi((char *)S);
	return(Result);
}

inline u32
U32FromASCII(string S)
{
	u32 Result = U32FromASCII(S.Data);
	return(Result);
}

inline f32
F32FromASCII(u8 *S)
{
	f32 Result = (f32)atof((char *)S);
	return(Result);
}

inline f32
F32FromASCII(string S)
{
	f32 Result = F32FromASCII(S.Data);
	return(Result);
}

inline char 
U32ToASCII(u32 U32)
{
	char Result = (char)('0' + U32);
	return(Result);
}

inline u32
DigitCount(u32 U32)
{
	u32 Result = 0;
	if(U32 > 9)
	{
		while(U32 != 0)
		{
			U32 /= 10;
			Result++;
		}
	}
	else
	{
		Result++;
	}

	return(Result);
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

inline u32
StringLength(char *Cstr)
{
	u32 Result = 0;
	for(char *C = Cstr; *C++;) {Result++;}
	return(Result);
}

internal void
StringPrint(string S)
{
	printf("%s\n", (char *)S.Data);
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

inline b32
StringsAreSame(char *Str1, char *Str2)
{
	string S1 = String((u8 *)Str1);
	string S2 = String((u8 *)Str2);

	b32 Result = StringsAreSame(S1, S2);

	return(Result);
}

inline b32
StringsAreSame(u8 *Str1, char *Str2)
{
	string S1 = String(Str1);
	string S2 = String((u8 *)Str2);

	b32 Result = StringsAreSame(S1, S2);

	return(Result);
}

inline b32
StringsAreSame(u8 *Str1, u8 *Str2)
{
	string S1 = String(Str1);
	string S2 = String(Str2);

	b32 Result = StringsAreSame(S1, S2);

	return(Result);
}

inline b32
StringsAreSame(string S1, char *Str2)
{
	string S2 = String((u8 *)Str2);

	b32 Result = StringsAreSame(S1, S2);

	return(Result);
}

inline b32
StringsAreSame(char *Str1, u8 *Str2)
{
	b32 Result = StringsAreSame(Str2, Str1);
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

internal string
StringSearchFor(string S, char C)
{
	u8 *P = S.Data;
	u8 PC = (u8)C;
	for(u32 Index = 0; Index < S.Size; Index++)
	{
		P++;
		if(*P == PC)
		{
			break;
		}
	}

	string Result = StringFromRange(P, S.Data + S.Size);

	return(Result);

}

internal string
StringCat(memory_arena *Arena, string S1, string S2)
{
	string Result = {};
	Result.Size = S1.Size + S2.Size;
	Result.Data = PushArray(Arena, Result.Size + 1, u8);

	u32 i;
	for(i = 0; i < S1.Size; ++i)
	{
		Result.Data[i] = S1.Data[i];
	}

	for(u32 j = 0; j < S2.Size; ++j)
	{
		Result.Data[i++] = S2.Data[j];
	}
	Result.Data[Result.Size] = 0;

	return(Result);
}

internal void
StringListPushExplicit(string_list *List, string String, string_node *Node)
{
	Node->String = String;
	SLLQueuePush(List->First, List->Last, Node);
	List->Count += 1;
	List->Size += String.Size;
}

internal void
StringListPush(memory_arena *Arena, string_list *List, string String)
{
	string_node *Node = PushArray(Arena, 1, string_node);
	StringListPushExplicit(List, String, Node);
}

internal string_list
StringSplit(memory_arena *Arena, string String, u8 *Splits, u32 Count)
{
	string_list Result = {};

	u8 *At = String.Data;
	u8 *FirstWord = String.Data;
	u8 *OnePastLast = String.Data + String.Size;
	for(; At < OnePastLast; ++At)
	{
		u8 Byte = *At;
		b32 ShouldSplit = false;
		for(u32 Index = 0; Index < Count; ++Index)
		{
			if(Byte == Splits[Index])
			{
				ShouldSplit = true;
				break;
			}
		}

		if(ShouldSplit)
		{
			if(FirstWord < At)
			{
				StringListPush(Arena, &Result, StringFromRange(FirstWord, At));
			}

			FirstWord = At + 1;
		}
	}

	if(FirstWord < At)
	{
		StringListPush(Arena, &Result, StringFromRange(FirstWord, At));
	}

	return(Result);
}


internal string
StringAlloc(memory_arena *Arena, u32 Count)
{
	string Result = {};
	
	Result.Size = Count;
	Result.Data = PushArray(Arena, Result.Size + 1, u8);
	Result.Data[Result.Size] = 0;

	return(Result);
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

internal string
StringAllocAndCopy(memory_arena *Arena, u8 *Str)
{
	string Result = StringAllocAndCopy(Arena, String(Str));
	return(Result);
}

internal string_array
StringArrayAllocate(memory_arena *Arena, u32 Count)
{
	string_array Result = {};

	Result.Count = Count;
	Result.Strings = PushArray(Arena, Result.Count, string);
	
	return(Result);
}

internal string
DigitToString(memory_arena *Arena, u32 U32)
{
	string Result;
	Result.Size = DigitCount(U32);
	Result.Data = PushArray(Arena, Result.Size + 1, u8);
	Result.Data[Result.Size + 1] = 0;

	u32 Last = (u32)Result.Size - 1;
	for(u32 Index = 0; Index < Result.Size; ++Index)
	{
		u32 Digit = U32 % 10;
		U32 /= 10;
		Result.Data[Last - Index] = U32ToASCII(Digit);
	}

	return(Result);
}

internal string_array
StringSplitIntoArray(memory_arena *Arena, string Str, u8 *Delimeters, u32 DelimCount)
{
	string_array Result = {};

	string_list List = StringSplit(Arena, Str, Delimeters, DelimCount);

	Result.Count = (u32)List.Count;
	Result.Strings = PushArray(Arena, Result.Count, string);

	string_node *StrNode = List.First;
	for(u32 Index = 0; Index < List.Count; ++Index)
	{
		string S = StringAllocAndCopy(Arena, StrNode->String);
		S.Data[S.Size] = 0;
		Result.Strings[Index] = S;
		StrNode = StrNode->Next;
	}

	return(Result);
}

internal string
F32ToString(memory_arena *Arena, f32 F32)
{
	string Result;

	char Buff[256];
	sprintf(Buff, "%f", F32);

	Result = StringAllocAndCopy(Arena, Buff);

	return(Result);
}

internal string_list
F32ArrayToStringList(memory_arena *Arena, f32 *A, u32 Count)
{
	string_list Result = {};

	char Buff[256];
	for(u32 i = 0; i < Count; ++i)
	{
		sprintf(Buff, "%f", A[i]);
		string S = StringAllocAndCopy(Arena, Buff);
		StringListPush(Arena, &Result, S);
	}

	return(Result);
}

internal string_array
F32ArrayToStringArray(memory_arena *Arena, f32 *A, u32 Count)
{
	string_array Result = {};

	Result.Count = Count;
	Result.Strings = PushArray(Arena, Result.Count, string);

	char Buff[256];
	for(u32 i = 0; i < Count; ++i)
	{
		sprintf(Buff, "%f", A[i]);
		string S = StringAllocAndCopy(Arena, Buff);
		Result.Strings[i] = S;
	}

	return(Result);
}


internal void
ParseU32Array(u32 *Dest, u32 DestCount, string Str)
{
	if(DestCount != 0)
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
}

internal void
ParseF32Array(f32 *Dest, u32 DestCount, string Str)
{
	if(DestCount != 0)
	{
		Assert(Dest);

		char *Context;
		char *Tok = strtok_s((char *)Str.Data, " \n\r", &Context);
		Dest[0] = F32FromASCII((u8 *)Tok);
		for(u32 Index = 1; Index < DestCount; ++Index)
		{
			Tok = strtok_s(0, " \n\r", &Context);
			if(Tok)
			{
				Dest[Index] = F32FromASCII((u8 *)Tok);
			}
		}
	}
}

internal void
ParseF32Array(memory_arena *Arena, f32 *Dest, u32 DestCount, string Str)
{
	if(DestCount != 0)
	{
		Assert(Dest);

		char Delim[] = " \n\r";
		string_list List = StringSplit(Arena, Str, (u8 *)Delim, 3);
		Assert(List.Count == DestCount);
		string_node *Token = List.First;
		Dest[0] = F32FromASCII((u8 *)Token->String.Data);
		for(u32 Index = 1; Index < List.Count; ++Index)
		{
			Token = Token->Next;
			if(Token)
			{
				Dest[Index] = F32FromASCII((u8 *)Token->String.Data);
			}
		}
	}
}

internal void
ParseStringArray(memory_arena *Arena, string *Dest, u32 DestCount, string Str)
{
	char *Context;
	char *Tok = strtok_s((char *)Str.Data, " \n\r", &Context);

	u32 Index = 0;
	if(!IsNewLine(Tok))
	{
		Dest[Index++] = StringAllocAndCopy(Arena, Tok);
	}

	while(Tok)
	{
		Tok = strtok_s(0, " \n\r", &Context);
		if(Tok)
		{
			if(!IsNewLine(Tok))
			{
				Dest[Index++] = StringAllocAndCopy(Arena, Tok);
			}
		}
	}
}

inline void
EatSpaces(u8 **Buff)
{
	while(**Buff == ' ')
	{
		(*Buff)++;
	}
}

inline void
BufferNextWord(u8 **C, u8 *Buff)
{
	u32 Index = 0;
	while(!IsNewLine(*C) && !IsSpace(*C))
	{
		Buff[Index++] = **C;
		(*C)++;

	}
	Buff[Index] = 0;
}

inline void
AdvanceToNewLine(u8 **C)
{
	while(!IsNewLine(*C))
	{
		(*C)++;
	}
	(*C)++;
}
