@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0.."
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "NAV_TEST_VS=%%I"
if not defined NAV_TEST_VS exit /b 1
call "%NAV_TEST_VS%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
if errorlevel 1 exit /b 1
if not exist "archive\测试\小地图圆形" mkdir "archive\测试\小地图圆形"
cl /nologo /EHsc /std:c++17 /utf-8 /W3 /WX /Fe"archive\测试\小地图圆形\test_minimap_disc.exe" /Fo"archive\测试\小地图圆形\test_minimap_disc.obj" tools\test_minimap_disc.cpp
if errorlevel 1 exit /b 1
archive\测试\小地图圆形\test_minimap_disc.exe
exit /b %errorlevel%
