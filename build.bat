@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"

:: --- Unpack Arguments ----------------------------------------
for %%a in (%*) do set "%%a=1"
if not "%msvc%"=="1" if not "%clang%"=="1" set msvc=1
if not "%release%"=="1" set debug=1
if "%debug%"=="1"   set release=0 && echo [debug mode]
if "%release%"=="1" set debug=0 && echo [release mode]
if "%msvc%"=="1"    set clang=0 && echo [msvc compile]
if "%clang%"=="1"   set msvc=0 && echo [clang compile]

:: --- Define Compile/Link Lines -------------------------------
set cl_common=  /D_CRT_SECURE_NO_WARNINGS /I..\src\ /nologo /MT /GR- /EHa- /Oi /FC /Z7 /W4 /wd4100 /wd4189 /wd4201 /wd4310 /wd4456 /wd4505
set cl_debug=   call cl /Od /DEXCALIBUR_INTERNAL=1 /DEXCALIBUR_DEBUG=1 %cl_common%
set cl_release= call cl /O2 /DEXCALIBUR_INTERNAL=1 /DEXCALIBUR_DEBUG=0 %cl_common%
set cl_link=    /link /opt:ref
set cl_out=     /out:

set clang_common=  -I..\src\ -gcodeview -fdiagnostics-absolute-paths -Wall -Wno-missing-braces -Wno-unused-function -Wno-unused-variable -Wno-writable-strings -Wno-format-security
set clang_debug=   call clang++ -g -O0 -DEXCALIBUR_INTERNAL=1 -DEXCALIBUR_DEBUG=1 %clang_common%
set clang_release= call clang++ -g -O2 -DEXCALIBUR_INTERNAL=0 -DEXCALIBUR_DEBUG=0 %clang_common%
set clang_link=    
set clang_out=     -o

:: --- Choose Compile Lines ------------------------------------
set link_dll =-DLL

if "%msvc%"=="1" set compile_debug=%cl_debug%
if "%msvc%"=="1" set compile_release=%cl_release%
if "%msvc%"=="1" set compile_link=%cl_link%
if "%msvc%"=="1" set compile_out=%cl_out%

if "%clang%"=="1" set compile_debug=%clang_debug%
if "%clang%"=="1" set compile_release=%clang_release%
if "%clang%"=="1" set compile_link=%clang_link%
if "%clang%"=="1" set compile_out=%clang_out%

if "%debug%"=="1"   set compile=%compile_debug%
if "%release%"=="1" set compile=%compile_release%

:: --- Prep Directories ----------------------------------------
if not exist build mkdir build

:: --- Build Everything ----------------------------------------
pushd build
%compile% ..\src\excalibur_game.cpp %compile_link% %link_dll% %compile_out%excalibur_game.dll || exit /b 1
%compile% ..\src\os\excalibur_os_win32.cpp %compile_link% %compile_out%excalibur.exe || exit /b 1
popd
