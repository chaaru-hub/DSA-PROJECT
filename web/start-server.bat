@echo off
rem Double-click to start the BST Visualizer on localhost.
rem Needs Python (python.org - tick "Add Python to PATH" when installing).
title BST Visualizer - local server
cd /d "%~dp0"
set PY=
where py >nul 2>nul && set PY=py
if not defined PY where python >nul 2>nul && set PY=python
if not defined PY (
    echo.
    echo  Python was not found.
    echo  Install it from https://www.python.org/downloads/
    echo  and tick "Add Python to PATH", then run this file again.
    echo.
    pause
    exit /b 1
)
%PY% serve.py
pause
