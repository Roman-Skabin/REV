@echo off
setlocal EnableDelayedExpansion

REM use: build project config platform

set PROJECT=%1
set CONFIG=%2
set PLATFORM=%3

if /I "!CONFIG!"   EQU "" set CONFIG=debug
if /I "!PLATFORM!" EQU "" set PLATFORM=x64

if /I "!CONFIG!" NEQ "debug" (
if /I "!CONFIG!" NEQ "release" (
if /I "!CONFIG!" NEQ "nsight" (
    echo Syntax error: Invalid config name: expected debug or release or nsight, got !CONFIG!
    goto error
)))

if /I "!PLATFORM!" NEQ "x64" (
    echo Syntax error: Invalid platform name: expected x64, got !PLATFORM!
    goto error
)

if /I "!PROJECT!" NEQ "rev" (
if /I "!PROJECT!" NEQ "sandbox" (
if /I "!PROJECT!" NEQ "" (
    echo Syntax error: Invalid project name: expected rev or sandbox, got !PROJECT!
    goto error
)))

pushd ..
    if /I "!PROJECT!" NEQ "" (
        call devenv REV.sln /rebuild "%CONFIG%|%PLATFORM%" /project %PROJECT%
        goto done
    ) else (
        call devenv REV.sln /rebuild "%CONFIG%|%PLATFORM%"
        goto done
    )
popd

:error
echo use: build.bat project config platform

:done

endlocal
