#if !defined(MEMORY_H)

struct memory_arena
{
	u8 *Base;
	memory_index Size;
	memory_index Used;
	s32 TempCount;
};

struct temporary_memory
{
	memory_arena *Arena;
	memory_index Used;
};

inline void
ArenaInitialize(memory_arena *Arena, u8 *Base, memory_index Size)
{
	Arena->Base = Base;
	Arena->Size = Size;
	Arena->Used = 0;
}

inline void *
PushSize_(memory_arena *Arena, memory_index Size)
{
	Assert((Arena->Used + Size) <= Arena->Size);

	void *Result = Arena->Base + Arena->Used;
	Arena->Used += Size;

	return(Result);
}

inline void *
MemoryCopy(memory_index Size, void *SrcInit, void *DestInit)
{
	u8 *Src = (u8 *)SrcInit;
	u8 *Dest = (u8 *)DestInit;
	while(Size--) {*Dest++ = *Src++;}

	return(DestInit);
}

inline void
MemoryZero(void *Src, memory_index Size)
{
	u8 *P = (u8 *)Src;
	while(Size--)
	{
		*P++ = 0;
	}
}

inline temporary_memory
TemporaryMemoryBegin(memory_arena *Arena)
{
	temporary_memory Result;

	Result.Arena = Arena;
	Result.Used = Arena->Used;

	Arena->TempCount++;

	return(Result);
}

inline void
TemporaryMemoryEnd(temporary_memory TempMemory)
{
	memory_arena *Arena = TempMemory.Arena;
	Assert(Arena->Used >= TempMemory.Used);
	Arena->Used = TempMemory.Used;
	Assert(Arena->TempCount > 0);
	Arena->TempCount--;
}

#define MEMORY_H
#endif
