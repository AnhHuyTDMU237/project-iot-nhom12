#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2s_std.h"

#include "ssd1306.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "mqtt_client.h"

#include "dht22.h"


/* =========================================================
 * 1. KHAI BÁO GPIO
 * ========================================================= */

// -------- CẢM BIẾN --------
#define DHT22_GPIO              GPIO_NUM_15

#define MQ2_ADC_CHANNEL         ADC_CHANNEL_0     // GPIO1 - MQ-2 AO

#define FLAME_GPIO              GPIO_NUM_3
#define PIR_GPIO                GPIO_NUM_16


// -------- THIẾT BỊ CHẤP HÀNH --------
#define RELAY_FAN_GPIO          GPIO_NUM_5
#define RELAY_PUMP_GPIO         GPIO_NUM_6

#define BUZZER_GPIO             GPIO_NUM_7

#define SERVO_GPIO              GPIO_NUM_8

// -------- OLED SSD1306 I2C --------
#define OLED_SDA_GPIO            GPIO_NUM_13
#define OLED_SCL_GPIO            GPIO_NUM_12
#define OLED_I2C_PORT            I2C_NUM_0
#define OLED_I2C_ADDR            0x3C
#define OLED_WIDTH               128
#define OLED_HEIGHT              64

// -------- MAX98357A I2S --------
#define I2S_BCLK_GPIO           GPIO_NUM_35
#define I2S_LRC_GPIO            GPIO_NUM_36
#define I2S_DOUT_GPIO           GPIO_NUM_37

// -------- LIGHT --------
#define LIGHT_GPIO        GPIO_NUM_9

/* =========================================================
 * 2. CẤU HÌNH LOGIC THIẾT BỊ
 * ========================================================= */

/*
 * Relay module của bạn thường là ACTIVE LOW:
 *
 * GPIO LOW  -> Relay ON
 * GPIO HIGH -> Relay OFF
 *
 * Nếu relay của bạn ngược lại thì đổi thành 0.
 */
#define RELAY_ACTIVE_LOW        1


/*
 * Flame sensor thường:
 *
 * LOW  = phát hiện lửa
 * HIGH = không có lửa
 *
 * Nếu module của bạn ngược lại thì đổi thành 0.
 */
#define FLAME_ACTIVE_LOW        1


/*
 * PIR:
 *
 * HIGH = có người
 * LOW  = không có người
 */
#define PIR_ACTIVE_HIGH         1


/*
 * Buzzer:
 *
 * HIGH = kêu
 * LOW  = tắt
 */
#define BUZZER_ACTIVE_HIGH      1


/* =========================================================
 * 3. NGƯỠNG CẢNH BÁO
 * ========================================================= */

/*
 * Đây chỉ là giá trị ban đầu.
 *
 * Sau khi chạy thực tế:
 * - xem GAS
 * - xem SMOKE
 *
 * rồi điều chỉnh lại cho phù hợp.
 */

#define MQ2_THRESHOLD            300


/* =========================================================
 * 4. SERVO
 * ========================================================= */

#define SERVO_FREQ               50

#define SERVO_MIN_US             500
#define SERVO_MAX_US             2500

#define SERVO_ANGLE_CLOSED       0
#define SERVO_ANGLE_OPEN         90


/* =========================================================
 * 5. BIẾN TOÀN CỤC
 * ========================================================= */

static const char *TAG = "FIRE_GAS_SYSTEM";

/* =========================================================
 * 5A. WIFI + MQTT
 * =========================================================
 *
 * QUAN TRỌNG:
 * - ESP32 không được dùng localhost để kết nối MQTT.
 * - MQTT_BROKER_URI phải là IP LAN của máy tính đang chạy
 *   Mosquitto Docker, ví dụ: mqtt://192.168.1.100:1883
 * - Đổi WIFI_SSID / WIFI_PASSWORD theo Wi-Fi của bạn.
 */

#define WIFI_SSID               "ESP32TEST"
#define WIFI_PASSWORD           "12345678"

#define MQTT_BROKER_URI         "mqtt://192.168.137.1:1883"

#define DEVICE_ID               "esp32-001"

#define MQTT_SENSOR_TOPIC       "iot/firegas/sensor"
#define MQTT_EVENT_TOPIC        "iot/firegas/event"
#define MQTT_DEVICE_TOPIC       "iot/firegas/device"
#define MQTT_COMMAND_TOPIC      "iot/firegas/command"
#define MQTT_CAMERA_TOPIC      "iot/firegas/camera"


/* =========================================================
 * TRẠNG THÁI HỆ THỐNG
 * ========================================================= */

typedef enum
{
    SYSTEM_NORMAL = 0,

    SYSTEM_MQ2_ALERT,

    SYSTEM_FIRE_ALERT

} system_state_t;


static system_state_t current_state = SYSTEM_NORMAL;

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool wifi_connected = false;
static bool mqtt_connected = false;

static void mqtt_publish_device(const char *status);

static void mqtt_publish_sensor(
    float temperature,
    float humidity,
    int mq2,
    bool flame,
    bool pir
);

static void mqtt_publish_event(
    const char *event,
    const char *severity,
    const char *message
);

static void mqtt_publish_camera_capture(void);

static void mqtt_handle_command(const char *payload);
static bool safety_command_allowed(const char *command);


/* =========================================================
 * PROTOTYPE CÁC HÀM ĐIỀU KHIỂN THIẾT BỊ
 * ========================================================= */

static void relay_fan(bool on);
static void relay_pump(bool on);
static void relay_light(bool on);
static void buzzer(bool on);
static void servo_set_angle(int angle);

// ============================================================
// MANUAL CONTROL - Dieu khien thiet bi tu MQTT/App
// ============================================================

static bool manual_fan = false;
static bool manual_pump = false;
static bool manual_buzzer = false;
static bool manual_door_open = false;
static bool manual_light = false;
                               

/* =========================================================
 * WIFI EVENT
 * ========================================================= */

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "WiFi: bat dau ket noi...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_connected = false;
        mqtt_connected = false;

        ESP_LOGW(TAG, "WiFi mat ket noi -> dang ket noi lai...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        wifi_connected = true;

        ESP_LOGI(
            TAG,
            "WiFi OK - IP: " IPSTR,
            IP2STR(&event->ip_info.ip)
        );
    }
}

/* =========================================================
 * MQTT EVENT
 * ========================================================= */

