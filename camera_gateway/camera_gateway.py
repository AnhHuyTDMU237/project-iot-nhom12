import cv2
import os
import json
import time
from datetime import datetime

import paho.mqtt.client as mqtt


# ==========================================
# CONFIG
# ==========================================

MQTT_BROKER = "localhost"
MQTT_PORT = 1883

MQTT_TOPIC = "iot/firegas/camera"

CAMERA_INDEX = 0

SAVE_FOLDER = "fire_images"

os.makedirs(
    SAVE_FOLDER,
    exist_ok=True
)


# ==========================================
# CAPTURE CAMERA
# ==========================================

def capture_fire_image():

    print("📷 Đang mở webcam...")

    camera = cv2.VideoCapture(
        CAMERA_INDEX
    )

    if not camera.isOpened():

        print(
            "❌ Không mở được webcam"
        )

        return None

    time.sleep(2)

    for _ in range(5):
        camera.read()

    ret, frame = camera.read()

    if not ret:

        print(
            "❌ Không lấy được hình"
        )

        camera.release()

        return None

    timestamp = datetime.now().strftime(
        "%Y%m%d_%H%M%S"
    )

    filename = (
        f"fire_{timestamp}.jpg"
    )

    filepath = os.path.join(
        SAVE_FOLDER,
        filename
    )

    success = cv2.imwrite(
        filepath,
        frame
    )

    camera.release()

    if success:

        print(
            f"✅ Đã lưu ảnh: {filepath}"
        )

        return filepath

    print(
        "❌ Không lưu được ảnh"
    )

    return None


# ==========================================
# MQTT CONNECT
# ==========================================

def on_connect(
    client,
    userdata,
    flags,
    rc,
    properties=None
):

    if rc == 0:

        print(
            "✅ MQTT CONNECTED"
        )

        client.subscribe(
            MQTT_TOPIC
        )

        print(
            f"📡 SUBSCRIBE: {MQTT_TOPIC}"
        )

    else:

        print(
            f"❌ MQTT ERROR: {rc}"
        )


# ==========================================
# MQTT MESSAGE
# ==========================================

def on_message(
    client,
    userdata,
    msg
):

    print(
        "📩 MQTT:",
        msg.topic,
        msg.payload
    )

    try:

        payload = msg.payload.decode(
            "utf-8"
        )

        data = json.loads(
            payload
        )

        event = data.get(
            "event"
        )

        camera_command = data.get(
            "camera"
        )

        if (
            event == "FIRE"
            and camera_command == "CAPTURE"
        ):

            capture_fire_image()

    except Exception as e:

        print(
            "❌ MQTT DATA ERROR:",
            e
        )


# ==========================================
# MAIN
# ==========================================

def main():

    client = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2
    )

    client.on_connect = on_connect

    client.on_message = on_message

    client.connect(
        MQTT_BROKER,
        MQTT_PORT,
        60
    )

    client.loop_forever()


if __name__ == "__main__":

    main()