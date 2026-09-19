@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_windows_portable.ps1" %*
set "AUDMES_EXIT_CODE=%ERRORLEVEL%"
if not "%AUDMES_EXIT_CODE%"=="0" (
    echo.
    echo AudMeS build failed. See the Chinese error message above.
    pause
)
exit /b %AUDMES_EXIT_CODE%
