@echo off

rem use: rebuild [release|nsight]

call clean
call build %1
