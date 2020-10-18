@echo off
setlocal EnableDelayedExpansion

rem use: build [engine|sandbox] [release|nsight]

cls

set PROJECT=%1
set BUILD_TYPE=%2

if /I "!PROJECT!" == "release" (
    set BUILD_TYPE=!PROJECT!
    set PROJECT=
) else if /I "!PROJECT!" == "nsight" (
    set BUILD_TYPE=!PROJECT!
    set PROJECT=
)

REM must have folders
if not exist ..\assets          mkdir ..\assets
if not exist ..\assets\audio    mkdir ..\assets\audio
if not exist ..\assets\shaders  mkdir ..\assets\shaders
if not exist ..\bin             mkdir ..\bin
if not exist ..\bin\obj         mkdir ..\bin\obj
if not exist ..\bin\obj\engine  mkdir ..\bin\obj\engine
if not exist ..\bin\obj\sandbox mkdir ..\bin\obj\sandbox
if not exist ..\log             mkdir ..\log

set /A COMPILE_SANDBOX = 0
set /A COMPILE_CENGINE = 0

if /I "!PROJECT!" == "sandbox" (
    set /A COMPILE_SANDBOX = 1
) else if /I "!PROJECT!" == "engine" (
    set /A COMPILE_CENGINE = 1
) else if "!PROJECT!" == "" (
    set /A COMPILE_SANDBOX = 1
    set /A COMPILE_CENGINE = 1
)

ctime -begin full.time

REM engine
if !COMPILE_CENGINE! == 1 (
    set OPTIMIZATION= -Ob2 -Oi -favor:blend
    set CODE_GENERATION= -fp:fast -Qpar -arch:AVX
    set LANGUAGE= -Zc:wchar_t- -Zc:inline -std:c++17
    set DIAGNOSTICS= -W3
    set IMPORT_LIBS=
    set LINKER= -link
    set LINKING=
    set INPUT_FILES=
    set PREPROCESSOR= -Iengine -D_ENGINE_DEV
    set MISCELLANEOUS= -TP
    set OUTPUT_FILES= -Fo:bin\obj\engine\ -Fe:bin\engine.dll -Fp:bin\obj\engine\engine.pch

    REM first 'for' needs just for getting full filename
    for /F %%i in ('dir /A-D /S /B ..\engine\core\pch.cpp') do (
        set PCH_FILE=%%i
    )
    for /F %%i in ('dir /A-D /S /B ..\engine\*.cpp') do (
        set FILE=%%i
        if /I "!FILE!" NEQ "!PCH_FILE!" (
            set INPUT_FILES=!INPUT_FILES! !FILE!
        )
    )

    if /I "!BUILD_TYPE!" == "release" (
        set OPTIMIZATION= !OPTIMIZATION! -O2 -Ot
        set CODE_GENERATION= !CODE_GENERATION! -GL
        set LINKING= !LINKING! -MT -LD
        set LINKER= !LINKER! -incremental:no -opt:ref
        REM Temporary, added for debuging release build
        set OUTPUT_FILES= !OUTPUT_FILES! -Fd:bin\engine.pdb
        set LANGUAGE= !LANGUAGE! -Zi
    ) else if /I "!BUILD_TYPE!" == "nsight" (
        set OPTIMIZATION= !OPTIMIZATION! -Od
        set LINKING= !LINKING! -MT -LD
        set OUTPUT_FILES= !OUTPUT_FILES! -Fd:bin\engine.pdb
        set LANGUAGE= !LANGUAGE! -Zi
    ) else (
        set OPTIMIZATION= !OPTIMIZATION! -Od
        set LINKING= !LINKING! -MTd -LDd
        set OUTPUT_FILES= !OUTPUT_FILES! -Fd:bin\engine.pdb
        set LANGUAGE= !LANGUAGE! -Zi
    )

    ctime -begin engine.time

    pushd ..
        echo ==========================    Compiling engine...    ==========================
        cl !OPTIMIZATION! !CODE_GENERATION! !PREPROCESSOR! !LANGUAGE! !MISCELLANEOUS! -Yccore\pch.h !LINKING! !DIAGNOSTICS! engine\core\pch.cpp !OUTPUT_FILES! !LINKER! !IMPORT_LIBS! -nologo
        cl !OPTIMIZATION! !CODE_GENERATION! !PREPROCESSOR! !LANGUAGE! !MISCELLANEOUS! -Yucore\pch.h !LINKING! !DIAGNOSTICS! !INPUT_FILES! !OUTPUT_FILES! !LINKER! bin\obj\engine\pch.obj !IMPORT_LIBS! -nologo
    popd

    ctime -end engine.time

    echo.
)

REM sandbox
if !COMPILE_SANDBOX! == 1 (
    set OPTIMIZATION= -Ob2 -Oi -favor:blend
    set CODE_GENERATION= -fp:fast -Qpar -arch:AVX
    set LANGUAGE= -Zc:wchar_t- -Zc:inline -std:c++17
    set DIAGNOSTICS= -W3
    set IMPORT_LIBS= bin\engine.lib
    set LINKER= -link
    set LINKING=
    set INPUT_FILES=
    set PREPROCESSOR= -Iengine -Isandbox
    set MISCELLANEOUS= -TP
    set OUTPUT_FILES= -Fo:bin\obj\sandbox\ -Fe:bin\sandbox.exe

    for /F %%i in ('dir /A-D /S /B ..\sandbox\*.cpp') do (
        set INPUT_FILES=!INPUT_FILES! %%i
    )

    if /I "!BUILD_TYPE!" == "release" (
        set OPTIMIZATION= !OPTIMIZATION! -O2 -Ot
        set CODE_GENERATION= !CODE_GENERATION! -GL
        set LINKING= !LINKING! -MT
        set LINKER= !LINKER! -incremental:no -opt:ref
        REM Temporary, added for debuging release build
        set OUTPUT_FILES= !OUTPUT_FILES! -Fd:bin\sandbox.pdb
        set LANGUAGE= !LANGUAGE! -Zi
    ) else if /I "!BUILD_TYPE!" == "nsight" (
        set OPTIMIZATION= !OPTIMIZATION! -Od
        set LINKING= !LINKING! -MT
        set OUTPUT_FILES= !OUTPUT_FILES! -Fd:bin\sandbox.pdb
        set LANGUAGE= !LANGUAGE! -Zi
    ) else (
        set OPTIMIZATION= !OPTIMIZATION! -Od
        set LINKING= !LINKING! -MTd
        set OUTPUT_FILES= !OUTPUT_FILES! -Fd:bin\sandbox.pdb
        set LANGUAGE= !LANGUAGE! -Zi
    )
    
    ctime -begin sandbox.time

    pushd ..
        echo ==========================    Compiling sandbox...    ==========================
        cl !OPTIMIZATION! !CODE_GENERATION! !PREPROCESSOR! !LANGUAGE! !MISCELLANEOUS! !LINKING! !DIAGNOSTICS! !INPUT_FILES! !OUTPUT_FILES! !LINKER! !IMPORT_LIBS! -nologo
    popd

    ctime -end sandbox.time

    echo.
)

ctime -end full.time
endlocal
