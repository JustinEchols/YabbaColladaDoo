
struct window
{
	GLFWwindow *Handle;
	s32 Width, Height;
};

internal void
GLFWErrorCallback(s32 ErrorCode, const char *Message)
{
	printf("GLFW Error: Code %d Message %s", ErrorCode, Message);
}

internal void
GLFWFrameBufferResizeCallBack(GLFWwindow *Window, s32 Width, s32 Height)
{
	glViewport(0, 0, Width, Height);
}

internal void GLAPIENTRY
GLDebugCallback(GLenum Source, GLenum Type, GLuint ID, GLenum Severity, GLsizei Length,
		const GLchar *Message, const void *UserParam)
{
	printf("OpenGL Debug Callback: %s\n", Message);
}

// TODO(Justin): Either pass in a void pointer or a type then cast 
internal void
GLVbInitAndPopulate(u32 *VB, u32 VA, u32 Index, u32 ComponentCount, f32 *BufferData, u32 TotalCount)
{
	glGenBuffers(1, VB);
	glBindVertexArray(VA);
	glBindBuffer(GL_ARRAY_BUFFER, *VB);
	glBufferData(GL_ARRAY_BUFFER, TotalCount * sizeof(f32), BufferData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(Index);
	glVertexAttribPointer(Index, ComponentCount, GL_FLOAT, GL_FALSE, 0, (void *)0);
}

internal void
GLIBOInit(u32 *IBO, u32 *Indices, u32 IndicesCount)
{
	glGenBuffers(1, IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndicesCount * sizeof(u32), Indices, GL_STATIC_DRAW);
}

internal u32
GLProgramCreate(char *VS, char *FS)
{
	u32 VSHandle;
	u32 FSHandle;

	VSHandle = glCreateShader(GL_VERTEX_SHADER);
	FSHandle = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(VSHandle, 1, &VS, 0);
	glShaderSource(FSHandle, 1, &FS, 0);

	b32 VSIsValid = false;
	b32 FSIsValid = false;
	char Buffer[512];

	glCompileShader(VSHandle);
	glGetShaderiv(VSHandle, GL_COMPILE_STATUS, &VSIsValid);
	if(!VSIsValid)
	{
		glGetShaderInfoLog(VSHandle, 512, 0, Buffer);
		printf("ERROR: Vertex Shader Compile Failed\n %s", Buffer);
	}

	glCompileShader(FSHandle);
	glGetShaderiv(FSHandle, GL_COMPILE_STATUS, &FSIsValid);
	if(!FSIsValid)
	{
		glGetShaderInfoLog(FSHandle, 512, 0, Buffer);
		printf("ERROR: Fragment Shader Compile Failed\n %s", Buffer);
	}

	u32 Program;
	Program = glCreateProgram();
	glAttachShader(Program, VSHandle);
	glAttachShader(Program, FSHandle);
	glLinkProgram(Program);
	glValidateProgram(Program);

	b32 ProgramIsValid = false;
	glGetProgramiv(Program, GL_LINK_STATUS, &ProgramIsValid);
	if(!ProgramIsValid)
	{
		glGetProgramInfoLog(Program, 512, 0, Buffer);
		printf("ERROR: Program link failed\n %s", Buffer);
	}

	glDeleteShader(VSHandle);
	glDeleteShader(FSHandle);

	u32 Result = Program;

	return(Result);
}

internal void
UniformMatrixSet(u32 ShaderProgram, char *UniformName, mat4 M)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	glUniformMatrix4fv(UniformLocation, 1, GL_TRUE, &M.E[0][0]);
}

internal void
UniformV3Set(u32 ShaderProgram, char *UniformName, v3 V)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	glUniform3fv(UniformLocation, 1, &V.E[0]);
}

internal void
AttributesInterleave(f32 *BufferData, mesh *Mesh)
{
	u32 BufferIndex = 0;

	u32 Stride = 3;
	u32 UVStride = 2;
	for(u32 Index = 0; Index < Mesh->PositionsCount/3; ++Index)
	{
		BufferData[BufferIndex++] = Mesh->Positions[Stride * Index];
		BufferData[BufferIndex++] = Mesh->Positions[Stride * Index + 1];
		BufferData[BufferIndex++] = Mesh->Positions[Stride * Index + 2];

		BufferData[BufferIndex++] = Mesh->Normals[Stride * Index];
		BufferData[BufferIndex++] = Mesh->Normals[Stride * Index + 1];
		BufferData[BufferIndex++] = Mesh->Normals[Stride * Index + 2];

		BufferData[BufferIndex++] = Mesh->UV[UVStride * Index];
		BufferData[BufferIndex++] = Mesh->UV[UVStride * Index + 1];
	}
}