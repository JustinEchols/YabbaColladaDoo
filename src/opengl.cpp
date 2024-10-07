
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
		Assert(0);
	}

	glCompileShader(FSHandle);
	glGetShaderiv(FSHandle, GL_COMPILE_STATUS, &FSIsValid);
	if(!FSIsValid)
	{
		glGetShaderInfoLog(FSHandle, 512, 0, Buffer);
		printf("ERROR: Fragment Shader Compile Failed\n %s", Buffer);
		Assert(0);
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
UniformBoolSet(u32 ShaderProgram, char *UniformName, b32 B32)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	Assert(UniformLocation != -1);
	glUniform1i(UniformLocation, B32);
}

internal void
UniformU32Set(u32 ShaderProgram, char *UniformName, u32 U32)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	Assert(UniformLocation != -1);
	glUniform1ui(UniformLocation, U32);
}

internal void
UniformF32Set(u32 ShaderProgram, char *UniformName, f32 F32)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	Assert(UniformLocation != -1);
	glUniform1f(UniformLocation, F32);
}

internal void
UniformV3Set(u32 ShaderProgram, char *UniformName, v3 V)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	Assert(UniformLocation != -1);
	glUniform3fv(UniformLocation, 1, &V.E[0]);
}
internal void
UniformV4Set(u32 ShaderProgram, char *UniformName, v4 V)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	Assert(UniformLocation != -1);
	glUniform4fv(UniformLocation, 1, &V.E[0]);
}

internal void
UniformMatrixArraySet(u32 ShaderProgram, char *UniformName, mat4 *M, u32 Count)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	Assert(UniformLocation != -1);
	glUniformMatrix4fv(UniformLocation, Count, GL_TRUE, &M[0].E[0][0]);
}

internal void
UniformMatrixSet(u32 ShaderProgram, char *UniformName, mat4 M)
{
	s32 UniformLocation = glGetUniformLocation(ShaderProgram, UniformName);
	Assert(UniformLocation != -1);
	glUniformMatrix4fv(UniformLocation, 1, GL_TRUE, &M.E[0][0]);
}

internal void
OpenGLAllocateTexture(texture *Texture)
{
	if(Texture->Memory)
	{
		glGenTextures(1, &Texture->Handle);
		glBindTexture(GL_TEXTURE_2D, Texture->Handle);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D,
					0,
					Texture->Format,
					Texture->Width,
					Texture->Height,
					0, 
					Texture->Format,
					GL_UNSIGNED_BYTE, 
					Texture->Memory);

		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

internal void
OpenGLAllocateAnimatedModel(model *Model, u32 ShaderProgram)
{
	for(u32 MeshIndex = 0; MeshIndex < Model->MeshCount; ++MeshIndex)
	{
		s32 ExpectedAttributeCount = 0;

		mesh *Mesh = Model->Meshes + MeshIndex;

		glGenVertexArrays(1, &Model->VA[MeshIndex]);
		glBindVertexArray(Model->VA[MeshIndex]);

		glGenBuffers(1, &Model->VB[MeshIndex]);
		glBindBuffer(GL_ARRAY_BUFFER, Model->VB[MeshIndex]);
		glBufferData(GL_ARRAY_BUFFER, Mesh->VertexCount * sizeof(vertex), Mesh->Vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)OffsetOf(vertex, N));
		glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(vertex), (void *)OffsetOf(vertex, UV));
		glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(vertex), (void *)OffsetOf(vertex, JointInfo));
		glVertexAttribIPointer(4, 3, GL_UNSIGNED_INT, sizeof(vertex), (void *)(OffsetOf(vertex, JointInfo) + OffsetOf(joint_info, JointIndex)));
		glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(OffsetOf(vertex, JointInfo) + OffsetOf(joint_info, Weights)));

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glEnableVertexAttribArray(4);
		glEnableVertexAttribArray(5);

		ExpectedAttributeCount = 6;

		GLIBOInit(&Model->IBO[MeshIndex], Mesh->Indices, Mesh->IndicesCount);
		glBindVertexArray(0);

		s32 AttrCount;
		glGetProgramiv(ShaderProgram, GL_ACTIVE_ATTRIBUTES, &AttrCount);
		Assert(ExpectedAttributeCount == AttrCount);

		char Name[256];
		s32 Size = 0;
		GLsizei Length = 0;
		GLenum Type;
		for(s32 i = 0; i < AttrCount; ++i)
		{
			glGetActiveAttrib(ShaderProgram, i, sizeof(Name), &Length, &Size, &Type, Name);
			printf("Attribute:%d\nName:%s\nSize:%d\n\n", i, Name, Size);
		}
	}
}

