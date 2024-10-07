#if !defined(SHADERS_H)

// WARNING(Justin): The max loop count must be a CONSTANT we cannot pass the
// joint count in and use it to cap the number of iterations of a loop.

// WARNING(Justin): If the compiler and linker determines that an attribute is
// not used IT WILL OPTIMIZE IT OUT OF THE PIPELINE AND CHAOS ENSUES.
// To detect this, everytime an attribute is initialized a counter can be
// incremented. After initializing all attributes use glGetProgramiv with
// GL_ACTIVE_ATTRIBUTES and ASSERT that the return value is equal to the
// counter.




struct shader
{
	u32 Handle;
	char VS[4096];
	char FS[4096];
};
#endif
