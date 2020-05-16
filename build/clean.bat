@echo off

rem use: clean

cls

pushd ..
    del /S /Q bin\*.*
    del /Q log\*.log
popd
