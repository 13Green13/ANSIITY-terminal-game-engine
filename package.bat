@echo off
:: Packages ANSIITY RPG into a distributable folder
:: Run from the project root (ANSIITY-terminal-game-engine/)

set DIST=dist\ANSIITY-RPG

echo Packaging ANSIITY RPG...

:: Build with static GCC runtime (no libstdc++ dependency)
echo Compiling rpg.exe...
g++ -O2 -static-libgcc -static-libstdc++ -o build\rpg.exe main_rpg.cpp -std=c++20 -I. -lwinhttp

:: Clean previous package
if exist %DIST% rmdir /s /q %DIST%
mkdir %DIST%
mkdir %DIST%\sprites

:: Copy executables
copy build\rpg.exe %DIST%\ >nul
copy dist\rpg_server.exe %DIST%\ >nul

:: Copy MinGW runtime DLL (libwinpthread)
for /f "tokens=*" %%i in ('where g++') do set MINGW_BIN=%%~dpi
copy "%MINGW_BIN%libwinpthread-1.dll" %DIST%\ >nul

:: Copy launcher
copy dist\play.bat %DIST%\ >nul

:: Copy server config files (editable, next to rpg_server.exe)
copy server\config.json %DIST%\ >nul
copy server\editor.html %DIST%\ >nul

:: Copy sprites
copy sprites\*.ansii %DIST%\sprites\ >nul

echo.
echo Package ready at: %DIST%\
echo Contents:
dir /b %DIST%
echo.
echo sprites/:
dir /b %DIST%\sprites | find /c ".ansii"
echo .ansii files copied
echo.
echo To play: run %DIST%\play.bat
echo To edit config: start rpg_server.exe then open http://127.0.0.1:5000/
