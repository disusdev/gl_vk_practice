@echo off

REM +#############################+
REM #### SAVE SCRIPT DIRECTORY ####
REM +#############################+
SET SCRIPT_PATH=%~dp0
REM +#############################+

REM +#############################+
REM ##### CREATE TEMP FOLDERS #####
REM +#############################+
IF NOT EXIST .bin mkdir .bin
IF NOT EXIST .lib mkdir .lib
REM +#############################+

REM +#############################+
REM ##### GET COMPILE OPTIONS #####
REM +#############################+
SET CC=%SCRIPT_PATH%/tools/tdm-gcc/bin/gcc
REM ###############################
SET INCLUDES=-I%VULKAN_SDK%/Include^
             -I%SCRIPT_PATH%/shared^
             -I%SCRIPT_PATH%/external
SET DEFINES=-DSTBI_NO_SIMD
SET LIB_INCLUDES=
SET LIBS=-lkernel32^
         -lshell32^
         -luser32^
         -lgdi32
SET COMPILE_OPTIONS=-std=c99
                    @REM -Wall^
                    @REM -Wextra
SET ADD_TO_COMPILE=%SCRIPT_PATH%/external/rglfw.c^
                   %SCRIPT_PATH%/external/volk/volk.c
REM ###############################

REM +#############################+
REM ######## COMPILATION ##########
REM +#############################+
echo +----------------------------+
echo +   Compile simple C file    +
echo +----------------------------+
echo %2
%CC% -o %SCRIPT_PATH%.bin/%1.exe %2^
 %ADD_TO_COMPILE% %INCLUDES% %DEFINES% %LIB_INCLUDES%^
 %LIBS% %COMPILE_OPTIONS%
REM +#############################+

REM +#############################+
REM ####### ERROR TRACKING ########
REM +#############################+
IF %ERRORLEVEL% EQU 0 ( 
echo +----------------------------+
echo +           Output           +
echo +----------------------------+
echo %SCRIPT_PATH%.bin/%1.exe
echo +----------------------------+
EXIT 0
) ELSE (
echo +----------------------------+
echo +     Compilation Error!     +
echo +----------------------------+
echo Error code: %ERRORLEVEL%
echo +----------------------------+
EXIT %ERRORLEVEL% )
REM +#############################+