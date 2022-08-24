@echo off
SET SCRIPT_PATH=%~dp0
pushd .bin
SET COMPILER_PATH=%SCRIPT_PATH%\tools\tcc
SET CC=%COMPILER_PATH%\tcc
echo +----------------------------+
echo +   Compile simple C file    +
echo +----------------------------+
echo %2
%CC% -o %SCRIPT_PATH%.bin\%1.exe %2 -I%SCRIPT_PATH%/shared -Wall -Wextra -std=c99
IF %ERRORLEVEL% EQU 0 ( 
echo +----------------------------+
echo +           Output           +
echo +----------------------------+
echo %SCRIPT_PATH%.bin\%1.exe
echo +----------------------------+
) ELSE (
echo +----------------------------+
echo +     Compilation Error!     +
echo +----------------------------+
echo Error code: %ERRORLEVEL%
echo +----------------------------+
)
popd