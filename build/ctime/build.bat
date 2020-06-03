@echo off
setlocal EnableDelayedExpansion

rem use: build [release]

set COMMON_FLAGS= -Ob2 -Oi -favor:blend -fp:fast -Qpar -arch:AVX -Zc:inline -nologo

IF /I "%1" == "release" (
    set COMPILER_FLAGS= -O2 -Ot -GL !COMMON_FLAGS!
    cl !COMPILER_FLAGS! ctime.c -link -incremental:no -opt:ref winmm.lib
    move /Y ctime.exe ..
) ELSE (
    set COMPILER_FLAGS= -Od -Zi -W3 !COMMON_FLAGS!
    cl !COMPILER_FLAGS! ctime.c -link winmm.lib
)

del ctime.obj

set COMMON_FLAGS=
set COMPILER_FLAGS=

endlocal
