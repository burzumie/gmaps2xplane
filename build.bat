call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"

qmake -makefile -win32 -spec win32-msvc2013

rem nmake clean
nmake debug
rem nmake release
