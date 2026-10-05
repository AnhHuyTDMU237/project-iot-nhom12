#ifndef DHT22_H
#define DHT22_H

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * @brief Khởi tạo driver DHT22
 *
 * @param gpio GPIO kết nối chân DATA của DHT22
 */
esp_err_t dht22_init(gpio_num_t gpio);

/**
 * @brief Đọc nhiệt độ và độ ẩm từ DHT22
 *
 * @param temperature Con trỏ lưu nhiệt độ (°C)
 * @param humidity    Con trỏ lưu độ ẩm (%)
 *
 * @return
 *      ESP_OK nếu đọc thành công
 *      ESP_ERR_TIMEOUT nếu timeout
 *      ESP_FAIL nếu dữ liệu checksum sai
 */
esp_err_t dht22_read(float *temperature, float *humidity);

#endif // DHT22_H
