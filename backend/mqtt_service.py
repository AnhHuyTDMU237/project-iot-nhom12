import json
import os
from datetime import datetime

import paho.mqtt.client as mqtt
from dotenv import load_dotenv
from sqlalchemy.orm import Session

from database import SessionLocal
from models import Device, SensorData, Event


# ============================================================
# LOAD ENV
# ============================================================

load_dotenv()


# ============================================================
# CẤU HÌNH MQTT
# ============================================================

MQTT_HOST = os.getenv(
    "MQTT_HOST",
    "localhost"
)

MQTT_PORT = int(
    os.getenv(
        "MQTT_PORT",
        "1883"
    )
)


MQTT_SENSOR_TOPIC = os.getenv(
    "MQTT_SENSOR_TOPIC",
    "iot/firegas/sensor"
)

MQTT_EVENT_TOPIC = os.getenv(
    "MQTT_EVENT_TOPIC",
    "iot/firegas/event"
)

MQTT_DEVICE_TOPIC = os.getenv(
    "MQTT_DEVICE_TOPIC",
    "iot/firegas/device"
)


# ============================================================
# MQTT CLIENT
# ============================================================

mqtt_client = None


# ============================================================
# LOG THỜI GIAN
# ============================================================

def now():

    return datetime.utcnow()


# ============================================================
# UPDATE DEVICE
# ============================================================

def update_device(
    db: Session,
    device_id: str,
    status: str = "ONLINE"
):

    if not device_id:

        return None


    device = (
        db.query(Device)
        .filter(
            Device.device_id == device_id
        )
        .first()
    )


    if device is None:

        device = Device(

            device_id=device_id,

            name=device_id,

            status=status,

            last_seen=now()
        )

        db.add(device)

        print(
            f"[DB] Tạo device mới: {device_id}"
        )

    else:

        device.status = status

        device.last_seen = now()

        print(
            f"[DB] Update device: {device_id}"
        )


    return device


# ============================================================
# MQTT CONNECT
# ============================================================

def on_connect(
    client,
    userdata,
    flags,
    reason_code,
    properties
):

    print()
    print("=" * 70)

    print(
        "[MQTT] CONNECTED"
    )

    print(
        "[MQTT] HOST:",
        MQTT_HOST
    )

    print(
        "[MQTT] PORT:",
        MQTT_PORT
    )

    print(
        "[MQTT] REASON CODE:",
        reason_code
    )


    # ========================================================
    # SUBSCRIBE SENSOR
    # ========================================================

    result = client.subscribe(
        MQTT_SENSOR_TOPIC,
        qos=1
    )

    print(
        "[MQTT] SUBSCRIBE SENSOR:",
        MQTT_SENSOR_TOPIC,
        "=>",
        result
    )


    # ========================================================
    # SUBSCRIBE EVENT
    # ========================================================

    result = client.subscribe(
        MQTT_EVENT_TOPIC,
        qos=1
    )

    print(
        "[MQTT] SUBSCRIBE EVENT:",
        MQTT_EVENT_TOPIC,
        "=>",
        result
    )


    # ========================================================
    # SUBSCRIBE DEVICE
    # ========================================================

    result = client.subscribe(
        MQTT_DEVICE_TOPIC,
        qos=1
    )

    print(
        "[MQTT] SUBSCRIBE DEVICE:",
        MQTT_DEVICE_TOPIC,
        "=>",
        result
    )


    print("=" * 70)
    print()


# ============================================================
# MQTT DISCONNECT
# ============================================================

def on_disconnect(
    client,
    userdata,
    disconnect_flags,
    reason_code,
    properties
):

    print()
    print("=" * 70)

    print(
        "[MQTT] DISCONNECTED"
    )

    print(
        "[MQTT] REASON CODE:",
        reason_code
    )

    print("=" * 70)
    print()


# ============================================================
# MQTT MESSAGE
# ============================================================

