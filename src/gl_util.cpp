
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
UniformU32Set(u32 ShaderProgram, char *UniformName, u32 U32)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	glUniform1ui(UniformLocation, U32);
}

internal void
UniformF32Set(u32 ShaderProgram, char *UniformName, f32 F32)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	glUniform1f(UniformLocation, F32);
}


internal void
UniformV3Set(u32 ShaderProgram, char *UniformName, v3 V)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	glUniform3fv(UniformLocation, 1, &V.E[0]);
}
internal void
UniformV4Set(u32 ShaderProgram, char *UniformName, v4 V)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	glUniform4fv(UniformLocation, 1, &V.E[0]);
}

internal void
UniformMatrixArraySet(u32 ShaderProgram, char *UniformName, mat4 *M, u32 Count)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	glUniformMatrix4fv(UniformLocation, Count, GL_TRUE, &M[0].E[0][0]);
}

internal void
UniformMatrixSet(u32 ShaderProgram, char *UniformName, mat4 M)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	glUniformMatrix4fv(UniformLocation, 1, GL_TRUE, &M.E[0][0]);
}