static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data
)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
        /* =================================================
         * MQTT CONNECTED
         * ================================================= */

        case MQTT_EVENT_CONNECTED:
        {
            mqtt_connected = true;

            ESP_LOGI(
                TAG,
                "MQTT CONNECTED -> %s",
                MQTT_BROKER_URI
            );

            /*
             * Báo ESP32 ONLINE
             */
            mqtt_publish_device("ONLINE");

            /*
             * Subscribe command từ Web
             */
            int msg_id =
                esp_mqtt_client_subscribe(
                    mqtt_client,
                    MQTT_COMMAND_TOPIC,
                    1
                );

            ESP_LOGI(
                TAG,
                "SUBSCRIBE COMMAND: %s, msg_id=%d",
                MQTT_COMMAND_TOPIC,
                msg_id
            );

            break;
        }


        /* =================================================
         * MQTT DISCONNECTED
         * ================================================= */

        case MQTT_EVENT_DISCONNECTED:
        {
            mqtt_connected = false;

            ESP_LOGW(
                TAG,
                "MQTT DISCONNECTED"
            );

            break;
        }


        /* =================================================
         * MQTT ERROR
         * ================================================= */

        case MQTT_EVENT_ERROR:
        {
            ESP_LOGE(
                TAG,
                "MQTT ERROR"
            );

            break;
        }


        /* =================================================
         * MQTT DATA
         * Nhận command từ Web
         * ================================================= */

        case MQTT_EVENT_DATA:
        {
            ESP_LOGI(
                TAG,
                "===== MQTT COMMAND RECEIVED ====="
            );

            ESP_LOGI(
                TAG,
                "TOPIC: %.*s",
                event->topic_len,
                event->topic
            );

            ESP_LOGI(
                TAG,
                "DATA : %.*s",
                event->data_len,
                event->data
            );


            /*
             * Kiểm tra topic
             */

            if (
                event->topic_len ==
                    strlen(MQTT_COMMAND_TOPIC)
                &&
                strncmp(
                    event->topic,
                    MQTT_COMMAND_TOPIC,
                    event->topic_len
                ) == 0
            )
            {
                /*
                 * MQTT payload không đảm bảo
                 * có ký tự '\0'
                 *
                 * Vì vậy phải copy sang buffer.
                 */

                char payload[256];

                int len = event->data_len;

                if (len >= sizeof(payload))
                {
                    len = sizeof(payload) - 1;
                }

                memcpy(
                    payload,
                    event->data,
                    len
                );

                payload[len] = '\0';


                ESP_LOGI(
                    TAG,
                    "COMMAND PAYLOAD: %s",
                    payload
                );


                /*
                 * Xử lý command
                 */

                mqtt_handle_command(payload);
            }

            break;
        }


        /* =================================================
         * DEFAULT
         * ================================================= */

        default:
        {
            break;
        }
    }
}

static bool safety_command_allowed(const char *command)
{
    if (command == NULL)
    {
        return false;
    }

    // =========================
    // GAS ALERT
    // =========================
    if (current_state == SYSTEM_MQ2_ALERT)
    {
        // Không cho Web tắt quạt
        if (strstr(command, "FAN_OFF") != NULL)
        {
            ESP_LOGW(
                TAG,
                "[SAFETY] GAS ALERT -> KHONG CHO FAN_OFF"
            );

            return false;
        }

        // Không cho Web tắt còi
        if (strstr(command, "BUZZER_OFF") != NULL)
        {
            ESP_LOGW(
                TAG,
                "[SAFETY] GAS ALERT -> KHONG CHO BUZZER_OFF"
            );

            return false;
        }
    }

    // =========================
    // FIRE ALERT
    // =========================
    if (current_state == SYSTEM_FIRE_ALERT)
    {
        // Không cho Web tắt bơm
        if (strstr(command, "PUMP_OFF") != NULL)
        {
            ESP_LOGW(
                TAG,
                "[SAFETY] FIRE ALERT -> KHONG CHO PUMP_OFF"
            );

            return false;
        }

        // Không cho Web tắt còi
        if (strstr(command, "BUZZER_OFF") != NULL)
        {
            ESP_LOGW(
                TAG,
                "[SAFETY] FIRE ALERT -> KHONG CHO BUZZER_OFF"
            );

            return false;
        }

        // Không cho Web đóng cửa
        if (strstr(command, "DOOR_CLOSE") != NULL)
        {
            ESP_LOGW(
                TAG,
                "[SAFETY] FIRE ALERT -> KHONG CHO DOOR_CLOSE"
            );

            return false;
        }
    }

    return true;
}

static void mqtt_handle_command(const char *payload)
{
    if (payload == NULL)
    {
        return;
    }

    ESP_LOGI(TAG, "[MQTT COMMAND] %s", payload);

    // ========================================================
    // KIEM TRA DEVICE ID
    // ========================================================

    if (strstr(payload, DEVICE_ID) == NULL)
    {
        ESP_LOGW(TAG, "[MQTT] Sai DEVICE_ID -> BO QUA");
        return;
    }

    // ========================================================
    // KIEM TRA SAFETY
    // ========================================================

    if (!safety_command_allowed(payload))
    {
        ESP_LOGW(TAG, "[SAFETY] COMMAND BI TU CHOI");
        return;
    }

    // ========================================================
    // FAN
    // ========================================================

    if (strstr(payload, "FAN_ON") != NULL)
    {
        manual_fan = true;

        relay_fan(true);

        ESP_LOGI(TAG, "[MANUAL] FAN -> ON");
    }
    else if (strstr(payload, "FAN_OFF") != NULL)
    {
        manual_fan = false;

        relay_fan(false);

        ESP_LOGI(TAG, "[MANUAL] FAN -> OFF");
    }

    // ========================================================
    // PUMP
    // ========================================================

    else if (strstr(payload, "PUMP_ON") != NULL)
    {
        manual_pump = true;

        relay_pump(true);

        ESP_LOGI(TAG, "[MANUAL] PUMP -> ON");
    }
    else if (strstr(payload, "PUMP_OFF") != NULL)
    {
        manual_pump = false;

        relay_pump(false);

        ESP_LOGI(TAG, "[MANUAL] PUMP -> OFF");
    }

    // ========================================================
    // BUZZER
    // ========================================================

    else if (strstr(payload, "BUZZER_ON") != NULL)
    {
        manual_buzzer = true;

        buzzer(true);

        ESP_LOGI(TAG, "[MANUAL] BUZZER -> ON");
    }
    else if (strstr(payload, "BUZZER_OFF") != NULL)
    {
        manual_buzzer = false;

        buzzer(false);

        ESP_LOGI(TAG, "[MANUAL] BUZZER -> OFF");
    }

    // ========================================================
    // DOOR
    // ========================================================

    else if (strstr(payload, "DOOR_OPEN") != NULL)
    {
        manual_door_open = true;

        servo_set_angle(90);

        ESP_LOGI(TAG, "[MANUAL] DOOR -> OPEN");
    }
    else if (strstr(payload, "DOOR_CLOSE") != NULL)
    {
        manual_door_open = false;

        servo_set_angle(0);

        ESP_LOGI(TAG, "[MANUAL] DOOR -> CLOSE");
    }

    // ========================================================
    // LIGHT
    // ========================================================

    else if (strstr(payload, "LIGHT_ON") != NULL)
    {
        manual_light = true;

        relay_light(true);

        ESP_LOGI(TAG, "[MANUAL] LIGHT -> ON");
    }
    else if (strstr(payload, "LIGHT_OFF") != NULL)
    {
        manual_light = false;

        relay_light(false);

        ESP_LOGI(TAG, "[MANUAL] LIGHT -> OFF");
    }

    // ========================================================
    // UNKNOWN COMMAND
    // ========================================================

    else
    {
        ESP_LOGW(TAG, "[MQTT] COMMAND KHONG HOP LE");
    }
}

