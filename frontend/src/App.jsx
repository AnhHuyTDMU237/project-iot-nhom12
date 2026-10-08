import { useEffect, useState } from "react";
import api from "./api";
import "./App.css";

function App() {

    const [activeTab, setActiveTab] = useState("overview");

    const [devices, setDevices] = useState([]);
    const [sensor, setSensor] = useState(null);
    const [events, setEvents] = useState([]);
    const [sensorHistory, setSensorHistory] = useState([]);
    const [cameraImages, setCameraImages] = useState([]);

    // Ảnh camera đang được chọn để xem chi tiết
    const [selectedImage, setSelectedImage] = useState(null);

    const [apiStatus, setApiStatus] = useState("CONNECTING");

    // Trạng thái điều khiển local
    const [controls, setControls] = useState({
        fan: false,
        pump: false,
        buzzer: false,
        door: false,
        light: false
    });


    // =====================================================
    // API BASE URL
    // =====================================================

    const API_BASE_URL =
        api.defaults.baseURL ||
        "http://localhost:8000";


    // =====================================================
    // TẠO URL ẢNH
    // =====================================================

    const getImageUrl = (imagePath) => {

        if (!imagePath) {
            return "";
        }

        // Nếu Backend đã trả về URL đầy đủ
        if (
            imagePath.startsWith("http://") ||
            imagePath.startsWith("https://")
        ) {
            return imagePath;
        }

        // Nếu Backend trả:
        // /camera-images/fire_xxx.jpg
        return `${API_BASE_URL}${imagePath}`;
    };


    // =====================================================
    // DEVICES
    // =====================================================

    const loadDevices = async () => {

        try {

            const response = await api.get(
                "/api/devices"
            );

            setDevices(response.data);

        } catch (error) {

            console.error(
                "Lỗi devices:",
                error
            );

        }
    };


    // =====================================================
    // SENSOR
    // =====================================================

    const loadSensor = async () => {

        try {

            const response = await api.get(
                "/api/sensors/latest",
                {
                    params: {
                        device_id: "esp32-001"
                    }
                }
            );

            setSensor(response.data);

            setApiStatus("ONLINE");

        } catch (error) {

            console.error(
                "Lỗi sensor:",
                error
            );

            setApiStatus("OFFLINE");

        }
    };


    // =====================================================
    // SENSOR HISTORY
    // =====================================================

    const loadSensorHistory = async () => {

        try {

            const response = await api.get(
                "/api/sensors/history",
                {
                    params: {
                        device_id: "esp32-001"
                    }
                }
            );

            setSensorHistory(
                response.data.slice(0, 20)
            );

        } catch (error) {

            console.error(
                "Lỗi sensor history:",
                error
            );

        }
    };


    // =====================================================
    // EVENTS
    // =====================================================

    const loadEvents = async () => {

        try {

            const response = await api.get(
                "/api/events",
                {
                    params: {
                        device_id: "esp32-001"
                    }
                }
            );

            setEvents(
                response.data.slice(0, 20)
            );

        } catch (error) {

            console.error(
                "Lỗi events:",
                error
            );

        }
    };


    // =====================================================
    // CAMERA
    // =====================================================

    const loadCamera = async () => {

        try {

            const response = await api.get(
                "/api/camera",
                {
                    params: {
                        device_id: "esp32-001"
                    }
                }
            );

            setCameraImages(
                response.data.slice(0, 20)
            );

        } catch (error) {

            console.error(
                "Lỗi camera:",
                error
            );

        }
    };


    // =====================================================
    // GỬI COMMAND
    // =====================================================

    const sendCommand = async (
        command,
        controlName = null,
        value = null
    ) => {

        try {

            console.log(
                "Sending command:",
                command
            );

            await api.post(
                "/api/device/command",
                {
                    device_id: "esp32-001",
                    command: command
                }
            );

            // Cập nhật giao diện ngay
            if (controlName) {

                setControls(
                    prev => ({
                        ...prev,
                        [controlName]: value
                    })
                );

            }

        } catch (error) {

            console.error(
                "Lỗi gửi command:",
                error
            );

            alert(
                "Không thể gửi lệnh đến ESP32!"
            );

        }

    };


    // =====================================================
    // LOAD DATA
    // =====================================================

    const loadAllData = async () => {

        await Promise.all([
            loadDevices(),
            loadSensor(),
            loadSensorHistory(),
            loadEvents(),
            loadCamera()
        ]);

    };


    // =====================================================
    // AUTO REFRESH
    // =====================================================

    useEffect(() => {

        // Load lần đầu
        loadAllData();

        // Cập nhật mỗi 3 giây
        const interval = setInterval(() => {

            loadSensor();

            loadSensorHistory();

            loadEvents();

            loadDevices();

            // Quan trọng:
            // Camera cũng được cập nhật tự động
            loadCamera();

        }, 3000);

        return () => {

            clearInterval(interval);

        };

    }, []);


    // =====================================================
    // ĐÓNG MODAL KHI NHẤN ESC
    // =====================================================

    useEffect(() => {

        const handleKeyDown = (event) => {

            if (event.key === "Escape") {

                setSelectedImage(null);

            }

        };

        window.addEventListener(
            "keydown",
            handleKeyDown
        );

        return () => {

            window.removeEventListener(
                "keydown",
                handleKeyDown
            );

        };

    }, []);


    // =====================================================
    // DEVICE STATUS
    // =====================================================

    const device =
        devices.find(
            item =>
                item.device_id === "esp32-001"
        );


    // =====================================================
    // RENDER
    // =====================================================

    return (

        <div className="app">

            {/* ================================================= */}
            {/* HEADER */}
            {/* ================================================= */}

            <header className="topbar">

                <div className="brand">

                    <div className="brand-icon">
                        🔥
                    </div>

                    <div>

                        <h1>
                            FIREGUARD
                        </h1>

                        <span>
                            IoT FIRE & GAS MONITORING
                        </span>

                    </div>

                </div>


                <div className="connection">

                    <span
                        className={
                            apiStatus === "ONLINE"
                                ? "status-dot online"
                                : "status-dot"
                        }
                    />

                    <div>

                        <strong>
                            ESP32-001
                        </strong>

                        <small>
                            {device?.status || apiStatus}
                        </small>

                    </div>

                </div>

            </header>


            {/* ================================================= */}
            {/* NAVIGATION */}
            {/* ================================================= */}

            <nav className="tabs">

                <button
                    className={
                        activeTab === "overview"
                            ? "tab active"
                            : "tab"
                    }
                    onClick={() =>
                        setActiveTab("overview")
                    }
                >
                    ◉ Tổng quan
                </button>


                <button
                    className={
                        activeTab === "control"
                            ? "tab active"
                            : "tab"
                    }
                    onClick={() =>
                        setActiveTab("control")
                    }
                >
                    ⚡ Điều khiển
                </button>


                <button
                    className={
                        activeTab === "history"
                            ? "tab active"
                            : "tab"
                    }
                    onClick={() =>
                        setActiveTab("history")
                    }
                >
                    📊 Lịch sử
                </button>


                <button
                    className={
                        activeTab === "events"
                            ? "tab active"
                            : "tab"
                    }
                    onClick={() =>
                        setActiveTab("events")
                    }
                >
                    🚨 Sự kiện
                </button>


                <button
                    className={
                        activeTab === "camera"
                            ? "tab active"
                            : "tab"
                    }
                    onClick={() =>
                        setActiveTab("camera")
                    }
                >
                    📷 Camera
                </button>

            </nav>


            {/* ================================================= */}
            {/* OVERVIEW */}
            {/* ================================================= */}

            {activeTab === "overview" && (

                <>

                    <section className="section-title">

                        <div>

                            <span>
                                LIVE MONITORING
                            </span>

                            <h2>
                                Trạng thái hệ thống
                            </h2>

                        </div>

                        <div className="live-badge">
                            ● LIVE
                        </div>

                    </section>


                    <div className="sensor-grid">


                        <div className="sensor-card temperature">

                            <span className="sensor-icon">
                                🌡️
                            </span>

                            <div>

                                <small>
                                    TEMPERATURE
                                </small>

                                <strong>
                                    {sensor?.temperature ?? "--"}
                                    <em> °C</em>
                                </strong>

                            </div>

                        </div>


                        <div className="sensor-card humidity">

                            <span className="sensor-icon">
                                💧
                            </span>

                            <div>

                                <small>
                                    HUMIDITY
                                </small>

                                <strong>
                                    {sensor?.humidity ?? "--"}
                                    <em> %</em>
                                </strong>

                            </div>

                        </div>


                        <div
                            className={
                                sensor?.mq2 != null && sensor.mq2 >= 300
                                    ? "sensor-card gas-danger"
                                    : "sensor-card mq2"
                            }
                        >
                            <span className="sensor-icon">
                                🧪
                            </span>

                            <div>

                                <small>
                                    MQ-2 GAS / SMOKE
                                </small>

                                <strong>
                                    {sensor?.mq2 ?? "--"}
                                </strong>

                                <span
                                    className={
                                        sensor?.mq2 != null && sensor.mq2 >= 300
                                            ? "gas-status danger"
                                            : "gas-status normal"
                                    }
                                >
                                    {sensor?.mq2 == null
                                        ? "WAITING"
                                        : sensor.mq2 >= 300
                                            ? "⚠ GAS ALERT"
                                            : "✓ NORMAL"}
                                </span>

                            </div>
                        </div>


                        <div
                            className={
                                sensor?.flame
                                    ? "sensor-card danger"
                                    : "sensor-card"
                            }
                        >

                            <span className="sensor-icon">
                                🔥
                            </span>

                            <div>

                                <small>
                                    FLAME
                                </small>

                                <strong>
                                    {!sensor
                                        ? "--"
                                        : sensor.flame
                                            ? "DETECTED"
                                            : "NORMAL"}
                                </strong>

                            </div>

                        </div>


                        <div
                            className={
                                sensor?.pir
                                    ? "sensor-card warning"
                                    : "sensor-card"
                            }
                        >

                            <span className="sensor-icon">
                                👤
                            </span>

                            <div>

                                <small>
                                    PIR
                                </small>

                                <strong>
                                    {!sensor
                                        ? "--"
                                        : sensor.pir
                                            ? "PERSON"
                                            : "CLEAR"}
                                </strong>

                            </div>

                        </div>

                    </div>


                    {/* QUICK CONTROL */}

                    <section className="panel">

                        <div className="panel-header">

                            <div>

                                <span>
                                    QUICK CONTROL
                                </span>

                                <h2>
                                    Điều khiển nhanh
                                </h2>

                            </div>

                            <button
                                className="view-button"
                                onClick={() =>
                                    setActiveTab("control")
                                }
                            >
                                Mở điều khiển →
                            </button>

                        </div>


                        <div className="quick-grid">

                            <ControlButton
                                icon="🌀"
                                title="Quạt"
                                value={controls.fan}
                                onOn={() =>
                                    sendCommand(
                                        "FAN_ON",
                                        "fan",
                                        true
                                    )
                                }
                                onOff={() =>
                                    sendCommand(
                                        "FAN_OFF",
                                        "fan",
                                        false
                                    )
                                }
                            />


                            <ControlButton
                                icon="💦"
                                title="Bơm"
                                value={controls.pump}
                                onOn={() =>
                                    sendCommand(
                                        "PUMP_ON",
                                        "pump",
                                        true
                                    )
                                }
                                onOff={() =>
                                    sendCommand(
                                        "PUMP_OFF",
                                        "pump",
                                        false
                                    )
                                }
                            />


                            <ControlButton
                                icon="🔊"
                                title="Buzzer"
                                value={controls.buzzer}
                                onOn={() =>
                                    sendCommand(
                                        "BUZZER_ON",
                                        "buzzer",
                                        true
                                    )
                                }
                                onOff={() =>
                                    sendCommand(
                                        "BUZZER_OFF",
                                        "buzzer",
                                        false
                                    )
                                }
                            />


                            <ControlButton
                                icon="💡"
                                title="Đèn"
                                value={controls.light}
                                onOn={() =>
                                    sendCommand(
                                        "LIGHT_ON",
                                        "light",
                                        true
                                    )
                                }
                                onOff={() =>
                                    sendCommand(
                                        "LIGHT_OFF",
                                        "light",
                                        false
                                    )
                                }
                            />

                        </div>

                    </section>

                </>

            )}


            {/* ================================================= */}
            {/* CONTROL */}
            {/* ================================================= */}

            {activeTab === "control" && (

                <section className="panel">

                    <div className="section-title">

                        <div>

                            <span>
                                DEVICE CONTROL
                            </span>

                            <h2>
                                Điều khiển thiết bị
                            </h2>

                        </div>

                    </div>


                    <div className="control-grid">


                        <ControlPanel
                            icon="🌀"
                            title="Quạt thông gió"
                            status={controls.fan}
                            onOn={() =>
                                sendCommand(
                                    "FAN_ON",
                                    "fan",
                                    true
                                )
                            }
                            onOff={() =>
                                sendCommand(
                                    "FAN_OFF",
                                    "fan",
                                    false
                                )
                            }
                        />


                        <ControlPanel
                            icon="💦"
                            title="Máy bơm"
                            status={controls.pump}
                            onOn={() =>
                                sendCommand(
                                    "PUMP_ON",
                                    "pump",
                                    true
                                )
                            }
                            onOff={() =>
                                sendCommand(
                                    "PUMP_OFF",
                                    "pump",
                                    false
                                )
                            }
                        />


                        <ControlPanel
                            icon="🔊"
                            title="Còi cảnh báo"
                            status={controls.buzzer}
                            onOn={() =>
                                sendCommand(
                                    "BUZZER_ON",
                                    "buzzer",
                                    true
                                )
                            }
                            onOff={() =>
                                sendCommand(
                                    "BUZZER_OFF",
                                    "buzzer",
                                    false
                                )
                            }
                        />


                        <ControlPanel
                            icon="💡"
                            title="Đèn"
                            status={controls.light}
                            onOn={() =>
                                sendCommand(
                                    "LIGHT_ON",
                                    "light",
                                    true
                                )
                            }
                            onOff={() =>
                                sendCommand(
                                    "LIGHT_OFF",
                                    "light",
                                    false
                                )
                            }
                        />


                        <ControlPanel
                            icon="🚪"
                            title="Cửa thoát hiểm"
                            status={controls.door}
                            openLabel="OPEN"
                            closeLabel="CLOSE"
                            onOn={() =>
                                sendCommand(
                                    "DOOR_OPEN",
                                    "door",
                                    true
                                )
                            }
                            onOff={() =>
                                sendCommand(
                                    "DOOR_CLOSE",
                                    "door",
                                    false
                                )
                            }
                        />

                    </div>

                </section>

            )}


            {/* ================================================= */}
            {/* HISTORY */}
            {/* ================================================= */}

            {activeTab === "history" && (

                <section className="panel">

                    <div className="panel-header">

                        <div>

                            <span>
                                SENSOR HISTORY
                            </span>

                            <h2>
                                20 dữ liệu gần nhất
                            </h2>

                        </div>

                        <div className="refresh-info">
                            ↻ Tự động cập nhật 3s
                        </div>

                    </div>


                    <div className="table-container">

                        <table>

                            <thead>

                                <tr>

                                    <th>
                                        Thời gian
                                    </th>

                                    <th>
                                        Temp
                                    </th>

                                    <th>
                                        Humidity
                                    </th>

                                    <th>
                                        MQ2
                                    </th>

                                    <th>
                                        Flame
                                    </th>

                                    <th>
                                        PIR
                                    </th>

                                </tr>

                            </thead>


                            <tbody>

                                {sensorHistory.map(
                                    item => (

                                        <tr
                                            key={item.id}
                                        >

                                            <td>
                                                {new Date(
                                                    item.created_at
                                                ).toLocaleTimeString(
                                                    "vi-VN"
                                                )}
                                            </td>

                                            <td>
                                                {item.temperature}
                                                °C
                                            </td>

                                            <td>
                                                {item.humidity}
                                                %
                                            </td>

                                            <td>
                                                {item.mq2}
                                            </td>

                                            <td>

                                                <span
                                                    className={
                                                        item.flame
                                                            ? "badge danger"
                                                            : "badge normal"
                                                    }
                                                >
                                                    {item.flame
                                                        ? "FIRE"
                                                        : "NORMAL"}
                                                </span>

                                            </td>

                                            <td>

                                                <span
                                                    className={
                                                        item.pir
                                                            ? "badge warning"
                                                            : "badge normal"
                                                    }
                                                >
                                                    {item.pir
                                                        ? "PERSON"
                                                        : "CLEAR"}
                                                </span>

                                            </td>

                                        </tr>

                                    )
                                )}

                            </tbody>

                        </table>

                    </div>

                </section>

            )}


            {/* ================================================= */}
            {/* EVENTS */}
            {/* ================================================= */}

            {activeTab === "events" && (

                <section className="panel">

                    <div className="panel-header">

                        <div>

                            <span>
                                EVENT LOG
                            </span>

                            <h2>
                                Lịch sử cảnh báo
                            </h2>

                        </div>

                    </div>


                    <div className="event-list">

                        {events.length === 0 && (

                            <div className="empty">
                                Chưa có sự kiện
                            </div>

                        )}


                        {events.map(
                            event => (

                                <div
                                    className="event-row"
                                    key={event.id}
                                >

                                    <div className="event-icon">
                                        🚨
                                    </div>

                                    <div className="event-content">

                                        <strong>
                                            {event.event}
                                        </strong>

                                        <span>
                                            {event.message}
                                        </span>

                                    </div>

                                    <div className="event-time">

                                        {new Date(
                                            event.created_at
                                        ).toLocaleString(
                                            "vi-VN"
                                        )}

                                    </div>

                                </div>

                            )
                        )}

                    </div>

                </section>

            )}


            {/* ================================================= */}
            {/* CAMERA */}
            {/* ================================================= */}

            {activeTab === "camera" && (

                <section className="panel">

                    <div className="panel-header">

                        <div>

                            <span>
                                CAMERA MONITORING
                            </span>

                            <h2>
                                Hình ảnh sự kiện
                            </h2>

                        </div>

                        <div className="refresh-info">
                            ↻ Tự động cập nhật 3s
                        </div>

                    </div>


                    {cameraImages.length === 0 ? (

                        <div className="camera-empty">

                            <div>
                                📷
                            </div>

                            <h3>
                                Chưa có hình ảnh
                            </h3>

                            <p>
                                Camera sẽ ghi lại hình ảnh
                                khi phát hiện sự cố.
                            </p>

                        </div>

                    ) : (

                        <div className="camera-grid">

                            {cameraImages.map(
                                image => {

                                    const imageUrl =
                                        getImageUrl(
                                            image.image_url
                                        );

                                    return (

                                        <div
                                            className="camera-card"
                                            key={image.id}
                                        >

                                            {/* ================================= */}
                                            {/* ẢNH THẬT */}
                                            {/* ================================= */}

                                            <div
                                                className="camera-image-wrapper"
                                                onClick={() =>
                                                    setSelectedImage(
                                                        image
                                                    )
                                                }
                                                title="Bấm để xem ảnh lớn"
                                            >

                                                <img
                                                    src={imageUrl}
                                                    alt={
                                                        `Camera FIRE ${image.id}`
                                                    }
                                                    className="camera-image"
                                                    onError={(event) => {

                                                        event.currentTarget.style.display =
                                                            "none";

                                                        const errorBox =
                                                            event.currentTarget
                                                                .parentElement
                                                                .querySelector(
                                                                    ".camera-image-error"
                                                                );

                                                        if (errorBox) {

                                                            errorBox.style.display =
                                                                "flex";

                                                        }

                                                    }}
                                                />

                                                {/* Nếu ảnh lỗi */}

                                                <div
                                                    className="camera-image-error"
                                                    style={{
                                                        display: "none"
                                                    }}
                                                >
                                                    <span>
                                                        📷
                                                    </span>

                                                    <small>
                                                        Không tải được ảnh
                                                    </small>
                                                </div>

                                                {/* Overlay khi rê chuột */}

                                                <div className="camera-image-overlay">

                                                    <span>
                                                        🔍
                                                    </span>

                                                    <strong>
                                                        Xem ảnh
                                                    </strong>

                                                </div>

                                            </div>


                                            {/* ================================= */}
                                            {/* THÔNG TIN ẢNH */}
                                            {/* ================================= */}

                                            <div className="camera-info">

                                                <strong>
                                                    🔥 {image.event}
                                                </strong>

                                                <small>
                                                    {new Date(
                                                        image.created_at
                                                    ).toLocaleString(
                                                        "vi-VN"
                                                    )}
                                                </small>

                                                <button
                                                    className="camera-detail-button"
                                                    onClick={() =>
                                                        setSelectedImage(
                                                            image
                                                        )
                                                    }
                                                >
                                                    Xem chi tiết →
                                                </button>

                                            </div>

                                        </div>

                                    );
                                }
                            )}

                        </div>

                    )}

                </section>

            )}


            {/* ================================================= */}
            {/* CAMERA DETAIL MODAL */}
            {/* ================================================= */}

            {selectedImage && (

                <div
                    className="camera-modal"
                    onClick={() =>
                        setSelectedImage(null)
                    }
                >

                    <div
                        className="camera-modal-content"
                        onClick={(event) =>
                            event.stopPropagation()
                        }
                    >

                        {/* ================================= */}
                        {/* MODAL HEADER */}
                        {/* ================================= */}

                        <div className="camera-modal-header">

                            <div>

                                <span>
                                    CAMERA EVENT
                                </span>

                                <h2>
                                    🔥 {selectedImage.event}
                                </h2>

                            </div>

                            <button
                                className="camera-modal-close"
                                onClick={() =>
                                    setSelectedImage(null)
                                }
                            >
                                ✕
                            </button>

                        </div>


                        {/* ================================= */}
                        {/* ẢNH LỚN */}
                        {/* ================================= */}

                        <div className="camera-modal-image-container">

                            <img
                                src={
                                    getImageUrl(
                                        selectedImage.image_url
                                    )
                                }
                                alt="Fire event"
                                className="camera-modal-image"
                            />

                        </div>


                        {/* ================================= */}
                        {/* THÔNG TIN */}
                        {/* ================================= */}

                        <div className="camera-modal-info">

                            <div>

                                <span>
                                    Sự kiện
                                </span>

                                <strong>
                                    🔥 {selectedImage.event}
                                </strong>

                            </div>


                            <div>

                                <span>
                                    Thời gian
                                </span>

                                <strong>
                                    {new Date(
                                        selectedImage.created_at
                                    ).toLocaleString(
                                        "vi-VN"
                                    )}
                                </strong>

                            </div>


                            <div>

                                <span>
                                    Thiết bị
                                </span>

                                <strong>
                                    {selectedImage.device_id}
                                </strong>

                            </div>

                        </div>


                        {/* ================================= */}
                        {/* BUTTON */}
                        {/* ================================= */}

                        <button
                            className="camera-modal-button"
                            onClick={() =>
                                setSelectedImage(null)
                            }
                        >
                            Đóng
                        </button>

                    </div>

                </div>

            )}

        </div>
    );
}


