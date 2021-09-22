@echo off

set MSVS_VERSION=

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019" (
    set MSVS_VERSION=vs2019
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017" (
    set MSVS_VERSION=vs2017
)

call "premake/premake5.exe" %MSVS_VERSION% --file="premake/premake5.lua"

set MSVS_VERSION=
