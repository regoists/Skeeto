@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0.."
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "UI_TEST_VS=%%I"
if not defined UI_TEST_VS exit /b 1
call "%UI_TEST_VS%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
if errorlevel 1 exit /b 1
if not exist "archive\测试\本地化" mkdir "archive\测试\本地化"
cl /nologo /EHsc /std:c++17 /utf-8 /W4 /WX /Fe"archive\测试\本地化\test_ui_text.exe" /Fo"archive\测试\本地化\test_ui_text.obj" tools\test_ui_text.cpp
if errorlevel 1 exit /b 1
archive\测试\本地化\test_ui_text.exe
exit /b %errorlevel%