def on_message(
    client,
    userdata,
    msg
):

    print()
    print("=" * 70)

    print(
        "[MQTT] MESSAGE RECEIVED"
    )

    print(
        "[MQTT] TOPIC:",
        msg.topic
    )


    # ========================================================
    # DECODE PAYLOAD
    # ========================================================

    try:

        payload = msg.payload.decode(
            "utf-8"
        )

    except Exception as e:

        print(
            "[MQTT] DECODE ERROR:",
            repr(e)
        )

        print("=" * 70)

        return


    print(
        "[MQTT] PAYLOAD:",
        payload
    )


    # ========================================================
    # PARSE JSON
    # ========================================================

    try:

        data = json.loads(
            payload
        )

    except Exception as e:

        print(
            "[MQTT] JSON ERROR:",
            repr(e)
        )

        print("=" * 70)

        return


    if not isinstance(
        data,
        dict
    ):

        print(
            "[MQTT] JSON KHONG PHAI OBJECT"
        )

        print("=" * 70)

        return


    # ========================================================
    # OPEN DATABASE
    # ========================================================

    db: Session = SessionLocal()


    try:

        # ====================================================
        # SENSOR
        # ====================================================

        if msg.topic == MQTT_SENSOR_TOPIC:

            save_sensor_data(
                db,
                data
            )


        # ====================================================
        # EVENT
        # ====================================================

        elif msg.topic == MQTT_EVENT_TOPIC:

            save_event(
                db,
                data
            )


        # ====================================================
        # DEVICE
        # ====================================================

        elif msg.topic == MQTT_DEVICE_TOPIC:

            save_device(
                db,
                data
            )


        # ====================================================
        # UNKNOWN TOPIC
        # ====================================================

        else:

            print(
                "[MQTT] UNKNOWN TOPIC:",
                msg.topic
            )


    except Exception as e:

        print()
        print(
            "[DATABASE] ERROR:",
            repr(e)
        )

        db.rollback()


    finally:

        db.close()


    print("=" * 70)
    print()


# ============================================================
# LƯU SENSOR DATA
# ============================================================

def save_sensor_data(
    db: Session,
    data: dict
):

    print()
    print(
        "---------------- SAVE SENSOR ----------------"
    )


    # ========================================================
    # DEVICE ID
    # ========================================================

    device_id = data.get(
        "device_id"
    )


    if not device_id:

        print(
            "[SENSOR] ERROR: missing device_id"
        )

        return


    # ========================================================
    # SENSOR VALUES
    # ========================================================

    temperature = data.get(
        "temperature"
    )

    humidity = data.get(
        "humidity"
    )

    mq2 = data.get(
        "mq2"
    )

    flame = data.get(
        "flame",
        False
    )

    pir = data.get(
        "pir",
        False
    )


    # ========================================================
    # LOG
    # ========================================================

    print(
        "[SENSOR] device_id  :",
        device_id
    )

    print(
        "[SENSOR] temperature:",
        temperature
    )

    print(
        "[SENSOR] humidity   :",
        humidity
    )

    print(
        "[SENSOR] mq2        :",
        mq2
    )

    print(
        "[SENSOR] flame      :",
        flame
    )

    print(
        "[SENSOR] pir        :",
        pir
    )


    # ========================================================
    # XỬ LÝ BOOLEAN AN TOÀN
    # ========================================================

    if isinstance(
        flame,
        str
    ):

        flame = flame.lower() in (
            "true",
            "1",
            "on",
            "yes"
        )

    else:

        flame = bool(
            flame
        )


    if isinstance(
        pir,
        str
    ):

        pir = pir.lower() in (
            "true",
            "1",
            "on",
            "yes"
        )

    else:

        pir = bool(
            pir
        )


    # ========================================================
    # TẠO SENSOR RECORD
    # ========================================================

    sensor = SensorData(

        device_id=device_id,

        temperature=temperature,

        humidity=humidity,

        mq2=mq2,

        flame=flame,

        pir=pir,

        created_at=now()
    )


    db.add(
        sensor
    )


    # ========================================================
    # UPDATE DEVICE
    # ========================================================

    update_device(
        db,
        device_id,
        "ONLINE"
    )


    # ========================================================
    # COMMIT
    # ========================================================

    db.commit()


    # ========================================================
    # REFRESH
    # ========================================================

    db.refresh(
        sensor
    )


    # ========================================================
    # SUCCESS
    # ========================================================

    print()
    print(
        "[DB] SENSOR SAVED SUCCESSFULLY"
    )

    print(
        "[DB] sensor_data.id =",
        sensor.id
    )

    print(
        "[DB] device_id      =",
        sensor.device_id
    )

    print(
        "[DB] temperature    =",
        sensor.temperature
    )

    print(
        "[DB] humidity       =",
        sensor.humidity
    )

    print(
        "[DB] mq2            =",
        sensor.mq2
    )

    print(
        "[DB] flame          =",
        sensor.flame
    )

    print(
        "[DB] pir            =",
        sensor.pir
    )


