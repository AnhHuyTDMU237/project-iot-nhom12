@echo off
title IOT FIRE GAS - START PROJECT

cd /d "%~dp0"

echo.
echo ============================================================
echo              IOT FIRE GAS SYSTEM
echo              STARTING PROJECT
echo ============================================================
echo.

REM ============================================================
REM 1. START DOCKER
REM ============================================================

echo [1/3] Starting Docker containers...
echo.

docker compose up -d

if errorlevel 1 (
    echo.
    echo [ERROR] Docker Compose failed!
    echo.
    pause
    exit /b 1
)

echo.
echo [OK] Docker containers started.
echo.

timeout /t 3 /nobreak >nul


REM ============================================================
REM 2. START FASTAPI
REM ============================================================

echo ============================================================
echo [2/3] Starting FastAPI Backend...
echo ============================================================
echo.

if not exist "backend\venv\Scripts\activate.bat" (

    echo [ERROR] Khong tim thay:
    echo backend\venv\Scripts\activate.bat
    echo.
    echo Hay tao Python virtual environment trong backend.
    echo.

    pause
    exit /b 1
)

start "IoT FastAPI Backend" cmd /k ^
"cd /d "%~dp0backend" && call venv\Scripts\activate.bat && python -m uvicorn main:app --host 0.0.0.0 --port 8000"

echo [OK] FastAPI dang khoi dong.
echo.

timeout /t 3 /nobreak >nul


REM ============================================================
REM 3. START CAMERA GATEWAY
REM ============================================================

echo ============================================================
echo [3/3] Starting Camera Gateway...
echo ============================================================
echo.

if not exist "camera_gateway\camera_gateway.py" (

    echo [ERROR] Khong tim thay:
    echo camera_gateway\camera_gateway.py
    echo.

    pause
    exit /b 1
)

start "IoT Camera Gateway" cmd /k ^
"cd /d "%~dp0camera_gateway" && python camera_gateway.py"

echo [OK] Camera Gateway dang khoi dong.
echo.


REM ============================================================
REM DONE
REM ============================================================

echo ============================================================
echo                 PROJECT STARTED
echo ============================================================
echo.

echo PostgreSQL:
echo     localhost:5432

echo.
echo Mosquitto:
echo     localhost:1883

echo.
echo FastAPI:
echo     http://localhost:8000

echo.
echo Health:
echo     http://localhost:8000/api/health

echo.
echo Vercel:
echo     https://project-iot-nhom12.vercel.app

echo.
echo Camera:
echo     camera_gateway\camera_gateway.py

echo.
echo ============================================================
echo.
echo He thong da khoi dong.
echo Khong dong cac cua so FastAPI va Camera Gateway.
echo.

pause