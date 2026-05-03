@echo off
title ANSIITY RPG

:: Ensure working directory is the folder containing this bat file
cd /d "%~dp0"

echo ==============================
echo   ANSIITY RPG - Starting...
echo ==============================
echo.

:: Kill any stale server from a previous run
taskkill /f /im rpg_server.exe >nul 2>&1

:: Start the server in a separate hidden window (must not share console with game)
start "RPG Server" /MIN rpg_server.exe

:: Wait for server to be ready
echo Waiting for server...
set /a tries=0
:wait_loop
set /a tries+=1
if %tries% gtr 30 (
    echo ERROR: Server failed to start after 15 seconds.
    echo Make sure rpg_server.exe is not blocked by your firewall.
    pause
    exit /b 1
)
powershell -Command "(Invoke-WebRequest -Uri http://127.0.0.1:5000/run -UseBasicParsing -TimeoutSec 1).StatusCode" >nul 2>&1
if errorlevel 1 (
    ping -n 2 127.0.0.1 >nul
    goto wait_loop
)

echo Server ready!
echo.
echo Starting game...
echo (Close the game window to exit)
echo.

:: Launch game (blocks until game exits)
rpg.exe
if errorlevel 1 (
    echo.
    echo ERROR: rpg.exe crashed with exit code %errorlevel%
    pause
)

:: Kill server when game exits
echo.
echo Shutting down server...
taskkill /f /im rpg_server.exe >nul 2>&1

echo Done!
