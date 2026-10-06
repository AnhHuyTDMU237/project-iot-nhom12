@echo off
title IOT FIRE GAS - STOP PROJECT

cd /d "%~dp0"

echo.
echo ============================================================
echo              IOT FIRE GAS SYSTEM
echo              STOPPING PROJECT
echo ============================================================
echo.

echo [1/3] Stopping Camera Gateway...

taskkill /FI "WINDOWTITLE eq IoT Camera Gateway*" /T /F >nul 2>&1

echo [OK] Camera Gateway stopped.
echo.


echo [2/3] Stopping FastAPI...

taskkill /FI "WINDOWTITLE eq IoT FastAPI Backend*" /T /F >nul 2>&1

echo [OK] FastAPI stopped.
echo.


echo [3/3] Stopping Docker...

docker compose down

echo.
echo [OK] Docker containers stopped.
echo.

echo ============================================================
echo                 PROJECT STOPPED
echo ============================================================
echo.

pause