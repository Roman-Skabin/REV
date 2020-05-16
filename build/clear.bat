@echo off

rem use: clear

cls

pushd ..
    del /S /Q bin\*.*
    del /Q log\*.log
    del /Q *.exe, *.dll, *.lib, *.exp, 
popd