/* =========================================================
 * KHOI TAO WIFI
 * ========================================================= */

static void wifi_init_sta(void)
{
    ESP_LOGI(TAG, "Khoi tao WiFi...");

    esp_err_t nvs_ret = nvs_flash_init();

    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(nvs_ret);

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(
        esp_event_loop_create_default()
    );

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg)
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL
        )
    );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );

    ESP_ERROR_CHECK(
        esp_wifi_start()
    );

    ESP_LOGI(TAG, "WiFi initialization OK");
}

/* =========================================================
 * KHOI TAO MQTT
 * ========================================================= */

static void mqtt_init(void)
{
    ESP_LOGI(TAG, "Khoi tao MQTT...");

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    if (mqtt_client == NULL)
    {
        ESP_LOGE(TAG, "Khong tao duoc MQTT client");
        return;
    }

    ESP_ERROR_CHECK(
        esp_mqtt_client_register_event(
            mqtt_client,
            ESP_EVENT_ANY_ID,
            mqtt_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_mqtt_client_start(mqtt_client)
    );

    ESP_LOGI(TAG, "MQTT initialization OK");
}

/* =========================================================
 * MQTT PUBLISH DEVICE
 * ========================================================= */

static void mqtt_publish_device(const char *status)
{
    if (!mqtt_connected || mqtt_client == NULL)
        return;

    char payload[256];

    snprintf(
        payload,
        sizeof(payload),
        "{\"device_id\":\"%s\",\"status\":\"%s\"}",
        DEVICE_ID,
        status
    );

    int msg_id = esp_mqtt_client_publish(
        mqtt_client,
        MQTT_DEVICE_TOPIC,
        payload,
        0,
        1,
        0
    );

    ESP_LOGI(
        TAG,
        "MQTT DEVICE: %s (msg_id=%d)",
        payload,
        msg_id
    );
}

/* =========================================================
 * MQTT PUBLISH SENSOR
 * ========================================================= */

static void mqtt_publish_sensor(
    float temperature,
    float humidity,
    int mq2,
    bool flame,
    bool pir
)
{
    if (!mqtt_connected || mqtt_client == NULL)
        return;

    char payload[384];

    snprintf(
        payload,
        sizeof(payload),
        "{"
        "\"device_id\":\"%s\","
        "\"temperature\":%.1f,"
        "\"humidity\":%.1f,"
        "\"mq2\":%d,"
        "\"flame\":%s,"
        "\"pir\":%s"
        "}",
        DEVICE_ID,
        temperature,
        humidity,
        mq2,
        flame ? "true" : "false",
        pir ? "true" : "false"
    );

    int msg_id = esp_mqtt_client_publish(
        mqtt_client,
        MQTT_SENSOR_TOPIC,
        payload,
        0,
        0,
        0
    );

    ESP_LOGI(
        TAG,
        "MQTT SENSOR: %s (msg_id=%d)",
        payload,
        msg_id
    );
}

/* =========================================================
 * MQTT PUBLISH EVENT
 * ========================================================= */

static void mqtt_publish_event(
    const char *event,
    const char *severity,
    const char *message
)
{
    if (!mqtt_connected || mqtt_client == NULL)
        return;

    char payload[512];

    snprintf(
        payload,
        sizeof(payload),
        "{"
        "\"device_id\":\"%s\","
        "\"event\":\"%s\","
        "\"severity\":\"%s\","
        "\"message\":\"%s\""
        "}",
        DEVICE_ID,
        event,
        severity,
        message
    );

    int msg_id = esp_mqtt_client_publish(
        mqtt_client,
        MQTT_EVENT_TOPIC,
        payload,
        0,
        1,
        0
    );

    ESP_LOGW(
        TAG,
        "MQTT EVENT: %s (msg_id=%d)",
        payload,
        msg_id
    );
}

/* =========================================================
 * MQTT PUBLISH CAMERA CAPTURE
 * =========================================================
 *
 * Khi ESP32 phát hiện FIRE:
 *
 * ESP32
 *   ↓ MQTT
 * iot/firegas/camera
 *   ↓
 * Laptop Camera Gateway
 *   ↓
 * Webcam Gsou
 *   ↓
 * Chụp ảnh
 */

static void mqtt_publish_camera_capture(void)
{
    if (!mqtt_connected || mqtt_client == NULL)
    {
        ESP_LOGW(
            TAG,
            "CAMERA: MQTT chua ket noi"
        );

        return;
    }

    char payload[256];

    snprintf(
        payload,
        sizeof(payload),
        "{"
        "\"device_id\":\"%s\","
        "\"event\":\"FIRE\","
        "\"camera\":\"CAPTURE\""
        "}",
        DEVICE_ID
    );

    int msg_id = esp_mqtt_client_publish(
        mqtt_client,
        MQTT_CAMERA_TOPIC,
        payload,
        0,
        1,
        0
    );

    ESP_LOGW(
        TAG,
        "MQTT CAMERA CAPTURE: %s (msg_id=%d)",
        payload,
        msg_id
    );
}


static adc_oneshot_unit_handle_t adc1_handle;

static ssd1306_t oled;
static bool oled_ready = false;
static i2s_chan_handle_t i2s_tx_handle = NULL;
static bool audio_ready = false;


/*
 * Đã phát cảnh báo âm thanh cho tình huống hiện tại chưa?
 *
 * false = chưa phát
 * true  = đã phát
 */
static bool audio_warning_played = false;

static void relay_light(bool on)
{
    // Đèn của bạn đang có logic:
    // GPIO HIGH -> ĐÈN BẬT
    // GPIO LOW  -> ĐÈN TẮT

    gpio_set_level(
        LIGHT_GPIO,
        on ? 1 : 0
    );

    ESP_LOGI(
        TAG,
        "LIGHT: %s",
        on ? "ON" : "OFF"
    );
}

/* =========================================================
 * 7. HÀM ĐIỀU KHIỂN RELAY
 * ========================================================= */

static void relay_fan(bool on)
{
    /* THEM MOI: Khi dang canh bao GAS, bat buoc quat ON.
     * Khong cho phep tat quat trong luc nguy hiem.
     */
    if (!on && current_state == SYSTEM_MQ2_ALERT)
    {
        ESP_LOGW(TAG, "EMERGENCY LOCK: KHONG CHO TAT QUAT KHI DANG CANH BAO GAS");
        on = true;
    }

#if RELAY_ACTIVE_LOW

    gpio_set_level(
        RELAY_FAN_GPIO,
        on ? 0 : 1
    );

#else

    gpio_set_level(
        RELAY_FAN_GPIO,
        on ? 1 : 0
    );

#endif

    ESP_LOGI(
        TAG,
        "QUAT: %s",
        on ? "ON" : "OFF"
    );
}


static void relay_pump(bool on)
{
    /* THEM MOI: Khi dang canh bao FIRE, bat buoc bom ON.
     * Khong cho phep tat bom trong luc nguy hiem.
     */
    if (!on && current_state == SYSTEM_FIRE_ALERT)
    {
        ESP_LOGW(TAG, "EMERGENCY LOCK: KHONG CHO TAT BOM KHI DANG CANH BAO FIRE");
        on = true;
    }

#if RELAY_ACTIVE_LOW

    gpio_set_level(
        RELAY_PUMP_GPIO,
        on ? 0 : 1
    );

#else

    gpio_set_level(
        RELAY_PUMP_GPIO,
        on ? 1 : 0
    );

#endif

    ESP_LOGI(
        TAG,
        "BOM: %s",
        on ? "ON" : "OFF"
    );
}


/* =========================================================
 * 8. BUZZER
 * ========================================================= */

static void buzzer(bool on)
{
#if BUZZER_ACTIVE_HIGH

    gpio_set_level(
        BUZZER_GPIO,
        on ? 1 : 0
    );

#else

    gpio_set_level(
        BUZZER_GPIO,
        on ? 0 : 1
    );

#endif
}


/* =========================================================
 * 9. SERVO
 * ========================================================= */

static uint32_t servo_angle_to_duty(int angle)
{
    /*
     * Servo:
     *
     * 0°   -> khoảng 500us
     * 90°  -> khoảng 1500us
     * 180° -> khoảng 2500us
     */

    uint32_t pulse_us =
        SERVO_MIN_US +
        ((SERVO_MAX_US - SERVO_MIN_US) * angle) / 180;

    /*
     * LEDC 50Hz:
     *
     * 1 chu kỳ = 20ms = 20,000us
     *
     * duty 16-bit:
     */
    uint32_t duty =
        (pulse_us * ((1U << 14) - 1U)) / 20000;

    return duty;
}


static void servo_set_angle(int angle)
{
    if (angle < 0)
        angle = 0;

    if (angle > 180)
        angle = 180;

    uint32_t duty =
        servo_angle_to_duty(angle);

    ledc_set_duty(
        LEDC_LOW_SPEED_MODE,
        LEDC_CHANNEL_0,
        duty
    );

    ledc_update_duty(
        LEDC_LOW_SPEED_MODE,
        LEDC_CHANNEL_0
    );

    ESP_LOGI(
        TAG,
        "SERVO: %d do",
        angle
    );
}


/* =========================================================
 * 10. KHỞI TẠO GPIO
 * ========================================================= */

static void gpio_init_all(void)
{
    /*
     * -------------------------
     * OUTPUT
     * -------------------------
     */

    gpio_config_t output_conf = {
        .pin_bit_mask =
            (1ULL << RELAY_FAN_GPIO) |
            (1ULL << RELAY_PUMP_GPIO) |
            (1ULL << BUZZER_GPIO) |
            (1ULL << LIGHT_GPIO),

        .mode = GPIO_MODE_OUTPUT,

        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,

        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&output_conf);


    /*
     * -------------------------
     * INPUT
     * -------------------------
     */

    gpio_config_t input_conf = {
        .pin_bit_mask =
            (1ULL << FLAME_GPIO) |
            (1ULL << PIR_GPIO),

        .mode = GPIO_MODE_INPUT,

        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,

        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&input_conf);


    /*
     * Trạng thái ban đầu
     */

    relay_fan(false);

    relay_pump(false);

    buzzer(false);

    relay_light(false);


    ESP_LOGI(
        TAG,
        "GPIO initialization OK"
    );
}


/* =========================================================
 * 11. KHỞI TẠO ADC
 * ========================================================= */

static void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(
            &adc_config,
            &adc1_handle
        )
    );


    /*
     * MQ-2 AO - GPIO1
     * MQ-2 chỉ có 1 chân AO nên chỉ cấu hình 1 kênh ADC.
     */
    adc_oneshot_chan_cfg_t mq2_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12
    };

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc1_handle,
            MQ2_ADC_CHANNEL,
            &mq2_config
        )
    );


    ESP_LOGI(
        TAG,
        "ADC initialization OK"
    );
}


