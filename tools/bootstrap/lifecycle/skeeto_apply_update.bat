@echo off
setlocal EnableExtensions DisableDelayedExpansion
chcp 65001 >nul
rem Both the released 1.x pump and layout2 pump sync this exact filename.
rem Run the staged engine itself: replacing this wrapper alone never leaves an
rem old root PowerShell engine applying a new manifest with old semantics.
set "SKT_ROOT=%~dp0"
rem New pumps invoke this wrapper inside the stage, leaving root files intact.
rem Released pumps still invoke the synchronized root wrapper.
if exist "%~dp0READY" if exist "%~dp0..\left4dead2.exe" if exist "%~dp0..\bin\engine.dll" set "SKT_ROOT=%~dp0..\"
if not exist "%SKT_ROOT%skeeto_update\READY" exit /b 0
if not exist "%SKT_ROOT%skeeto_update\Skeeto\Maintain.ps1" goto :missing
powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "%SKT_ROOT%skeeto_update\Skeeto\Maintain.ps1" -Mode Apply -GameRoot "%SKT_ROOT%." -NonInteractive
exit /b %errorlevel%
:missing
echo [Skeeto] Maintenance engine missing. READY kept. Extract the complete package and run Setup_安装 维护 卸载.bat.
exit /b 1
