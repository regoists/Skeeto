@echo off
setlocal
for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
set "TESTROOT=%TEMP%\skeeto-mainui-test-%RANDOM%-%RANDOM%"
mkdir "%TESTROOT%\left4dead2"
cl /nologo /EHsc /std:c++17 /utf-8 /W3 /D_CRT_SECURE_NO_WARNINGS /DSKEETO_MAINUI_SERVICE_TEST "%~dp0tests\mainui_services_test.cpp" "%~dp0core\skeeto_mainui_services.cpp" /Fo"%TESTROOT%\\" /Fe:"%TESTROOT%\services_test.exe" /link ws2_32.lib user32.lib shell32.lib ole32.lib comdlg32.lib
if errorlevel 1 exit /b 1
"%TESTROOT%\services_test.exe" "%TESTROOT%" text
if errorlevel 1 exit /b 1
"%TESTROOT%\services_test.exe" "%TESTROOT%" write
if errorlevel 1 exit /b 1
"%TESTROOT%\services_test.exe" "%TESTROOT%" read
exit /b %errorlevel%