/* =========================================================
 * 12. KHỞI TẠO SERVO
 * ========================================================= */

static void servo_init(void)
{
    /*
     * Timer LEDC
     */

    ledc_timer_config_t timer_config = {
        .speed_mode =
            LEDC_LOW_SPEED_MODE,

        .duty_resolution =
            LEDC_TIMER_14_BIT,

        .timer_num =
            LEDC_TIMER_0,

        .freq_hz =
            SERVO_FREQ,

        .clk_cfg =
            LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(
        ledc_timer_config(&timer_config)
    );


    /*
     * Channel
     */

    ledc_channel_config_t channel_config = {
        .gpio_num = SERVO_GPIO,

        .speed_mode =
            LEDC_LOW_SPEED_MODE,

        .channel =
            LEDC_CHANNEL_0,

        .intr_type =
            LEDC_INTR_DISABLE,

        .timer_sel =
            LEDC_TIMER_0,

        .duty = 0,

        .hpoint = 0
    };

    ESP_ERROR_CHECK(
        ledc_channel_config(&channel_config)
    );


    /*
     * Cửa đóng ban đầu
     */

    servo_set_angle(
        SERVO_ANGLE_CLOSED
    );


    ESP_LOGI(
        TAG,
        "SERVO initialization OK"
    );
}


/* =========================================================
 * 13. ĐỌC CẢM BIẾN
 * ========================================================= */

static void read_sensors(
    int *mq2_value,
    int *flame_value,
    int *pir_value,
    float *temperature,
    float *humidity
)
{    /* MQ-2 AO - chỉ đọc 1 chân GPIO1 */
    ESP_ERROR_CHECK(
        adc_oneshot_read(
            adc1_handle,
            MQ2_ADC_CHANNEL,
            mq2_value
        )
    );


    /*
     * Flame
     */

    *flame_value =
        gpio_get_level(FLAME_GPIO);


    /*
     * PIR
     */

    *pir_value =
        gpio_get_level(PIR_GPIO);


    /*
     * DHT22
     */

    esp_err_t result =
        dht22_read(
            temperature,
            humidity
        );

    if (result != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "Khong doc duoc DHT22"
        );

        *temperature = -999;

        *humidity = -999;
    }
}


