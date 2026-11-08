@echo off
setlocal EnableExtensions
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 1
set "VSINSTALL="
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
if not defined VSINSTALL exit /b 1
call "%VSINSTALL%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
if errorlevel 1 exit /b 1
set "OUT=%~dp0out"
if not "%~1"=="" set "OUT=%~f1"
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /LD /MT /O2 /EHsc /std:c++17 /utf-8 /W3 /WX ^
 /I"%~dp0..\..\include" /Fe"%OUT%\HelloMenu.dll" /Fo"%OUT%\\" /Fd"%OUT%\HelloMenu.pdb" ^
 "%~dp0plugin.cpp" /link /MACHINE:X86 /INCREMENTAL:NO /IMPLIB:"%OUT%\HelloMenu.lib"
exit /b %errorlevel%
