import cv2
import os
import json
import time
import requests
from datetime import datetime

import paho.mqtt.client as mqtt


# ============================================================
# CONFIG
# ============================================================

MQTT_BROKER = "localhost"
MQTT_PORT = 1883

MQTT_TOPIC = "iot/firegas/camera"

# FastAPI Backend
BACKEND_URL = "http://localhost:8000"

# Device ESP32
DEVICE_ID = "esp32-001"

# Webcam
CAMERA_INDEX = 0

# ============================================================
# FOLDER LƯU ẢNH
# ============================================================

SAVE_FOLDER = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "fire_images"
)

os.makedirs(
    SAVE_FOLDER,
    exist_ok=True
)


# ============================================================
# FIRE STATE
# ============================================================

# False:
#   Chưa chụp ảnh trong lần FIRE hiện tại
#
# True:
#   Đã chụp rồi
#
# Ví dụ:
#
# FIRE → chụp
# FIRE → không chụp
# FIRE → không chụp
#
# NORMAL → reset
#
# FIRE → chụp ảnh mới
# ============================================================

fire_already_captured = False


# ============================================================
# GỬI THÔNG TIN ẢNH CHO FASTAPI
# ============================================================

def send_image_to_backend(filename):
    """
    Gửi metadata của ảnh lên FastAPI.

    Không upload file ảnh qua HTTP.
    Ảnh vẫn nằm trong:
        camera_gateway/fire_images/

    FastAPI chỉ lưu tên file vào PostgreSQL.
    """

    url = f"{BACKEND_URL}/api/camera/upload"

    data = {
        "device_id": DEVICE_ID,
        "event": "FIRE",
        "image_path": filename
    }

    print()
    print("==========================================")
    print("📡 GỬI THÔNG TIN ẢNH CHO FASTAPI")
    print("==========================================")

    print(f"🌐 URL      : {url}")
    print(f"📱 Device   : {DEVICE_ID}")
    print(f"🔥 Event    : FIRE")
    print(f"📷 Filename : {filename}")

    try:

        response = requests.post(
            url,
            json=data,
            timeout=5
        )

        print(
            f"📥 HTTP Status: {response.status_code}"
        )

        if response.status_code == 200:

            try:

                result = response.json()

                print(
                    "✅ FastAPI đã nhận thông tin ảnh"
                )

                print(
                    f"📦 Response: {result}"
                )

                return True

            except Exception:

                print(
                    "⚠ FastAPI trả về dữ liệu "
                    "không phải JSON"
                )

                return True

        else:

            print(
                "❌ FastAPI không nhận được ảnh"
            )

            print(
                f"❌ Response: {response.text}"
            )

            return False

    except requests.exceptions.ConnectionError:

        print()
        print(
            "❌ KHÔNG KẾT NỐI ĐƯỢC FASTAPI"
        )

        print(
            f"❌ Kiểm tra Backend: {BACKEND_URL}"
        )

        print(
            "👉 Hãy chắc chắn FastAPI đang chạy."
        )

        return False

    except requests.exceptions.Timeout:

        print(
            "❌ FastAPI phản hồi quá lâu."
        )

        return False

    except Exception as e:

        print(
            f"❌ Lỗi gửi ảnh lên FastAPI: {e}"
        )

        return False


# ============================================================
# CAPTURE CAMERA
# ============================================================

