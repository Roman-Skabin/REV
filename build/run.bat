@echo off
setlocal EnableDelayedExpansion

set CONFIG=%1
set PLATFORM=%2

if /I "!CONFIG!" NEQ "debug" (
if /I "!CONFIG!" NEQ "release" (
if /I "!CONFIG!" NEQ "nsight" (
    echo Syntax error: Invalid config name: expected debug or release or nsight, got !CONFIG!
    goto error
)))

if /I "!PLATFORM!" NEQ "windows" (
    echo Syntax error: Invalid platform name: expected x64, got !PLATFORM!
    goto error
)

pushd ..
    call bin\%PLATFORM%\%CONFIG%\sandbox.exe
    goto done
popd

:error
echo use: run.bat config platform

:done

endlocal