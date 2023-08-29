@ECHO OFF

SET SRCS=drinn.c
SET EXE=drinn.exe

SET LIBS=-lWs2_32.lib -lKernel32.lib -lAdvapi32.lib -lIphlpapi.lib
SET PREPROCESSOR_DEFINITIONS=-D _CRT_SECURE_NO_WARNINGS

DEL /Q bin

ECHO Compiling %SRCS%

clang %PREPROCESSOR_DEFINITIONS% %SRCS% -g -o bin\%EXE% %LIBS%

