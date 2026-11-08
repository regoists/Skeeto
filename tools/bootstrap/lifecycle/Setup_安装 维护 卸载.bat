@echo off
setlocal EnableExtensions DisableDelayedExpansion
if not "%~1"=="" goto arguments
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Skeeto\Maintain.ps1" -InteractiveMenu
set "SKT_RESULT=%errorlevel%"
echo.
pause
exit /b %SKT_RESULT%
:arguments
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Skeeto\Maintain.ps1" %*
exit /b %errorlevel%
