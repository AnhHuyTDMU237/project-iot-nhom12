from fastapi import FastAPI, Depends
from fastapi.middleware.cors import CORSMiddleware
from sqlalchemy.orm import Session
from pydantic import BaseModel

from database import Base, engine, get_db
from models import Device, SensorData, Event, CameraImage
from mqtt_service import start_mqtt, publish_command


# =========================================================
# REQUEST MODELS
# =========================================================

class DeviceCommand(BaseModel):
    device_id: str = "esp32-001"
    command: str


class CommandRequest(BaseModel):
    command: str


# =========================================================
# FASTAPI APP
# =========================================================

app = FastAPI(
    title="IoT Fire Gas API",
    version="1.0.0"
)


# =========================================================
# CORS
# =========================================================
# Cho phép:
# - React chạy local
# - React chạy trên Vercel
# =========================================================

app.add_middleware(
    CORSMiddleware,

    allow_origins=[
        # React Vite local
        "http://localhost:5173",
        "http://127.0.0.1:5173",

        # React Vite port khác
        "http://localhost:5174",
        "http://127.0.0.1:5174",

        # Vercel project
        "https://project-iot-nhom12.vercel.app",
    ],

    # Cho phép các domain *.vercel.app
    allow_origin_regex=r"https://.*\.vercel\.app",

    allow_credentials=True,

    allow_methods=["*"],

    allow_headers=["*"],
)


# =========================================================
# STARTUP
# =========================================================

@app.on_event("startup")
def startup():

    print("=" * 60)
    print("STARTING IoT FIRE GAS API")
    print("=" * 60)

    # -----------------------------------------------------
    # CREATE DATABASE TABLES
    # -----------------------------------------------------

    print("Creating database tables...")

    try:

        Base.metadata.create_all(
            bind=engine
        )

        print("Database ready.")

    except Exception as e:

        print("DATABASE ERROR:")
        print(e)

    # -----------------------------------------------------
    # START MQTT
    # -----------------------------------------------------

    try:

        start_mqtt()

        print("MQTT service started.")

    except Exception as e:

        print("MQTT ERROR:")
        print(e)

    print("=" * 60)


# =========================================================
# ROOT
# =========================================================

@app.get("/")
def root():

    return {
        "message": "IoT Fire Gas API",
        "status": "running",
        "version": "1.0.0"
    }


# =========================================================
# HEALTH CHECK
# =========================================================

@app.get("/api/health")
def health():

    return {
        "status": "ok",
        "api": "running"
    }


# =========================================================
# GET DEVICES
# =========================================================

@app.get("/api/devices")
def get_devices(
    db: Session = Depends(get_db)
):

    devices = (
        db.query(Device)
        .order_by(Device.id.asc())
        .all()
    )

    return [
        {
            "id": device.id,
            "device_id": device.device_id,
            "name": device.name,
            "status": device.status,
            "last_seen": device.last_seen
        }

        for device in devices
    ]


# =========================================================
# GET LATEST SENSOR
# =========================================================

@app.get("/api/sensors/latest")
def get_latest_sensor(
    device_id: str = "esp32-001",
    db: Session = Depends(get_db)
):

    sensor = (
        db.query(SensorData)
        .filter(
            SensorData.device_id == device_id
        )
        .order_by(
            SensorData.id.desc()
        )
        .first()
    )

    if not sensor:

        return {
            "device_id": device_id,
            "temperature": None,
            "humidity": None,
            "mq2": None,
            "flame": False,
            "pir": False,
            "created_at": None,
            "message": "Chua co du lieu sensor"
        }

    return {

        "device_id": sensor.device_id,

        "temperature": sensor.temperature,

        "humidity": sensor.humidity,

        "mq2": sensor.mq2,

        "flame": sensor.flame,

        "pir": sensor.pir,

        "created_at": sensor.created_at
    }


# =========================================================
# GET SENSOR HISTORY
# =========================================================

@app.get("/api/sensors/history")
def get_sensor_history(
    device_id: str = "esp32-001",
    db: Session = Depends(get_db)
):

    sensors = (
        db.query(SensorData)
        .filter(
            SensorData.device_id == device_id
        )
        .order_by(
            SensorData.id.desc()
        )
        .limit(20)
        .all()
    )

    return [

        {
            "id": sensor.id,

            "device_id": sensor.device_id,

            "temperature": sensor.temperature,

            "humidity": sensor.humidity,

            "mq2": sensor.mq2,

            "flame": sensor.flame,

            "pir": sensor.pir,

            "created_at": sensor.created_at
        }

        for sensor in sensors
    ]


# =========================================================
# GET EVENTS
# =========================================================

@app.get("/api/events")
def get_events(
    device_id: str = "esp32-001",
    db: Session = Depends(get_db)
):

    events = (
        db.query(Event)
        .filter(
            Event.device_id == device_id
        )
        .order_by(
            Event.id.desc()
        )
        .limit(50)
        .all()
    )

    return [

        {
            "id": event.id,

            "device_id": event.device_id,

            "event": event.event,

            "severity": event.severity,

            "message": event.message,

            "created_at": event.created_at
        }

        for event in events
    ]


# =========================================================
# GET CAMERA IMAGES
# =========================================================

@app.get("/api/camera")
def get_camera_images(
    device_id: str = "esp32-001",
    db: Session = Depends(get_db)
):

    images = (
        db.query(CameraImage)
        .filter(
            CameraImage.device_id == device_id
        )
        .order_by(
            CameraImage.id.desc()
        )
        .limit(50)
        .all()
    )

    return [

        {
            "id": image.id,

            "device_id": image.device_id,

            "event": image.event,

            "image_path": image.image_path,

            "created_at": image.created_at
        }

        for image in images
    ]


# =========================================================
# SEND DEVICE COMMAND
# =========================================================

@app.post("/api/device/command")
def send_device_command_legacy(
    command_data: DeviceCommand
):

    allowed_commands = {

        "FAN_ON",
        "FAN_OFF",

        "PUMP_ON",
        "PUMP_OFF",

        "BUZZER_ON",
        "BUZZER_OFF",

        "DOOR_OPEN",
        "DOOR_CLOSE",

        "LIGHT_ON",
        "LIGHT_OFF"
    }

    command = command_data.command.upper()

    if command not in allowed_commands:

        return {

            "success": False,

            "message": "Command khong hop le",

            "command": command
        }

    try:

        payload = publish_command(
            command_data.device_id,
            command
        )

        return {

            "success": True,

            "message": "Command da gui",

            "device_id": command_data.device_id,

            "command": command,

            "data": payload
        }

    except Exception as e:

        return {

            "success": False,

            "message": str(e)
        }


# =========================================================
# SEND DEVICE COMMAND - NEW API
# =========================================================

@app.post("/api/devices/{device_id}/command")
def send_device_command(
    device_id: str,
    request: CommandRequest
):

    allowed_commands = {

        "FAN_ON",
        "FAN_OFF",

        "PUMP_ON",
        "PUMP_OFF",

        "BUZZER_ON",
        "BUZZER_OFF",

        "DOOR_OPEN",
        "DOOR_CLOSE",

        "LIGHT_ON",
        "LIGHT_OFF",
    }

    command = request.command.upper()

    if command not in allowed_commands:

        return {

            "success": False,

            "message": "Lenh khong hop le",

            "command": command
        }

    try:

        publish_command(
            device_id,
            command
        )

        return {

            "success": True,

            "device_id": device_id,

            "command": command
        }

    except Exception as e:

        return {

            "success": False,

            "message": str(e)
        }