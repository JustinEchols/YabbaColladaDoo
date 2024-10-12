#if !defined(OPENGL_H)

struct opengl
{
	char *RendererName;
	char *RendererVersion;

	s32 MaxCombinedTextureImageUnits;
	s32 MaxCubeMapTextureSize;
	s32 MaxDrawBuffers;
	s32 MaxFragmentUniformComponents;
	s32 MaxTextureImageUnits;
	s32 MaxTextureSize;
	s32 MaxVaryingFloats;
	s32 MaxVertexAttribs;
	s32 MaxVertexTextureImageUnits;
	s32 MaxVertexUniformComponents;
	s32 MaxViewportDims;
	s32 Stereo;

};

#define OPENGL_H
#endif
