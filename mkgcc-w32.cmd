@echo off
set MINGW_ROOT=..\mingw64-w32
set PATH=%MINGW_ROOT%\bin;extra\bin;%PATH%

set C_INCLUDE_PATH=extra\include
set LIBRARY_PATH=extra\lib

gcc -v

make ARCHOPTS=-march=pentiumpro
pause