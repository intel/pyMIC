@ECHO OFF
SETLOCAL

SET LIBXSTREAM_ROOT=%~d0%~p0\..

CALL %~d0"%~p0"_vs.bat vs2013
START %~d0"%~p0"_vs2013.sln

ENDLOCAL