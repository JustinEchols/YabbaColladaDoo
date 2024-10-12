
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

internal void
GLFWKeyCallBack(GLFWwindow *Handle, s32 Key, s32 ScanCode, s32 Action, s32 Mods)
{

	if((Action == GLFW_PRESS) || (Action == GLFW_REPEAT))
	{
		switch(Key)
		{
			case GLFW_KEY_W:
			{
				printf("W\n");
			} break;
			case GLFW_KEY_A:
			{
				printf("A\n");
			} break;
			case GLFW_KEY_S:
			{
				printf("S\n");
			} break;
			case GLFW_KEY_D:
			{
				printf("D\n");
			} break;
		};
	}
}
