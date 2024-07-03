@echo off

REM Find latest Visual Studio installation via vswhere.exe to use MSVC toolset from command line.
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

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM Conversion non-optimized
REM cl ..\src\%MainFile% -Od -nologo -W4 -wd4100 -wd4201 -wd4505 -Zi -D_CRT_SECURE_NO_WARNINGS=1

REM Conversion optimized
cl ..\src\%MainFile% -O2 -nologo -W4 -wd4100 -wd4201 -wd4505 -Zi -D_CRT_SECURE_NO_WARNINGS=1
popd


