@echo off
setlocal
for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "RADAR_TEST_VS=%%I"
if not defined RADAR_TEST_VS exit /b 1
call "%RADAR_TEST_VS%\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
set "RADAR_TEST_ROOT=%TEMP%\skeeto-radar-runtime-%RANDOM%-%RANDOM%"
mkdir "%RADAR_TEST_ROOT%"
cl /nologo /EHsc /std:c++17 /utf-8 /Od /Gy /RTC1 /W3 /D_CRT_SECURE_NO_WARNINGS "%~dp0..\..\tools\test_supply_radar_runtime.cpp" /Fo"%RADAR_TEST_ROOT%\\" /Fe:"%RADAR_TEST_ROOT%\runtime.exe" /link /OPT:REF user32.lib
if errorlevel 1 exit /b 1
"%RADAR_TEST_ROOT%\runtime.exe"
exit /b %errorlevel%
