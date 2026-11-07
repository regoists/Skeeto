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
if not exist "archive\测试\导航预算" mkdir "archive\测试\导航预算"
cl /nologo /EHsc /std:c++17 /utf-8 /W4 /WX /Fe"archive\测试\导航预算\test_nav_budget.exe" /Fo"archive\测试\导航预算\test_nav_budget.obj" tools\test_nav_budget.cpp
if errorlevel 1 exit /b 1
archive\测试\导航预算\test_nav_budget.exe
exit /b %errorlevel%
