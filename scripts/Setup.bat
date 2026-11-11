@echo off
rem Windows: runs Setup.py with Python 3. Arguments are passed on (see --help).
where py >nul 2>nul
if %ERRORLEVEL%==0 (py -3 "%~dp0Setup.py" %*) else (python "%~dp0Setup.py" %*)
set RESULT=%ERRORLEVEL%
rem Keep the window open when started by double-clicking
echo %cmdcmdline% | find /i "%~0" >nul && pause
exit /b %RESULT%