/* =========================================================
 * 14. XÁC ĐỊNH TRẠNG THÁI
 * ========================================================= */

static system_state_t detect_state(
    int mq2_value,
    int flame_value
)
{
    bool mq2_detected =
        mq2_value > MQ2_THRESHOLD;


#if FLAME_ACTIVE_LOW

    bool fire_detected =
        (flame_value == 0);

#else

    bool fire_detected =
        (flame_value == 1);

#endif


    /*
     * FIRE có độ ưu tiên cao nhất
     */

    if (fire_detected)
    {
        return SYSTEM_FIRE_ALERT;
    }


    /*
     * MQ-2: khí/khói vượt ngưỡng
     */

    if (mq2_detected)
    {
        return SYSTEM_MQ2_ALERT;
    }


    return SYSTEM_NORMAL;
}


/* =========================================================
 * 15. ĐIỀU KHIỂN THEO TRẠNG THÁI
 * ========================================================= */

static void control_system(void)
{
    switch (current_state)
    {
        // ====================================================
        // NORMAL
        // ====================================================
        case SYSTEM_NORMAL:

            relay_fan(manual_fan);
            relay_pump(manual_pump);
            buzzer(manual_buzzer);

            if (manual_door_open)
            {
                servo_set_angle(90);
            }
            else
            {
                servo_set_angle(0);
            }

            relay_light(manual_light);

            ESP_LOGI(TAG,
                     "[NORMAL] FAN:%s PUMP:%s BUZ:%s DOOR:%s LIGHT:%s",
                     manual_fan ? "ON" : "OFF",
                     manual_pump ? "ON" : "OFF",
                     manual_buzzer ? "ON" : "OFF",
                     manual_door_open ? "OPEN" : "CLOSE",
                     manual_light ? "ON" : "OFF");

            break;


        // ====================================================
        // GAS ALERT
        // ====================================================
        case SYSTEM_MQ2_ALERT:

            relay_fan(true);
            relay_pump(false);
            buzzer(true);
            servo_set_angle(0);

            // Khi gas -> đèn tắt
            relay_light(false);

            ESP_LOGW(TAG,
                     "[GAS ALERT] FAN:ON PUMP:OFF BUZ:ON DOOR:CLOSE LIGHT:OFF");

            break;


        // ====================================================
        // FIRE ALERT
        // ====================================================
        case SYSTEM_FIRE_ALERT:

            relay_fan(false);
            relay_pump(true);
            buzzer(true);
            servo_set_angle(90);

            // Khi cháy -> bật đèn
            relay_light(true);

            ESP_LOGE(TAG,
                     "[FIRE ALERT] FAN:OFF PUMP:ON BUZ:ON DOOR:OPEN LIGHT:ON");

            break;


        default:

            relay_fan(false);
            relay_pump(false);
            buzzer(false);
            servo_set_angle(0);
            relay_light(false);

            break;
    }
}


/* =========================================================
 * 16. IN TRẠNG THÁI
 * ========================================================= */

static const char* state_to_string(
    system_state_t state
)
{
    switch (state)
    {
        case SYSTEM_NORMAL:
            return "NORMAL";

        case SYSTEM_MQ2_ALERT:
            return "MQ2 ALERT";

        case SYSTEM_FIRE_ALERT:
            return "FIRE ALERT";

        default:
            return "UNKNOWN";
    }
}


/* =========================================================
 * 17. OLED SSD1306
 * ========================================================= */

/*
 * OLED 0.96" 128x64 I2C
 *
 * VDD -> 3V3
 * GND -> GND
 * SCK -> GPIO12
 * SDA -> GPIO13
 *
 * Địa chỉ I2C: 0x3C
 */

/* Khai báo trước để OLED dùng được state_to_string() */
static const char* state_to_string(system_state_t state);


static void oled_init_display(void)
{
    ESP_LOGI(TAG, "Khoi tao OLED SSD1306...");

    esp_err_t err = ssd1306_init(
        &oled,
        OLED_SDA_GPIO,
        OLED_SCL_GPIO,
        OLED_I2C_ADDR
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OLED init that bai: %s", esp_err_to_name(err));
        oled_ready = false;
        return;
    }

    oled_ready = true;
    ESP_LOGI(TAG, "OLED SSD1306 OK - I2C 0x%02X", OLED_I2C_ADDR);
}

static void oled_update_display(
    float temperature,
    float humidity,
    int mq2,
    int flame,
    int person,
    system_state_t state,
    bool fan_on,
    bool pump_on,
    bool buzzer_on,
    int door_angle)
{
    if (!oled_ready) {
        return;
    }

    char line[32];

    ssd1306_clear(&oled);

    ssd1306_draw_text(&oled, 0, 0, "FIRE GAS SYSTEM");

    snprintf(line, sizeof(line), "T:%.1fC H:%.1f%%",
             temperature, humidity);
    ssd1306_draw_text(&oled, 0, 8, line);

    snprintf(line, sizeof(line), "MQ2:%d", mq2);
    ssd1306_draw_text(&oled, 0, 16, line);

    snprintf(line, sizeof(line), "FLAME:%s PIR:%s",
             flame ? "ON" : "OFF",
             person ? "ON" : "OFF");
    ssd1306_draw_text(&oled, 0, 24, line);

    snprintf(line, sizeof(line), "STATE:%s",
             state_to_string(state));
    ssd1306_draw_text(&oled, 0, 32, line);

    snprintf(line, sizeof(line), "FAN:%s PUMP:%s",
             fan_on ? "ON" : "OFF",
             pump_on ? "ON" : "OFF");
    ssd1306_draw_text(&oled, 0, 40, line);

    snprintf(line, sizeof(line), "BUZ:%s DOOR:%s",
             buzzer_on ? "ON" : "OFF",
             door_angle > 0 ? "OPEN" : "CLOSE");
    ssd1306_draw_text(&oled, 0, 48, line);

    if (ssd1306_display(&oled) != ESP_OK) {
        ESP_LOGW(TAG, "Khong gui duoc du lieu OLED");
    }
}