def capture_fire_image():

    print()
    print("==========================================")
    print("📷 FIRE DETECTED - CAPTURE CAMERA")
    print("==========================================")

    print(
        "📷 Đang mở webcam..."
    )

    camera = cv2.VideoCapture(
        CAMERA_INDEX
    )

    # ========================================================
    # KIỂM TRA CAMERA
    # ========================================================

    if not camera.isOpened():

        print(
            "❌ Không mở được webcam"
        )

        return None

    # ========================================================
    # CHỜ WEBCAM ỔN ĐỊNH
    # ========================================================

    print(
        "⏳ Đang ổn định webcam..."
    )

    time.sleep(2)

    # ========================================================
    # BỎ CÁC FRAME ĐẦU
    # ========================================================

    for i in range(5):

        ret, frame = camera.read()

        if not ret:

            print(
                f"⚠ Không đọc được frame {i + 1}"
            )

    # ========================================================
    # ĐỌC FRAME CUỐI
    # ========================================================

    ret, frame = camera.read()

    if not ret:

        print(
            "❌ Không lấy được hình từ webcam"
        )

        camera.release()

        return None

    # ========================================================
    # TẠO TÊN FILE
    # ========================================================

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

    # ========================================================
    # LƯU ẢNH
    # ========================================================

    success = cv2.imwrite(
        filepath,
        frame
    )

    camera.release()

    # ========================================================
    # KIỂM TRA LƯU ẢNH
    # ========================================================

    if not success:

        print(
            "❌ Không lưu được ảnh"
        )

        return None

    # ========================================================
    # CHỤP THÀNH CÔNG
    # ========================================================

    print()
    print("==========================================")
    print("✅ CHỤP ẢNH FIRE THÀNH CÔNG")
    print("==========================================")

    print(
        f"📁 File: {filepath}"
    )

    print(
        f"📷 Filename: {filename}"
    )

    print("==========================================")
    print()

    # ========================================================
    # GỬI METADATA CHO FASTAPI
    # ========================================================

    backend_success = send_image_to_backend(
        filename
    )

    if backend_success:

        print(
            "✅ Ảnh đã được đăng ký vào Backend."
        )

    else:

        print(
            "⚠ Ảnh đã lưu local nhưng "
            "chưa đăng ký được với Backend."
        )

    return filepath


# ============================================================
# MQTT CONNECT
# ============================================================

def on_connect(
    client,
    userdata,
    flags,
    reason_code,
    properties=None
):

    if reason_code == 0:

        print()
        print("==========================================")
        print("✅ MQTT CONNECTED")
        print("==========================================")

        print(
            f"📡 Broker: "
            f"{MQTT_BROKER}:{MQTT_PORT}"
        )

        print(
            f"📡 Topic : {MQTT_TOPIC}"
        )

        print("==========================================")

        # Subscribe
        client.subscribe(
            MQTT_TOPIC
        )

        print(
            f"📡 SUBSCRIBE: {MQTT_TOPIC}"
        )

        print()
        print(
            "📷 Camera Gateway đang "
            "chờ FIRE..."
        )
        print()

    else:

        print(
            f"❌ MQTT ERROR: {reason_code}"
        )


# ============================================================
# MQTT DISCONNECT
# ============================================================

def on_disconnect(
    client,
    userdata,
    disconnect_flags,
    reason_code,
    properties=None
):

    print()

    print(
        f"⚠ MQTT DISCONNECTED: "
        f"{reason_code}"
    )

    print(
        "🔄 MQTT sẽ tự kết nối lại..."
    )

    print()


# ============================================================
# MQTT MESSAGE
# ============================================================