# ============================================================
# LƯU EVENT
# ============================================================

def save_event(
    db: Session,
    data: dict
):

    print()
    print(
        "---------------- SAVE EVENT ----------------"
    )


    device_id = data.get(
        "device_id"
    )

    event_name = data.get(
        "event"
    )

    severity = data.get(
        "severity"
    )

    message = data.get(
        "message"
    )


    # ========================================================
    # VALIDATE
    # ========================================================

    if not device_id:

        print(
            "[EVENT] ERROR: missing device_id"
        )

        return


    if not event_name:

        print(
            "[EVENT] ERROR: missing event"
        )

        return


    # ========================================================
    # LOG
    # ========================================================

    print(
        "[EVENT] device_id:",
        device_id
    )

    print(
        "[EVENT] event    :",
        event_name
    )

    print(
        "[EVENT] severity :",
        severity
    )

    print(
        "[EVENT] message  :",
        message
    )


    # ========================================================
    # CREATE EVENT
    # ========================================================

    event = Event(

        device_id=device_id,

        event=event_name,

        severity=severity,

        message=message,

        created_at=now()
    )


    db.add(
        event
    )


    # ========================================================
    # UPDATE DEVICE
    # ========================================================

    update_device(
        db,
        device_id,
        "ONLINE"
    )


    # ========================================================
    # COMMIT
    # ========================================================

    db.commit()


    db.refresh(
        event
    )


    # ========================================================
    # SUCCESS
    # ========================================================

    print()
    print(
        "[DB] EVENT SAVED SUCCESSFULLY"
    )

    print(
        "[DB] event.id =",
        event.id
    )


# ============================================================
# LƯU DEVICE
# ============================================================

def save_device(
    db: Session,
    data: dict
):

    print()
    print(
        "---------------- SAVE DEVICE ----------------"
    )


    device_id = data.get(
        "device_id"
    )

    status = data.get(
        "status",
        "UNKNOWN"
    )


    # ========================================================
    # VALIDATE
    # ========================================================

    if not device_id:

        print(
            "[DEVICE] ERROR: missing device_id"
        )

        return


    # ========================================================
    # LOG
    # ========================================================

    print(
        "[DEVICE] device_id:",
        device_id
    )

    print(
        "[DEVICE] status   :",
        status
    )


    # ========================================================
    # UPDATE / CREATE
    # ========================================================

    device = (
        db.query(Device)
        .filter(
            Device.device_id == device_id
        )
        .first()
    )


    if device is None:

        device = Device(

            device_id=device_id,

            name=device_id,

            status=status,

            last_seen=now()
        )

        db.add(
            device
        )

        print(
            "[DB] DEVICE CREATED"
        )

    else:

        device.status = status

        device.last_seen = now()

        print(
            "[DB] DEVICE UPDATED"
        )


    # ========================================================
    # COMMIT
    # ========================================================

    db.commit()


    print(
        "[DB] DEVICE SAVED SUCCESSFULLY"
    )


# ============================================================
# START MQTT
# ============================================================