static void print_dashboard(
    int mq2_value,
    int flame_value,
    int pir_value,
    float temperature,
    float humidity,
    system_state_t state
)
{
    printf("\n");
    printf("====================================================\n");
    printf("          IOT GAS / FIRE SAFETY SYSTEM\n");
    printf("====================================================\n");

    printf(
        "Temperature : %.1f C\n",
        temperature
    );

    printf(
        "Humidity    : %.1f %%\n",
        humidity
    );

    printf(
        "MQ-2 AO     : %d\n",
        mq2_value
    );

    printf(
        "Flame       : %d\n",
        flame_value
    );

    printf(
        "Person      : %d\n",
        pir_value
    );

    printf(
        "----------------------------------------------------\n"
    );

    printf(
        "SYSTEM STATE: %s\n",
        state_to_string(state)
    );

    printf(
        "----------------------------------------------------\n"
    );

    /*
     * =====================================================
     * NORMAL
     * =====================================================
     */

    if (state == SYSTEM_NORMAL)
    {
        printf(
            "Fan         : %s\n",
            manual_fan ? "ON" : "OFF"
        );

        printf(
            "Pump        : %s\n",
            manual_pump ? "ON" : "OFF"
        );

        printf(
            "Buzzer      : %s\n",
            manual_buzzer ? "ON" : "OFF"
        );

        printf(
            "Door        : %s\n",
            manual_door_open ? "OPEN" : "CLOSED"
        );
    }

    /*
     * =====================================================
     * GAS ALERT
     * =====================================================
     */

    else if (state == SYSTEM_MQ2_ALERT)
    {
        printf("Fan         : ON\n");
        printf("Pump        : OFF\n");
        printf("Buzzer      : ON\n");
        printf("Door        : CLOSED\n");
    }

    /*
     * =====================================================
     * FIRE ALERT
     * =====================================================
     */

    else if (state == SYSTEM_FIRE_ALERT)
    {
        printf("Fan         : OFF\n");
        printf("Pump        : ON\n");
        printf("Buzzer      : ON\n");
        printf("Door        : OPEN\n");
    }

    printf("====================================================\n");
}

/* =========================================================
 * MAX98357A - I2S AUDIO
 * ========================================================= */

static void audio_init(void)
{
    ESP_LOGI(TAG, "Khoi tao MAX98357A...");

    /*
     * Tạo I2S TX channel
     */
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(
            I2S_NUM_0,
            I2S_ROLE_MASTER
        );

    esp_err_t err =
        i2s_new_channel(
            &chan_cfg,
            &i2s_tx_handle,
            NULL
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Khong tao duoc I2S channel: %s",
            esp_err_to_name(err)
        );

        audio_ready = false;
        return;
    }

    /*
     * Cấu hình I2S standard mode
     *
     * 16-bit
     * 16 kHz
     * Stereo
     */
    i2s_std_config_t std_cfg = {
        .clk_cfg =
            I2S_STD_CLK_DEFAULT_CONFIG(16000),

        .slot_cfg =
            I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                I2S_DATA_BIT_WIDTH_16BIT,
                I2S_SLOT_MODE_STEREO
            ),

        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,

            .bclk =
                I2S_BCLK_GPIO,

            .ws =
                I2S_LRC_GPIO,

            .dout =
                I2S_DOUT_GPIO,

            .din =
                I2S_GPIO_UNUSED,

            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
        }
    };

    err =
        i2s_channel_init_std_mode(
            i2s_tx_handle,
            &std_cfg
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "I2S init that bai: %s",
            esp_err_to_name(err)
        );

        audio_ready = false;
        return;
    }

    err =
        i2s_channel_enable(
            i2s_tx_handle
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "I2S enable that bai: %s",
            esp_err_to_name(err)
        );

        audio_ready = false;
        return;
    }

    audio_ready = true;

    ESP_LOGI(
        TAG,
        "MAX98357A I2S OK - BCLK=%d LRC=%d DIN=%d",
        I2S_BCLK_GPIO,
        I2S_LRC_GPIO,
        I2S_DOUT_GPIO
    );
}

/* =========================================================
 * AUDIO WAV
 * ========================================================= */

/*
 * Các file WAV được ESP-IDF embed trực tiếp vào firmware.
 *
 * Nội dung:
 *
 * gas_warning.wav:
 * "Phát hiện rò rỉ khí gas, hãy kiểm tra."
 *
 * fire_warning.wav:
 * "Phát hiện có người trong khu vực nguy hiểm,
 *  vui lòng rời khỏi đây."
 */

extern const uint8_t gas_warning_wav_start[]
    asm("_binary_gas_warning_wav_start");

extern const uint8_t gas_warning_wav_end[]
    asm("_binary_gas_warning_wav_end");


extern const uint8_t fire_warning_wav_start[]
    asm("_binary_fire_warning_wav_start");

extern const uint8_t fire_warning_wav_end[]
    asm("_binary_fire_warning_wav_end");


/*
 * Đọc số nguyên little-endian 16-bit
 */

static uint16_t read_le16(
    const uint8_t *data
)
{
    return
        ((uint16_t)data[0]) |
        ((uint16_t)data[1] << 8);
}


/*
 * Đọc số nguyên little-endian 32-bit
 */

static uint32_t read_le32(
    const uint8_t *data
)
{
    return
        ((uint32_t)data[0]) |
        ((uint32_t)data[1] << 8) |
        ((uint32_t)data[2] << 16) |
        ((uint32_t)data[3] << 24);
}


/*
 * Phát file WAV
 *
 * WAV yêu cầu:
 *
 * PCM
 * 16-bit
 * Mono hoặc Stereo
 */

