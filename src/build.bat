@echo off

setlocal enabledelayedexpansion
where /Q cl.exe || (
  set __VSCMD_ARG_NO_LOGO=1
  for /f "tokens=*" %%i in ('"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath') do set "VS=%%i"
  if "!VS!" equ "" (
    echo ERROR: Visual Studio installation not found
    exit /b 1
  )  
  call "!VS!\VC\Auxiliary\Build\vcvarsall.bat" amd64 || exit /b 1
)
if "%VSCMD_ARG_TGT_ARCH%" neq "x64" (
  echo ERROR: please run this from MSVC x64 native tools command prompt, 32-bit target is not supported!
  exit /b 1
)

set MainFile=main.cpp
set CommonCompilerFlags= -Od -nologo -W4 -wd4100 -wd4201 -wd4996 -wd4505 -DGLEW_STATIC=1 -D__CRT_SECURE_NO_WARNINGS=1 -Zi
set CommonLinkerFlags=-incremental:no gdi32.lib user32.lib shell32.lib msvcrt.lib opengl32.lib winmm.lib glew32s.lib glfw3.lib
set IncludeDirectories= /I "../dependencies/GLFW64/include" /I "../dependencies/GLEW/include"
set LibDirectories= /LIBPATH:"../dependencies/GLFW64/lib-vc2019" /LIBPATH:"../dependencies/GLEW/lib/Release/x64"

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

cl ..\src\%MainFile% %CommonCompilerFlags% %IncludeDirectories% /link /NODEFAULTLIB:"LIBCMT" %LibDirectories% %CommonLinkerFlags%
popd
