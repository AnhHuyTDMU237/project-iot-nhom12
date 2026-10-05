#include "dht22.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"


// =====================================================
// LOG TAG
// =====================================================

static const char *TAG = "DHT22";


// =====================================================
// BIẾN DRIVER
// =====================================================

static gpio_num_t dht_gpio;


// =====================================================
// HÀM ĐỢI GPIO ĐẠT MỨC
// =====================================================

static bool wait_for_level(
    int level,
    uint32_t timeout_us
)
{
    int64_t start = esp_timer_get_time();

    while (gpio_get_level(dht_gpio) != level)
    {
        if ((esp_timer_get_time() - start) >= timeout_us)
        {
            return false;
        }
    }

    return true;
}


// =====================================================
// INIT
// =====================================================

esp_err_t dht22_init(gpio_num_t gpio)
{
    dht_gpio = gpio;

    gpio_config_t config = {
        .pin_bit_mask = (1ULL << gpio),

        .mode = GPIO_MODE_INPUT_OUTPUT_OD,

        .pull_up_en = GPIO_PULLUP_ENABLE,

        .pull_down_en = GPIO_PULLDOWN_DISABLE,

        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t ret = gpio_config(&config);

    if (ret != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Khong the cau hinh GPIO %d",
            gpio
        );

        return ret;
    }


    // Đưa DATA về trạng thái HIGH khi không đọc

    gpio_set_level(dht_gpio, 1);

    return ESP_OK;
}


// =====================================================
// ĐỌC DHT22
// =====================================================

esp_err_t dht22_read(
    float *temperature,
    float *humidity
)
{
    if (temperature == NULL || humidity == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }


    uint8_t data[5] = {0};


    // =================================================
    // BƯỚC 1
    // ESP32 kéo DATA xuống LOW
    // =================================================

    gpio_set_direction(
        dht_gpio,
        GPIO_MODE_OUTPUT_OD
    );

    gpio_set_level(
        dht_gpio,
        0
    );


    /*
     * DHT22 yêu cầu tín hiệu START
     * khoảng tối thiểu 1 ms.
     */

    esp_rom_delay_us(2000);


    // =================================================
    // BƯỚC 2
    // Nhả chân DATA
    // =================================================

    gpio_set_direction(
        dht_gpio,
        GPIO_MODE_INPUT
    );


    /*
     * Chờ DHT22 phản hồi LOW
     */

    if (!wait_for_level(0, 100))
    {
        ESP_LOGE(
            TAG,
            "DHT22 khong phan hoi LOW"
        );

        return ESP_ERR_TIMEOUT;
    }


    /*
     * Chờ DHT22 lên HIGH
     */

    if (!wait_for_level(1, 100))
    {
        ESP_LOGE(
            TAG,
            "DHT22 khong phan hoi HIGH"
        );

        return ESP_ERR_TIMEOUT;
    }


    /*
     * Chờ bắt đầu bit dữ liệu
     */

    if (!wait_for_level(0, 100))
    {
        ESP_LOGE(
            TAG,
            "DHT22 khong bat dau data"
        );

        return ESP_ERR_TIMEOUT;
    }


    // =================================================
    // BƯỚC 3
    // ĐỌC 40 BIT
    // =================================================

    for (int i = 0; i < 40; i++)
    {
        /*
         * Mỗi bit bắt đầu bằng LOW.
         */

        if (!wait_for_level(1, 100))
        {
            ESP_LOGE(
                TAG,
                "Timeout tai bit %d",
                i
            );

            return ESP_ERR_TIMEOUT;
        }


        /*
         * Đợi khoảng thời gian HIGH.
         *
         * DHT22:
         *
         * ~26-28 us → bit 0
         * ~70 us    → bit 1
         */

        int64_t start = esp_timer_get_time();

        while (gpio_get_level(dht_gpio) == 1)
        {
            /*
             * Nếu HIGH quá lâu
             * coi như lỗi.
             */

            if (
                (esp_timer_get_time() - start)
                > 120
            )
            {
                ESP_LOGE(
                    TAG,
                    "HIGH pulse qua dai"
                );

                return ESP_ERR_TIMEOUT;
            }
        }


        int64_t pulse_width =
            esp_timer_get_time() - start;


        /*
         * Khoảng giữa thường:
         *
         * > 50 us → bit 1
         * <= 50 us → bit 0
         */

        int bit_value =
            (pulse_width > 50) ? 1 : 0;


        data[i / 8] <<= 1;

        data[i / 8] |= bit_value;
    }


    // =================================================
    // BƯỚC 4
    // CHECKSUM
    // =================================================

    uint8_t checksum =
        data[0] +
        data[1] +
        data[2] +
        data[3];


    if (checksum != data[4])
    {
        ESP_LOGE(
            TAG,
            "Checksum sai: calculated=%d received=%d",
            checksum,
            data[4]
        );

        return ESP_FAIL;
    }


    // =================================================
    // BƯỚC 5
    // ĐỔI DỮ LIỆU
    // =================================================

    uint16_t raw_humidity =
        ((uint16_t)data[0] << 8)
        | data[1];


    uint16_t raw_temperature =
        ((uint16_t)data[2] << 8)
        | data[3];


    *humidity =
        raw_humidity / 10.0f;


    /*
     * Bit cao nhất của temperature
     * biểu thị số âm.
     */

    if (raw_temperature & 0x8000)
    {
        raw_temperature &= 0x7FFF;

        *temperature =
            -(raw_temperature / 10.0f);
    }
    else
    {
        *temperature =
            raw_temperature / 10.0f;
    }


    ESP_LOGI(
        TAG,
        "Temperature: %.1f C | Humidity: %.1f %%",
        *temperature,
        *humidity
    );


    return ESP_OK;
}