static void audio_play_wav(
    const uint8_t *wav_start,
    const uint8_t *wav_end
)
{
    if (!audio_ready || i2s_tx_handle == NULL)
    {
        ESP_LOGW(
            TAG,
            "Audio chua san sang"
        );

        return;
    }


    size_t wav_size =
        wav_end - wav_start;


    if (wav_size < 44)
    {
        ESP_LOGE(
            TAG,
            "File WAV khong hop le"
        );

        return;
    }


    /*
     * Kiểm tra RIFF
     */

    if (
        wav_start[0] != 'R' ||
        wav_start[1] != 'I' ||
        wav_start[2] != 'F' ||
        wav_start[3] != 'F'
    )
    {
        ESP_LOGE(
            TAG,
            "Khong phai file WAV RIFF"
        );

        return;
    }


    /*
     * Kiểm tra WAVE
     */

    if (
        wav_start[8] != 'W' ||
        wav_start[9] != 'A' ||
        wav_start[10] != 'V' ||
        wav_start[11] != 'E'
    )
    {
        ESP_LOGE(
            TAG,
            "WAV khong co WAVE header"
        );

        return;
    }


    /*
     * Tìm chunk "fmt "
     * và "data"
     */

    const uint8_t *ptr =
        wav_start + 12;

    const uint8_t *end =
        wav_end;


    uint16_t audio_format = 0;
    uint16_t channels = 0;
    uint32_t sample_rate = 0;
    uint16_t bits_per_sample = 0;

    const uint8_t *data_ptr = NULL;
    uint32_t data_size = 0;


    while (ptr + 8 <= end)
    {
        uint32_t chunk_size =
            read_le32(ptr + 4);


        /*
         * fmt
         */

        if (
            ptr[0] == 'f' &&
            ptr[1] == 'm' &&
            ptr[2] == 't' &&
            ptr[3] == ' '
        )
        {
            if (chunk_size >= 16)
            {
                audio_format =
                    read_le16(ptr + 8);

                channels =
                    read_le16(ptr + 10);

                sample_rate =
                    read_le32(ptr + 12);

                bits_per_sample =
                    read_le16(ptr + 22);
            }
        }


        /*
         * data
         */

        if (
            ptr[0] == 'd' &&
            ptr[1] == 'a' &&
            ptr[2] == 't' &&
            ptr[3] == 'a'
        )
        {
            data_ptr =
                ptr + 8;

            data_size =
                chunk_size;

            if (data_ptr + data_size > end)
            {
                data_size =
                    end - data_ptr;
            }

            break;
        }


        ptr += 8 + chunk_size;


        /*
         * WAV chunk phải align 2 byte
         */

        if ((chunk_size & 1) != 0)
        {
            ptr++;
        }
    }


    /*
     * Kiểm tra format
     */

    if (
        audio_format != 1 ||
        bits_per_sample != 16 ||
        data_ptr == NULL
    )
    {
        ESP_LOGE(
            TAG,
            "WAV khong phu hop: format=%d channels=%d sample=%lu bits=%d",
            audio_format,
            channels,
            (unsigned long)sample_rate,
            bits_per_sample
        );

        return;
    }


    ESP_LOGI(
        TAG,
        "Phat WAV: %lu Hz, %d channel, %d bit, %lu bytes",
        (unsigned long)sample_rate,
        channels,
        bits_per_sample,
        (unsigned long)data_size
    );


    /*
     * MAX98357A đang chạy 16-bit.
     *
     * Nếu WAV là 16kHz thì không cần đổi sample rate.
     */

    if (sample_rate != 16000)
    {
        ESP_LOGW(
            TAG,
            "WAV khong phai 16kHz"
        );
    }


    /*
     * WAV mono:
     *
     * ESP32 I2S đang cấu hình stereo.
     *
     * Vì vậy cần nhân đôi sample:
     *
     * Mono sample
     *     ↓
     * Left
     * Right
     */

    const int16_t *samples =
        (const int16_t *)data_ptr;

    /*
     * Với stereo:
     * 1 frame = Left + Right = 2 sample 16-bit.
     *
     * Với mono:
     * 1 frame = 1 sample.
     */
    size_t frame_count =
        data_size / (sizeof(int16_t) * channels);

    const size_t buffer_frames = 512;

    int16_t buffer[
        buffer_frames * 2
    ];

    size_t position = 0;

    while (position < frame_count)
    {
        size_t count =
            frame_count - position;

        if (count > buffer_frames)
        {
            count = buffer_frames;
        }

        /*
         * Nếu WAV mono:
         * nhân đôi sample thành Left + Right.
         */
        if (channels == 1)
        {
            for (size_t i = 0; i < count; i++)
            {
                int16_t sample =
                    samples[position + i];

                buffer[i * 2] =
                    sample;

                buffer[i * 2 + 1] =
                    sample;
            }
        }

        /*
         * Nếu WAV stereo:
         * mỗi frame đã có L/R.
         */
        else if (channels == 2)
        {
            memcpy(
                buffer,
                samples + position * 2,
                count * 2 * sizeof(int16_t)
            );
        }

        else
        {
            ESP_LOGE(
                TAG,
                "So channel khong ho tro: %d",
                channels
            );

            return;
        }

        size_t bytes_written = 0;

        esp_err_t err =
            i2s_channel_write(
                i2s_tx_handle,
                buffer,
                count * 2 * sizeof(int16_t),
                &bytes_written,
                portMAX_DELAY
            );

        if (err != ESP_OK)
        {
            ESP_LOGE(
                TAG,
                "Loi phat WAV: %s",
                esp_err_to_name(err)
            );

            break;
        }

        position += count;
    }


    ESP_LOGI(
        TAG,
        "Phat WAV xong"
    );
}

/* =========================================================
 * CẢNH BÁO GAS
 * ========================================================= */

static void audio_gas_warning(void)
{
    ESP_LOGW(
        TAG,
        "AUDIO: PHAT HIEN RO RI KHI GAS"
    );

    audio_play_wav(
        gas_warning_wav_start,
        gas_warning_wav_end
    );
}


/* =========================================================
 * CẢNH BÁO CHÁY
 * ========================================================= */

static void audio_fire_warning(void)
{
    ESP_LOGW(
        TAG,
        "AUDIO: CO NGUOI TRONG KHU VUC NGUY HIEM"
    );

    audio_play_wav(
        fire_warning_wav_start,
        fire_warning_wav_end
    );
}


/* =========================================================
 * 18. MAIN
 * ========================================================= */

