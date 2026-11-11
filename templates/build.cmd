@echo off
rem Builds both templates as 32-bit DLLs with MSVC and runs the self-test. MIT, see LICENSE-MIT.
rem
rem   build\tmloader-plugin\TemplatePlugin.dll   TMLoader plugin
rem   build\dinput8-proxy\dinput8.dll            dinput8 proxy
rem   build\selftest.exe                         checks without any game
rem
rem Compiler: "cl" from PATH if it targets x86 (an "x86 Native Tools" or "x64_x86 Cross Tools"
rem prompt), else the vcvars script in %VCVARS% (e.g. ...\VC\Auxiliary\Build\vcvars32.bat),
rem else the newest Visual Studio / Build Tools found by vswhere.
rem "build.cmd notest" skips the self-test.
setlocal
cd /d "%~dp0"

call :havex86
if not errorlevel 1 goto build
if defined VCVARS goto usevcvars
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto nocompiler
if not exist build mkdir build
rem through a file: a quoted path with "(x86)" inside "for /f" breaks the parser
"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > build\vsinstall.txt
set /p VSINSTALL=<build\vsinstall.txt
del build\vsinstall.txt
if not defined VSINSTALL goto nocompiler
set "VCVARS=%VSINSTALL%\VC\Auxiliary\Build\vcvars32.bat"
:usevcvars
if not exist "%VCVARS%" (
  echo VCVARS points to a missing file: "%VCVARS%"
  exit /b 1
)
call "%VCVARS%" >nul || exit /b 1
call :havex86
if errorlevel 1 (
  echo "%VCVARS%" did not give an x86 compiler. Use vcvars32.bat or vcvarsamd64_x86.bat.
  exit /b 1
)

:build
for %%d in (build build\obj build\obj\plugin build\obj\proxy build\obj\test build\tmloader-plugin build\dinput8-proxy) do if not exist %%d mkdir %%d

set "DEFS=/DNDEBUG /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /DNOMINMAX"
set "CXX=cl /nologo /c /O2 /MT /GS /W4 /WX /EHsc /std:c++17 /utf-8 %DEFS% /Icommon"
set "LINKDLL=link /nologo /DLL /MACHINE:X86 /SAFESEH /DYNAMICBASE /NXCOMPAT /OPT:REF"
set "COMMON=common\log.cpp common\gamebuild.cpp common\hook.cpp common\frame_counter.cpp common\mod.cpp"

rem --- TMLoader plugin
%CXX% /Fo"build\obj\plugin\\" tmloader-plugin\plugin.cpp %COMMON% || exit /b 1
%LINKDLL% /OUT:build\tmloader-plugin\TemplatePlugin.dll /IMPLIB:build\obj\plugin\TemplatePlugin.lib build\obj\plugin\*.obj user32.lib kernel32.lib || exit /b 1

rem --- dinput8 proxy (exports from the .def file; the system DLL is loaded by full path)
%CXX% /Fo"build\obj\proxy\\" dinput8-proxy\proxy.cpp %COMMON% || exit /b 1
%LINKDLL% /DEF:dinput8-proxy\dinput8.def /OUT:build\dinput8-proxy\dinput8.dll /IMPLIB:build\obj\proxy\dinput8.lib build\obj\proxy\*.obj user32.lib kernel32.lib || exit /b 1

rem --- self-test
%CXX% /Fo"build\obj\test\\" tests\selftest.cpp common\log.cpp common\gamebuild.cpp common\hook.cpp common\frame_counter.cpp || exit /b 1
link /nologo /MACHINE:X86 /SAFESEH /DYNAMICBASE /NXCOMPAT /OUT:build\selftest.exe build\obj\test\*.obj user32.lib kernel32.lib d3d9.lib dxguid.lib || exit /b 1

if /i "%~1"=="notest" goto done
build\selftest.exe build\tmloader-plugin\TemplatePlugin.dll build\dinput8-proxy\dinput8.dll || exit /b 1
:done
echo build ok
exit /b 0

:havex86
rem errorlevel 0 when "cl" is on PATH and targets x86
where cl >nul 2>nul || exit /b 1
rem the banner ends in "for x86" (localised: "für x86", "pour x86" ...); x64 compilers say "x64"
cl 2>&1 | findstr /c:" x86" >nul || exit /b 1
exit /b 0

:nocompiler
echo No x86 MSVC compiler found. Open an "x86 Native Tools Command Prompt", or set VCVARS to vcvars32.bat,
echo or install the Visual Studio Build Tools with "Desktop development with C++".
exit /b 1