def start_mqtt():

    global mqtt_client


    print()
    print("=" * 70)

    print(
        "STARTING MQTT SERVICE"
    )

    print(
        "MQTT HOST:",
        MQTT_HOST
    )

    print(
        "MQTT PORT:",
        MQTT_PORT
    )

    print("=" * 70)


    # ========================================================
    # CREATE CLIENT
    # ========================================================

    mqtt_client = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2
    )


    # ========================================================
    # CALLBACKS
    # ========================================================

    mqtt_client.on_connect = on_connect

    mqtt_client.on_message = on_message

    mqtt_client.on_disconnect = on_disconnect


    # ========================================================
    # CONNECT
    # ========================================================

    try:

        mqtt_client.connect(
            MQTT_HOST,
            MQTT_PORT,
            60
        )

    except Exception as e:

        print()
        print(
            "[MQTT] CONNECTION ERROR:",
            repr(e)
        )

        return None


    # ========================================================
    # START LOOP
    # ========================================================

    mqtt_client.loop_start()


    print()
    print(
        "[MQTT] SERVICE STARTED"
    )

    print(
        "[MQTT] Waiting for messages..."
    )

    print()


    return mqtt_client

# ============================================================
# MQTT COMMAND
# GỬI LỆNH TỪ FASTAPI -> MQTT -> ESP32
# ============================================================

MQTT_COMMAND_TOPIC = os.getenv(
    "MQTT_COMMAND_TOPIC",
    "iot/firegas/command"
)


def publish_command(
    device_id: str,
    command: str
):

    global mqtt_client

    # ========================================================
    # KIỂM TRA MQTT CLIENT
    # ========================================================

    if mqtt_client is None:

        print(
            "[MQTT COMMAND] MQTT client chưa khởi động"
        )

        return {
            "success": False,
            "message": "MQTT client chưa khởi động"
        }


    # ========================================================
    # KIỂM TRA KẾT NỐI
    # ========================================================

    if not mqtt_client.is_connected():

        print(
            "[MQTT COMMAND] MQTT chưa kết nối"
        )

        return {
            "success": False,
            "message": "MQTT chưa kết nối"
        }


    # ========================================================
    # KIỂM TRA INPUT
    # ========================================================

    if not device_id:

        return {
            "success": False,
            "message": "Thiếu device_id"
        }


    if not command:

        return {
            "success": False,
            "message": "Thiếu command"
        }


    # ========================================================
    # TẠO PAYLOAD
    # ========================================================

    payload = {
        "device_id": device_id,
        "command": command
    }


    payload_json = json.dumps(
        payload
    )


    # ========================================================
    # LOG
    # ========================================================

    print()
    print("=" * 60)

    print(
        "[MQTT COMMAND] GỬI LỆNH"
    )

    print(
        "[MQTT COMMAND] TOPIC:",
        MQTT_COMMAND_TOPIC
    )

    print(
        "[MQTT COMMAND] PAYLOAD:",
        payload_json
    )


    # ========================================================
    # PUBLISH
    # ========================================================

    try:

        result = mqtt_client.publish(

            MQTT_COMMAND_TOPIC,

            payload_json,

            qos=1
        )


        # ====================================================
        # KIỂM TRA RESULT
        # ====================================================

        if result.rc != mqtt.MQTT_ERR_SUCCESS:

            print(
                "[MQTT COMMAND] PUBLISH ERROR:",
                result.rc
            )

            return {
                "success": False,
                "message": "Không gửi được MQTT command",
                "mqtt_rc": result.rc
            }


        print(
            "[MQTT COMMAND] PUBLISH SUCCESS"
        )

        print("=" * 60)
        print()


        return {
            "success": True,
            "message": "Đã gửi command",
            "device_id": device_id,
            "command": command,
            "topic": MQTT_COMMAND_TOPIC
        }


    except Exception as e:

        print(
            "[MQTT COMMAND] ERROR:",
            repr(e)
        )

        return {
            "success": False,
            "message": str(e)
        }