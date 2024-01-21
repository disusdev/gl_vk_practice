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
IF NOT EXIST .obj mkdir .obj
IF NOT EXIST .cache mkdir .cache
REM +#############################+

REM +#############################+
REM ##### GET COMPILE OPTIONS #####
REM +#############################+
SET CC=%SCRIPT_PATH%/tools/tdm-gcc/bin/gcc
SET CC_PLUS=%SCRIPT_PATH%/tools/tdm-gcc/bin/g++
SET AR=%SCRIPT_PATH%/tools/tdm-gcc/bin/ar
SET CV2PDB=%SCRIPT_PATH%/tools/cv2pdb/cv2pdb
REM ###############################
SET INCLUDES=-I%VULKAN_SDK%/Include^
             -I%SCRIPT_PATH%/shared^
             -I%SCRIPT_PATH%/external
SET DEFINES=
rem -DSTBI_NO_SIMD
SET LIB_INCLUDES=
SET LIBS=-lkernel32^
         -lshell32^
         -luser32^
         -lgdi32
SET COMPILE_OPTIONS=-std=c99 -w -g
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
%CC_PLUS% -c %SCRIPT_PATH%/external/gli/gli_c_wrapper.cpp -o %SCRIPT_PATH%/.obj/gli_c_wrapper.o %INCLUDES% -I%SCRIPT_PATH%/external/gli -I%SCRIPT_PATH%/external/gli/external
%AR% rcs %SCRIPT_PATH%.lib/gli.lib %SCRIPT_PATH%/.obj/gli_c_wrapper.o

%CC% -o %SCRIPT_PATH%.bin/%1.exe %2^
 %ADD_TO_COMPILE% %INCLUDES% %DEFINES% -L%SCRIPT_PATH%.lib/ -lgli %LIB_INCLUDES%^
 %LIBS% %COMPILE_OPTIONS% -lstdc++
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

echo +----------------------------+
echo +    Generate debug info     +
echo +----------------------------+
%CV2PDB% %SCRIPT_PATH%.bin/%1.exe
echo %SCRIPT_PATH%.bin/%1.pdb
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