@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_windows_portable.ps1" -CleanOnly
set "AUDMES_EXIT_CODE=%ERRORLEVEL%"
if not "%AUDMES_EXIT_CODE%"=="0" pause
exit /b %AUDMES_EXIT_CODE%
