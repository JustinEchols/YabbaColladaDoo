#if !defined(TEXTURE_H)

struct texture
{
	//char *FullPath;
	//char *FileName;
	u32 Handle;
	string FullPath;
	string FileName;
#if RENDER_TEST
	GLint StoredFormat;
	GLenum SrcFormat;
#endif
	int Width;
	int Height;
	int ChannelCount;
	u8 *Memory;
};

#define TEXTURE_H
#endif
