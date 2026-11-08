@echo off
setlocal
for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
set "TESTROOT=%TEMP%\skeeto-radar-settings-%RANDOM%-%RANDOM%"
mkdir "%TESTROOT%"
cl /nologo /EHsc /std:c++17 /utf-8 /W3 /WX /D_CRT_SECURE_NO_WARNINGS "%~dp0..\..\tools\test_supply_radar_settings.cpp" /Fo"%TESTROOT%\\" /Fe:"%TESTROOT%\settings.exe"
if errorlevel 1 exit /b 1
"%TESTROOT%\settings.exe"
exit /b %errorlevel%
