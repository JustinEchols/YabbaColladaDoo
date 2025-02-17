
internal texture
TextureLoad(memory_arena *Arena, char *Path)
{
	texture Texture = {};

	Texture.FullPath = StringAllocAndCopy(Arena, Path);

	char Buffer[256];
	MemoryZero(Buffer, sizeof(Buffer));
	GetFileName(Buffer, Texture.FullPath); 

	Texture.FileName = StringAllocAndCopy(Arena, Buffer);


	// NOTE(Justin): Force the image to load with 4 components. All texture formats will be the same.
	stbi_set_flip_vertically_on_load(true);
	Texture.Memory = stbi_load(Path, &Texture.Width, &Texture.Height, &Texture.ChannelCount, 4);
	stbi_set_flip_vertically_on_load(false);
	if(Texture.Memory)
	{
#if RENDER_TEST
		Texture.StoredFormat = GL_RGBA8;
		Texture.SrcFormat = GL_RGBA;
#endif
	}

	return(Texture);
}

internal void 
TextureUnLoad(texture *Texture)
{
	if(Texture->Memory)
	{
		stbi_image_free(Texture->Memory);
	}
}

