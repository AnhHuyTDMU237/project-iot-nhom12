#ifndef SSD1306_H
#define SSD1306_H

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdint.h>

#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 64

typedef struct {
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    uint8_t address;
    uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
} ssd1306_t;

/**
 * Khởi tạo OLED SSD1306 I2C 128x64.
 *
 * sda_gpio: GPIO SDA
 * scl_gpio: GPIO SCL
 * address : thường là 0x3C
 */
esp_err_t ssd1306_init(ssd1306_t *dev,
                       gpio_num_t sda_gpio,
                       gpio_num_t scl_gpio,
                       uint8_t address);

/* Xóa toàn bộ framebuffer */
void ssd1306_clear(ssd1306_t *dev);

/* Vẽ text ASCII 5x7 tại vị trí x,y; y nên là 0,8,16,... */
void ssd1306_draw_text(ssd1306_t *dev,
                       int x,
                       int y,
                       const char *text);

/* Gửi framebuffer lên OLED */
esp_err_t ssd1306_display(ssd1306_t *dev);

/* Tắt OLED */
esp_err_t ssd1306_deinit(ssd1306_t *dev);

#endif
