@echo off

rem use: build

set OPTIMIZATION= -Ob2 -Oi -favor:blend -O2 -Ot
set CODE_GENERATION= -fp:fast -Qpar -arch:AVX -GL
set LANGUAGE= -Zc:wchar_t- -Zc:inline
set IMPORT_LIBS= winmm.lib
set LINKER= -link -incremental:no -opt:ref
set LINKING= -MT
set INPUT_FILES= ctime.c
set MISCELLANEOUS= -TC

echo ==========================    Compiling ctime...    ==========================
cl %OPTIMIZATION% %CODE_GENERATION% %LANGUAGE% %MISCELLANEOUS% %LINKING% %INPUT_FILES% %LINKER% %IMPORT_LIBS% -nologo

del /Q ctime.obj
move /Y ctime.exe ..
