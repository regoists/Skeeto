@echo off
rem Called only by the MSVC x86 src/skeeto/build.bat entry point.
set "BOOT_OBJ=%OBJ%\steam_bootstrap"
if not exist "%BOOT_OBJ%" mkdir "%BOOT_OBJ%"
if not exist "%ROOT%runtime\bootstrap" mkdir "%ROOT%runtime\bootstrap"
python "%ROOT%tools\generate_wininet_proxy.py" --system "%WINDIR%\SysWOW64\wininet.dll" --output "%BOOT_OBJ%\generated"
if errorlevel 1 exit /b 1
ml /nologo /c /coff /Fo "%BOOT_OBJ%\thunks.obj" "%BOOT_OBJ%\generated\wininet_thunks.asm"
if errorlevel 1 exit /b 1
cl /nologo /LD /MT /O2 /EHsc /std:c++17 /utf-8 /W4 /WX ^
 /I"%ROOT%src\skeeto\core" /I"%BOOT_OBJ%\generated" ^
 /Fo"%BOOT_OBJ%\\" /Fe"%BOOT_OBJ%\wininet.dll" ^
 "%ROOT%src\skeeto\bootstrap\wininet_proxy.cpp" "%BOOT_OBJ%\thunks.obj" ^
 /link /MACHINE:X86 /INCREMENTAL:NO /DEF:"%BOOT_OBJ%\generated\wininet.def" /IGNORE:4222
if errorlevel 1 exit /b 1
copy /y "%BOOT_OBJ%\wininet.dll" "%ROOT%runtime\bootstrap\wininet.dll" >nul
if errorlevel 1 exit /b 1
python "%ROOT%tools\package_bootstrap_migration.py"
exit /b %errorlevel%
