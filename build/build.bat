@echo off
setlocal EnableDelayedExpansion

cls

set CONFIG=%1
set PLATFORM=%2
set PROJECT=%3

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

if /I "!PROJECT!" NEQ "engine" (
if /I "!PROJECT!" NEQ "sandbox" (
if /I "!PROJECT!" NEQ "" (
    echo Syntax error: Invalid project name: expected engine or sandbox, got !PROJECT!
    goto error
)))

pushd ..
    if /I "!PROJECT!" NEQ "" (
        call devenv REV.sln /build "%CONFIG%|%PLATFORM%" /project %PROJECT%
        goto done
    ) else (
        call devenv REV.sln /build "%CONFIG%|%PLATFORM%"
        goto done
    )
popd

:error
echo use: build.bat config platform project

:done

endlocal