def on_message(
    client,
    userdata,
    msg
):

    global fire_already_captured

    print()
    print("==========================================")
    print("📩 MQTT MESSAGE")
    print("==========================================")

    print(
        f"Topic: {msg.topic}"
    )

    raw_payload = msg.payload.decode(
        "utf-8",
        errors="ignore"
    )

    print(
        f"Data : {raw_payload}"
    )

    print("==========================================")

    try:

        # ====================================================
        # PARSE JSON
        # ====================================================

        data = json.loads(
            raw_payload
        )

        # ====================================================
        # EVENT
        # ====================================================

        event = str(
            data.get(
                "event",
                ""
            )
        ).upper()

        # ====================================================
        # CAMERA COMMAND
        # ====================================================

        camera_command = str(
            data.get(
                "camera",
                ""
            )
        ).upper()

        # ====================================================
        # FIRE + CAPTURE
        # ====================================================

        if (
            event == "FIRE"
            and camera_command == "CAPTURE"
        ):

            print()
            print(
                "🔥 NHẬN FIRE + CAPTURE"
            )

            # ------------------------------------------------
            # CHƯA CHỤP
            # ------------------------------------------------

            if not fire_already_captured:

                print(
                    "📸 Chưa chụp ảnh "
                    "→ bắt đầu chụp..."
                )

                filepath = (
                    capture_fire_image()
                )

                # ------------------------------------------------
                # CHỤP THÀNH CÔNG
                # ------------------------------------------------

                if filepath:

                    fire_already_captured = True

                    print()
                    print(
                        "✅ FIRE = CAPTURED"
                    )

                    print(
                        "⏭ Những FIRE tiếp theo "
                        "sẽ không chụp lại."
                    )

                else:

                    print(
                        "⚠ Chụp ảnh thất bại."
                    )

            # ------------------------------------------------
            # ĐÃ CHỤP
            # ------------------------------------------------

            else:

                print(
                    "⏭ FIRE đã được chụp "
                    "trước đó."
                )

                print(
                    "⏭ Không chụp lại."
                )

        # ====================================================
        # NORMAL
        # ====================================================

        elif event == "NORMAL":

            if fire_already_captured:

                print()
                print(
                    "🟢 HỆ THỐNG TRỞ LẠI NORMAL"
                )

                print(
                    "🔄 Reset trạng thái Camera"
                )

            fire_already_captured = False

        # ====================================================
        # EVENT KHÁC
        # ====================================================

        else:

            print(
                f"ℹ Event: {event}"
            )

    # ========================================================
    # JSON ERROR
    # ========================================================

    except json.JSONDecodeError:

        print(
            "❌ MQTT DATA không phải "
            "JSON hợp lệ"
        )

    # ========================================================
    # OTHER ERROR
    # ========================================================

    except Exception as e:

        print(
            f"❌ MQTT DATA ERROR: {e}"
        )


# ============================================================
# MAIN
# ============================================================

def main():

    print()
    print(
        "============================================================"
    )

    print(
        "             IOT FIRE GAS CAMERA GATEWAY"
    )

    print(
        "============================================================"
    )

    print(
        f"📡 MQTT Broker : "
        f"{MQTT_BROKER}:{MQTT_PORT}"
    )

    print(
        f"📡 MQTT Topic  : "
        f"{MQTT_TOPIC}"
    )

    print(
        f"🌐 Backend     : "
        f"{BACKEND_URL}"
    )

    print(
        f"📱 Device ID   : "
        f"{DEVICE_ID}"
    )

    print(
        f"📷 Camera      : "
        f"Index {CAMERA_INDEX}"
    )

    print(
        f"📁 Save folder : "
        f"{SAVE_FOLDER}"
    )

    print(
        "============================================================"
    )

    print()

    # ========================================================
    # MQTT CLIENT
    # ========================================================

    client = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2
    )

    client.on_connect = on_connect

    client.on_disconnect = on_disconnect

    client.on_message = on_message

    # ========================================================
    # CONNECT MQTT
    # ========================================================

    try:

        print(
            "🔌 Đang kết nối MQTT..."
        )

        client.connect(
            MQTT_BROKER,
            MQTT_PORT,
            60
        )

    except Exception as e:

        print()
        print(
            "❌ KHÔNG KẾT NỐI ĐƯỢC MQTT"
        )

        print(
            f"❌ Lỗi: {e}"
        )

        print()

        print(
            "Kiểm tra Mosquitto Docker:"
        )

        print(
            "docker ps"
        )

        print()

        input(
            "Nhấn Enter để thoát..."
        )

        return

    # ========================================================
    # LOOP
    # ========================================================

    print(
        "🔄 Camera Gateway đang chạy..."
    )

    print(
        "⏳ Đang chờ MQTT FIRE..."
    )

    print()

    client.loop_forever()


# ============================================================
# RUN
# ============================================================

if __name__ == "__main__":

    main()