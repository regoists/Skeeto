@echo off
setlocal
for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
if not defined VSINSTALL exit /b 1
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
set "TESTROOT=%TEMP%\skeeto-supply-radar-%RANDOM%-%RANDOM%"
mkdir "%TESTROOT%"
cl /nologo /EHsc /std:c++17 /utf-8 /W4 /D_CRT_SECURE_NO_WARNINGS "%~dp0..\..\tools\test_supply_radar_algorithms.cpp" /Fo"%TESTROOT%\\" /Fe:"%TESTROOT%\algorithms.exe"
if errorlevel 1 exit /b 1
"%TESTROOT%\algorithms.exe"
exit /b %errorlevel%