// =========================================================
// CONTROL BUTTON
// =========================================================

function ControlButton({
    icon,
    title,
    value,
    onOn,
    onOff
}) {

    return (

        <div className="quick-control">

            <div className="quick-icon">
                {icon}
            </div>

            <div>

                <strong>
                    {title}
                </strong>

                <span>
                    {value ? "ĐANG BẬT" : "ĐANG TẮT"}
                </span>

            </div>

            <div className="mini-buttons">

                <button
                    className={
                        value
                            ? "on selected"
                            : "on"
                    }
                    onClick={onOn}
                >
                    ON
                </button>

                <button
                    className={
                        !value
                            ? "off selected"
                            : "off"
                    }
                    onClick={onOff}
                >
                    OFF
                </button>

            </div>

        </div>
    );
}


// =========================================================
// CONTROL PANEL
// =========================================================

function ControlPanel({
    icon,
    title,
    status,
    onOn,
    onOff,
    openLabel = "ON",
    closeLabel = "OFF"
}) {

    return (

        <div className="control-card">

            <div className="control-icon">
                {icon}
            </div>

            <div className="control-info">

                <h3>
                    {title}
                </h3>

                <span
                    className={
                        status
                            ? "device-state active"
                            : "device-state"
                    }
                >
                    ● {status ? "ON" : "OFF"}
                </span>

            </div>


            <div className="control-buttons">

                <button
                    className="btn-on"
                    onClick={onOn}
                >
                    {openLabel}
                </button>

                <button
                    className="btn-off"
                    onClick={onOff}
                >
                    {closeLabel}
                </button>

            </div>

        </div>
    );
}


export default App;