void app_main(void)
{
    ESP_LOGI(
        TAG,
        "========================================"
    );

    ESP_LOGI(
        TAG,
        "   IOT GAS / FIRE SAFETY SYSTEM"
    );

    ESP_LOGI(
        TAG,
        "========================================"
    );


    /*
     * Khởi tạo GPIO
     */

    gpio_init_all();


    /*
     * Khởi tạo ADC
     */

    adc_init();


    /*
     * Khởi tạo Servo
     */

    servo_init();


    /*
     * Khởi tạo DHT22
     */

    ESP_ERROR_CHECK(
        dht22_init(DHT22_GPIO)
    );


    /*
    * Khởi tạo OLED SSD1306
    */
    oled_init_display();

    /*
    * Khởi tạo MAX98357A
    */
    audio_init();

    /*
     * Khoi tao WiFi.
     *
     * WiFi phai duoc khoi tao truoc MQTT.
     */
    wifi_init_sta();

    /*
     * Khoi tao MQTT.
     */
    mqtt_init();

    ESP_LOGI(
        TAG,
        "SYSTEM READY"
    );


    /*
     * Cho MQ-2 thời gian ổn định
     */

    ESP_LOGW(
        TAG,
        "MQ-2 dang khoi dong..."
    );

    vTaskDelay(
        pdMS_TO_TICKS(10000)
    );


    ESP_LOGI(
        TAG,
        "BAT DAU GIAM SAT"
    );


    /* =====================================================
     * MAIN LOOP
     * ===================================================== */

    while (1)
    {
        int mq2_value = 0;
        int flame_value = 0;
        int pir_value = 0;

        float temperature = 0;
        float humidity = 0;

        /*
        * Đọc toàn bộ cảm biến
        */
        read_sensors(
            &mq2_value,
            &flame_value,
            &pir_value,
            &temperature,
            &humidity
        );

        /*
        * Xác định có lửa hay không
        */
    #if FLAME_ACTIVE_LOW
        bool fire_detected = (flame_value == 0);
    #else
        bool fire_detected = (flame_value == 1);
    #endif

        /*
        * PIR
        */
    #if PIR_ACTIVE_HIGH
        bool person_detected =
            (pir_value == 1);
    #else
        bool person_detected =
            (pir_value == 0);
    #endif

        /*
        * Xác định trạng thái
        */
        system_state_t new_state =
            detect_state(
                mq2_value,
                flame_value
            );

        /*
        * Nếu trạng thái thay đổi
        */
        if (new_state != current_state)
        {
            ESP_LOGW(
                TAG,
                "STATE CHANGE: %s -> %s",
                state_to_string(current_state),
                state_to_string(new_state)
            );

            current_state = new_state;

            /*
            * Trạng thái mới
            * -> cho phép phát cảnh báo mới
            */
            audio_warning_played = false;


            /* =====================================================
            * GAS ALERT
            * ===================================================== */

            if (current_state == SYSTEM_MQ2_ALERT)
            {
                mqtt_publish_event(
                    "GAS",
                    "WARNING",
                    "Phat hien khi gas hoac khoi"
                );
            }


            /* =====================================================
            * FIRE ALERT
            * ===================================================== */

            else if (current_state == SYSTEM_FIRE_ALERT)
            {
                mqtt_publish_event(
                    "FIRE",
                    "CRITICAL",
                    "Phat hien lua"
                );

                /*
                * =================================================
                * GUI LENH CHUP ANH
                * =================================================
                *
                * Chỉ gửi 1 lần khi:
                *
                * NORMAL/GAS
                *      ↓
                * FIRE
                *
                * Camera Gateway trên laptop sẽ nhận:
                *
                * {
                *   "device_id":"esp32-001",
                *   "event":"FIRE",
                *   "camera":"CAPTURE"
                * }
                */

                mqtt_publish_camera_capture();
            }


            /* =====================================================
            * NORMAL
            * ===================================================== */

            else if (current_state == SYSTEM_NORMAL)
            {
                mqtt_publish_event(
                    "NORMAL",
                    "INFO",
                    "He thong tro lai binh thuong"
                );
            }
        }

        /*
        * Điều khiển thiết bị
        */
       control_system();

                /*
         * =====================================================
         * GỬI DỮ LIỆU SENSOR QUA MQTT
         * =====================================================
         */

        mqtt_publish_sensor(
            temperature,
            humidity,
            mq2_value,
            fire_detected,
            person_detected
        );

        /*
        * =====================================================
        * CẢNH BÁO BẰNG GIỌNG NÓI
        * =====================================================
        *
        * GAS + CÓ NGƯỜI
        * ->
        * "Phát hiện rò rỉ khí gas, hãy kiểm tra."
        *
        *
        * FIRE + CÓ NGƯỜI
        * ->
        * "Phát hiện có người trong khu vực nguy hiểm,
        *  vui lòng rời khỏi đây."
        *
        *
        * Chỉ phát 1 lần khi trạng thái nguy hiểm
        * được phát hiện.
        */

        if (
            person_detected &&
            !audio_warning_played
        )
        {
            /*
            * GAS + NGƯỜI
            */

            if (
                current_state == SYSTEM_MQ2_ALERT
            )
            {
                ESP_LOGW(
                    TAG,
                    "!!! CO NGUOI + RO RI KHI GAS !!!"
                );

                audio_gas_warning();

                audio_warning_played = true;
            }


            /*
            * FIRE + NGƯỜI
            */

            else if (
                current_state == SYSTEM_FIRE_ALERT
            )
            {
                ESP_LOGW(
                    TAG,
                    "!!! CO NGUOI TRONG KHU VUC CHAY !!!"
                );

                audio_fire_warning();

                audio_warning_played = true;
            }
        }

        /*
        * Nếu không còn người
        * -> cho phép phát lại khi người quay lại.
        */
        if (!person_detected)
        {
            audio_warning_played = false;
        }

        /*
        * In dashboard
        */
        print_dashboard(
            mq2_value,
            flame_value,
            pir_value,
            temperature,
            humidity,
            current_state
        );

        bool fan_on;
        bool pump_on;
        bool buzzer_on;
        int door_angle;

        /*
        * NORMAL:
        * Hiển thị theo lệnh MANUAL từ MQTT/App
        */
        if (current_state == SYSTEM_NORMAL)
        {
            fan_on = manual_fan;
            pump_on = manual_pump;
            buzzer_on = manual_buzzer;

            door_angle =
                manual_door_open
                ? SERVO_ANGLE_OPEN
                : SERVO_ANGLE_CLOSED;
        }

        /*
        * GAS:
        * Quạt + buzzer bắt buộc ON
        */
        else if (current_state == SYSTEM_MQ2_ALERT)
        {
            fan_on = true;
            pump_on = false;
            buzzer_on = true;
            door_angle = SERVO_ANGLE_CLOSED;
        }

        /*
        * FIRE:
        * Bơm + buzzer + cửa bắt buộc
        */
        else
        {
            fan_on = false;
            pump_on = true;
            buzzer_on = true;
            door_angle = SERVO_ANGLE_OPEN;
        }


        oled_update_display(
            temperature,
            humidity,
            mq2_value,
            fire_detected,
            person_detected,
            current_state,
            fan_on,
            pump_on,
            buzzer_on,
            door_angle
        );

        /*
        * Chờ 3 giây
        */
        vTaskDelay(
            pdMS_TO_TICKS(3000)
        );
    }
}