@echo off

set MainFile=main.cpp
set CommonCompilerFlags= -Od -nologo -W4 -wd4100 -wd4201 -wd4996 -wd4505 -Zi -DGLEW_STATIC=1 -D__CRT_SECURE_NO_WARNINGS=1
set CommonLinkerFlags=-incremental:no gdi32.lib user32.lib shell32.lib msvcrt.lib opengl32.lib winmm.lib glew32s.lib glfw3.lib

set IncludeDirectories= /I "../dependencies/GLFW64/include" /I "../dependencies/GLEW/include"
set LibDirectories= /LIBPATH:"../dependencies/GLFW64/lib-vc2019" /LIBPATH:"../dependencies/GLEW/lib/Release/x64"

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM Render test compile
cl ..\src\%MainFile% %CommonCompilerFlags% /Fe"render_test" -DRENDER_TEST=1 %IncludeDirectories% /link /NODEFAULTLIB:"LIBCMT" %LibDirectories% %CommonLinkerFlags%

REM Conversion only compile
cl ..\src\%MainFile% -Od -MTd -W4 -Gm- -GR- -EHa -Oi -ZI -wd4100 -wd4201 -wd4505 -D_CRT_SECURE_NO_WARNINGS=1 /Fe"converter" -DRENDER_TEST=0 /link -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib
popd


