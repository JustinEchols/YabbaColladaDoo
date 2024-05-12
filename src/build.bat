@echo off

set MainFile=main.cpp
set CommonCompilerFlags= -Od -nologo -W4 -wd4100 -wd4201 -wd4996 -wd4505 -DGLEW_STATIC=1 -D__CRT_SECURE_NO_WARNINGS=1 -Zi
set CommonLinkerFlags=-incremental:no gdi32.lib user32.lib shell32.lib msvcrt.lib opengl32.lib winmm.lib glew32s.lib glfw3.lib
set IncludeDirectories= /I "../dependencies/GLFW64/include" /I "../dependencies/GLEW/include"
set LibDirectories= /LIBPATH:"../dependencies/GLFW64/lib-vc2019" /LIBPATH:"../dependencies/GLEW/lib/Release/x64"

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

cl ..\src\%MainFile% %CommonCompilerFlags% %IncludeDirectories% /link /NODEFAULTLIB:"LIBCMT" %LibDirectories% %CommonLinkerFlags%
