import cv2
import os
import json
import time
from datetime import datetime

import paho.mqtt.client as mqtt


# ============================================================
# CONFIG
# ============================================================

MQTT_BROKER = "localhost"
MQTT_PORT = 1883

MQTT_TOPIC = "iot/firegas/camera"

CAMERA_INDEX = 0

# Thư mục lưu ảnh
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

# False = hệ thống đang bình thường
# True  = đã chụp ảnh FIRE rồi
#
# Mục đích:
# Nếu ESP32 gửi FIRE liên tục:
#
# FIRE → chụp 1 ảnh
# FIRE → không chụp
# FIRE → không chụp
#
# Khi NORMAL:
#
# NORMAL → reset
#
# FIRE lần tiếp theo → chụp ảnh mới
#
fire_already_captured = False


# ============================================================
# CAPTURE CAMERA
# ============================================================

def capture_fire_image():

    print()
    print("==========================================")
    print("📷 FIRE DETECTED - CAPTURE CAMERA")
    print("==========================================")

    print("📷 Đang mở webcam...")

    camera = cv2.VideoCapture(
        CAMERA_INDEX
    )

    if not camera.isOpened():

        print("❌ Không mở được webcam")

        return None

    # Cho webcam ổn định
    time.sleep(2)

    # Bỏ các frame đầu
    for _ in range(5):

        ret, frame = camera.read()

        if not ret:
            print("⚠ Không đọc được frame")

    # Đọc frame cuối
    ret, frame = camera.read()

    if not ret:

        print("❌ Không lấy được hình từ webcam")

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

    if success:

        print()
        print("==========================================")
        print("✅ CHỤP ẢNH FIRE THÀNH CÔNG")
        print(f"📁 {filepath}")
        print("==========================================")
        print()

        return filepath

    print(
        "❌ Không lưu được ảnh"
    )

    return None


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
        print(f"📡 Broker: {MQTT_BROKER}:{MQTT_PORT}")
        print("==========================================")

        client.subscribe(
            MQTT_TOPIC
        )

        print(
            f"📡 SUBSCRIBE: {MQTT_TOPIC}"
        )

        print()
        print("📷 Camera Gateway đang chờ FIRE...")
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
        f"⚠ MQTT DISCONNECTED: {reason_code}"
    )

    print(
        "🔄 Đang chờ MQTT kết nối lại..."
    )


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
    print("📩 MQTT MESSAGE")
    print(
        f"Topic: {msg.topic}"
    )
    print(
        f"Data : {msg.payload.decode('utf-8', errors='ignore')}"
    )

    try:

        payload = msg.payload.decode(
            "utf-8"
        )

        data = json.loads(
            payload
        )

        # ====================================================
        # ĐỌC EVENT
        # ====================================================

        event = str(
            data.get(
                "event",
                ""
            )
        ).upper()

        camera_command = str(
            data.get(
                "camera",
                ""
            )
        ).upper()

        # ====================================================
        # FIRE
        # ====================================================

        if (
            event == "FIRE"
            and camera_command == "CAPTURE"
        ):

            print(
                "🔥 Nhận lệnh FIRE + CAPTURE"
            )

            # ------------------------------------------------
            # Nếu chưa chụp trong lần FIRE hiện tại
            # ------------------------------------------------

            if not fire_already_captured:

                print(
                    "📸 Chưa chụp ảnh FIRE → bắt đầu chụp..."
                )

                filepath = capture_fire_image()

                if filepath:

                    fire_already_captured = True

                    print(
                        "✅ Đã đánh dấu FIRE = CAPTURED"
                    )

                else:

                    print(
                        "⚠ Chụp ảnh thất bại."
                    )

            else:

                print(
                    "⏭ FIRE đã được chụp trước đó."
                )

                print(
                    "⏭ Không chụp lại."
                )

        # ====================================================
        # NORMAL
        # ====================================================

        elif event == "NORMAL":

            if fire_already_captured:

                print(
                    "🟢 Hệ thống trở lại NORMAL."
                )

                print(
                    "🔄 Reset trạng thái Camera."
                )

            fire_already_captured = False

        # ====================================================
        # CÁC EVENT KHÁC
        # ====================================================

        else:

            print(
                f"ℹ Event: {event}"
            )

    except json.JSONDecodeError:

        print(
            "❌ MQTT DATA không phải JSON hợp lệ"
        )

    except Exception as e:

        print(
            f"❌ MQTT DATA ERROR: {e}"
        )


# ============================================================
# MAIN
# ============================================================

def main():

    print()
    print("============================================================")
    print("             IOT FIRE GAS CAMERA GATEWAY")
    print("============================================================")

    print(
        f"📡 MQTT Broker : {MQTT_BROKER}:{MQTT_PORT}"
    )

    print(
        f"📡 MQTT Topic  : {MQTT_TOPIC}"
    )

    print(
        f"📷 Camera      : Index {CAMERA_INDEX}"
    )

    print(
        f"📁 Save folder : {SAVE_FOLDER}"
    )

    print("============================================================")
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