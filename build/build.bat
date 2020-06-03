@echo off
setlocal EnableDelayedExpansion

rem use: build [cengine|sandbox] [release]

cls

set PROJECT=%1
set BUILD_TYPE=%2

if /I "!PROJECT!" == "release" (
    set BUILD_TYPE=!PROJECT!
    set PROJECT=
)

REM must have folders
if not exist ..\assets          mkdir ..\assets
if not exist ..\assets\audio    mkdir ..\assets\audio
if not exist ..\assets\shaders  mkdir ..\assets\shaders
if not exist ..\bin             mkdir ..\bin
if not exist ..\bin\obj         mkdir ..\bin\obj
if not exist ..\bin\obj\cengine mkdir ..\bin\obj\cengine
if not exist ..\bin\obj\sandbox mkdir ..\bin\obj\sandbox
if not exist ..\log             mkdir ..\log

set /A COMPILE_SANDBOX = 0
set /A COMPILE_CENGINE = 0

if /I "!PROJECT!" == "sandbox" (
    set /A COMPILE_SANDBOX = 1
    set /A COMPILE_CENGINE = 0
) else if /I "!PROJECT!" == "cengine" (
    set /A COMPILE_SANDBOX = 0
    set /A COMPILE_CENGINE = 1
) else if "!PROJECT!" == "" (
    set /A COMPILE_SANDBOX = 1
    set /A COMPILE_CENGINE = 1
)

ctime.exe -begin full.time

REM cengine
if !COMPILE_CENGINE! == 1 (
    set OPTIMIZATION= -Ob2 -Oi -favor:blend
    set CODE_GENERATION= -fp:fast -Qpar -arch:AVX
    set LANGUAGE= -Zc:wchar_t- -Zc:inline
    set DIAGNOSTICS= -W3
    set IMPORT_LIBS= User32.lib Ole32.lib
    set LINKER= -link
    set LINKING=
    set INPUT_FILES= cengine\*.c cengine\core\*.c cengine\gpu\core\*.c cengine\gpu\*.c cengine\math\*.c cengine\sound\*.c cengine\tools\*.c
    set PREPROCESSOR= -Icengine -D_CENGINE_DEV
    set MISCELLANEOUS= -MP -TC
    set OUTPUT_FILES= -Fo:bin\obj\cengine\ -Fe:bin\cengine.dll

    if /I "!BUILD_TYPE!" == "release" (
        set OPTIMIZATION= !OPTIMIZATION! -O2 -Ot
        set CODE_GENERATION= !CODE_GENERATION! -GL
        set LINKING= !LINKING! -MT -LD
        set LINKER= !LINKER! -incremental:no -opt:ref
        set LANGUAGE= !LANGUAGE!
    ) else (
        set OPTIMIZATION= !OPTIMIZATION! -Od
        set LINKING= !LINKING! -MTd -LDd
        set OUTPUT_FILES= !OUTPUT_FILES! -Fd:bin\cengine.pdb
        set LANGUAGE= !LANGUAGE! -ZI
    )
    
    ctime.exe -begin cengine.time

    pushd ..
        echo ==========================    Compiling cengine...    ==========================
        cl !OPTIMIZATION! !CODE_GENERATION! !PREPROCESSOR! !LANGUAGE! !MISCELLANEOUS! !LINKING! !DIAGNOSTICS! !OUTPUT_FILES! !INPUT_FILES! !LINKER! !IMPORT_LIBS! -nologo
    popd

    ctime.exe -end cengine.time

    echo.
)

REM sandbox
if !COMPILE_SANDBOX! == 1 (
    set OPTIMIZATION= -Ob2 -Oi -favor:blend
    set CODE_GENERATION= -fp:fast -Qpar -arch:AVX
    set LANGUAGE= -Zc:wchar_t- -Zc:inline
    set DIAGNOSTICS= -W3
    set IMPORT_LIBS= User32.lib bin\cengine.lib
    set LINKING=
    set LINKER= -link
    set INPUT_FILES= sandbox\*.c
    set PREPROCESSOR= -Icengine\
    set MISCELLANEOUS= -MP -TC
    set OUTPUT_FILES= -Fo:bin\obj\sandbox\ -Fe:bin\sandbox.exe 

    if /I "!BUILD_TYPE!" == "release" (
        set OPTIMIZATION= !OPTIMIZATION! -O2 -Ot
        set CODE_GENERATION= !CODE_GENERATION! -GL
        set LINKING= !LINKING! -MT
        set LINKER= !LINKER! -incremental:no -opt:ref
        set LANGUAGE= !LANGUAGE!
    ) else (
        set OPTIMIZATION= !OPTIMIZATION! -Od
        set LINKING= !LINKING! -MTd
        set OUTPUT_FILES= !OUTPUT_FILES! -Fd:bin\sandbox.pdb
        set LANGUAGE= !LANGUAGE! -ZI
    )
    
    ctime.exe -begin sandbox.time

    pushd ..
        echo ==========================    Compiling sandbox...    ==========================
        cl !OPTIMIZATION! !CODE_GENERATION! !PREPROCESSOR! !LANGUAGE! !MISCELLANEOUS! !DIAGNOSTICS! !LINKING! !INPUT_FILES! !OUTPUT_FILES! !LINKER! !IMPORT_LIBS! -nologo
    popd

    ctime.exe -end sandbox.time

    echo.
)

ctime.exe -end full.time
endlocal