internal void
OpenGLAllocateModel(model *Model, u32 ShaderProgram)
{
	for(u32 MeshIndex = 0; MeshIndex < Model->MeshCount; ++MeshIndex)
	{
		s32 ExpectedAttributeCount = 0;

		mesh *Mesh = Model->Meshes + MeshIndex;

		glGenVertexArrays(1, &Model->VA[MeshIndex]);
		glBindVertexArray(Model->VA[MeshIndex]);

		glGenBuffers(1, &Model->VB[MeshIndex]);
		glBindBuffer(GL_ARRAY_BUFFER, Model->VB[MeshIndex]);
		glBufferData(GL_ARRAY_BUFFER, Mesh->VertexCount * sizeof(vertex), Mesh->Vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)OffsetOf(vertex, N));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)OffsetOf(vertex, UV));

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		ExpectedAttributeCount = 3;

		GLIBOInit(&Model->IBO[MeshIndex], Mesh->Indices, Mesh->IndicesCount);
		glBindVertexArray(0);

		s32 AttrCount;
		glGetProgramiv(ShaderProgram, GL_ACTIVE_ATTRIBUTES, &AttrCount);
		Assert(ExpectedAttributeCount == AttrCount);

		char Name[256];
		s32 Size = 0;
		GLsizei Length = 0;
		GLenum Type;
		for(s32 i = 0; i < AttrCount; ++i)
		{
			glGetActiveAttrib(ShaderProgram, i, sizeof(Name), &Length, &Size, &Type, Name);
			printf("Attribute:%d\nName:%s\nSize:%d\n\n", i, Name, Size);
		}
	}
}

internal void
OpenGLDrawAnimatedModel(model *Model, u32 ShaderProgram, mat4 M)
{
	//mat4 M = Mat4Translate(Model->Basis.O) * Mat4(Model->Basis.X, Model->Basis.Y, Model->Basis.Z);

	v3 LightDir = V3(0.0f, 0.0f, 1.0f);

	glUseProgram(ShaderProgram);
	//UniformV3Set(ShaderProgram, "LightDir", LightDir);
	UniformBoolSet(ShaderProgram, "Texture", 0);
	UniformMatrixSet(ShaderProgram, "Model", M);
	for(u32 MeshIndex = 0; MeshIndex < Model->MeshCount; ++MeshIndex)
	{
		mesh *Mesh = Model->Meshes + MeshIndex;
		if(FlagIsSet(Mesh, MaterialFlag_Diffuse))
		{
			UniformBoolSet(ShaderProgram, "UsingTexture", true);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Mesh->DiffuseTexture);
			glBindVertexArray(Model->VA[MeshIndex]);
			UniformMatrixArraySet(ShaderProgram, "Transforms", Mesh->ModelSpaceTransforms, Mesh->JointCount);
			//UniformV4Set(ShaderProgram, "Diffuse", Mesh->MaterialSpec.Diffuse);
			//UniformV4Set(ShaderProgram, "Specular", Mesh->MaterialSpec.Specular);
			//UniformF32Set(ShaderProgram, "Shininess", Mesh->MaterialSpec.Shininess);
			glDrawElements(GL_TRIANGLES, Mesh->IndicesCount, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		else
		{
			UniformBoolSet(ShaderProgram, "UsingTexture", false);
			glBindVertexArray(Model->VA[MeshIndex]);
			UniformMatrixArraySet(ShaderProgram, "Transforms", Mesh->ModelSpaceTransforms, Mesh->JointCount);
			UniformV4Set(ShaderProgram, "Diffuse", Mesh->MaterialSpec.Diffuse);
			//UniformV4Set(ShaderProgram, "Specular", Mesh->MaterialSpec.Specular);
			//UniformF32Set(ShaderProgram, "Shininess", Mesh->MaterialSpec.Shininess);
			glDrawElements(GL_TRIANGLES, Mesh->IndicesCount, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
	}
}

internal void
OpenGLDrawModel(model *Model, u32 ShaderProgram, mat4 Transform)
{
	//mat4 Transform = Mat4Translate(Model->Basis.O) * R * Scale;
	glUseProgram(ShaderProgram);
	UniformBoolSet(ShaderProgram, "Texture", 0);
	UniformMatrixSet(ShaderProgram, "Model", Transform);
	for(u32 MeshIndex = 0; MeshIndex < Model->MeshCount; ++MeshIndex)
	{
		mesh *Mesh = Model->Meshes + MeshIndex;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Mesh->DiffuseTexture);
		glBindVertexArray(Model->VA[MeshIndex]);
		glDrawElements(GL_TRIANGLES, Mesh->IndicesCount, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}


