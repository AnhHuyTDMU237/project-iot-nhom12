#include "ssd1306.h"

#include "esp_log.h"
#include <string.h>

static const char *TAG = "SSD1306";

#define SSD1306_CTRL_CMD   0x00
#define SSD1306_CTRL_DATA  0x40

/* 5x7 font cho các ký tự cần dùng trong dashboard */
static uint8_t font_char(char c)
{
    (void)c;
    return 0;
}

/*
 * Trả về 5 cột của ký tự 5x7.
 * Bit thấp nằm ở hàng trên.
 */
static void get_glyph(char c, uint8_t g[5])
{
    memset(g, 0, 5);

    if (c >= 'a' && c <= 'z')
        c -= ('a' - 'A');

    switch (c) {
        case 'A': g[0]=0x7E; g[1]=0x11; g[2]=0x11; g[3]=0x11; g[4]=0x7E; break;
        case 'B': g[0]=0x7F; g[1]=0x49; g[2]=0x49; g[3]=0x49; g[4]=0x36; break;
        case 'C': g[0]=0x3E; g[1]=0x41; g[2]=0x41; g[3]=0x41; g[4]=0x22; break;
        case 'D': g[0]=0x7F; g[1]=0x41; g[2]=0x41; g[3]=0x22; g[4]=0x1C; break;
        case 'E': g[0]=0x7F; g[1]=0x49; g[2]=0x49; g[3]=0x49; g[4]=0x41; break;
        case 'F': g[0]=0x7F; g[1]=0x09; g[2]=0x09; g[3]=0x09; g[4]=0x01; break;
        case 'G': g[0]=0x3E; g[1]=0x41; g[2]=0x49; g[3]=0x49; g[4]=0x7A; break;
        case 'H': g[0]=0x7F; g[1]=0x08; g[2]=0x08; g[3]=0x08; g[4]=0x7F; break;
        case 'I': g[0]=0x00; g[1]=0x41; g[2]=0x7F; g[3]=0x41; g[4]=0x00; break;
        case 'J': g[0]=0x20; g[1]=0x40; g[2]=0x41; g[3]=0x3F; g[4]=0x01; break;
        case 'K': g[0]=0x7F; g[1]=0x08; g[2]=0x14; g[3]=0x22; g[4]=0x41; break;
        case 'L': g[0]=0x7F; g[1]=0x40; g[2]=0x40; g[3]=0x40; g[4]=0x40; break;
        case 'M': g[0]=0x7F; g[1]=0x02; g[2]=0x0C; g[3]=0x02; g[4]=0x7F; break;
        case 'N': g[0]=0x7F; g[1]=0x04; g[2]=0x08; g[3]=0x10; g[4]=0x7F; break;
        case 'O': g[0]=0x3E; g[1]=0x41; g[2]=0x41; g[3]=0x41; g[4]=0x3E; break;
        case 'P': g[0]=0x7F; g[1]=0x09; g[2]=0x09; g[3]=0x09; g[4]=0x06; break;
        case 'Q': g[0]=0x3E; g[1]=0x41; g[2]=0x51; g[3]=0x21; g[4]=0x5E; break;
        case 'R': g[0]=0x7F; g[1]=0x09; g[2]=0x19; g[3]=0x29; g[4]=0x46; break;
        case 'S': g[0]=0x46; g[1]=0x49; g[2]=0x49; g[3]=0x49; g[4]=0x31; break;
        case 'T': g[0]=0x01; g[1]=0x01; g[2]=0x7F; g[3]=0x01; g[4]=0x01; break;
        case 'U': g[0]=0x3F; g[1]=0x40; g[2]=0x40; g[3]=0x40; g[4]=0x3F; break;
        case 'V': g[0]=0x1F; g[1]=0x20; g[2]=0x40; g[3]=0x20; g[4]=0x1F; break;
        case 'W': g[0]=0x3F; g[1]=0x40; g[2]=0x38; g[3]=0x40; g[4]=0x3F; break;
        case 'X': g[0]=0x63; g[1]=0x14; g[2]=0x08; g[3]=0x14; g[4]=0x63; break;
        case 'Y': g[0]=0x07; g[1]=0x08; g[2]=0x70; g[3]=0x08; g[4]=0x07; break;
        case 'Z': g[0]=0x61; g[1]=0x51; g[2]=0x49; g[3]=0x45; g[4]=0x43; break;

        case '0': g[0]=0x3E; g[1]=0x51; g[2]=0x49; g[3]=0x45; g[4]=0x3E; break;
        case '1': g[0]=0x00; g[1]=0x42; g[2]=0x7F; g[3]=0x40; g[4]=0x00; break;
        case '2': g[0]=0x42; g[1]=0x61; g[2]=0x51; g[3]=0x49; g[4]=0x46; break;
        case '3': g[0]=0x21; g[1]=0x41; g[2]=0x45; g[3]=0x4B; g[4]=0x31; break;
        case '4': g[0]=0x18; g[1]=0x14; g[2]=0x12; g[3]=0x7F; g[4]=0x10; break;
        case '5': g[0]=0x27; g[1]=0x45; g[2]=0x45; g[3]=0x45; g[4]=0x39; break;
        case '6': g[0]=0x3C; g[1]=0x4A; g[2]=0x49; g[3]=0x49; g[4]=0x30; break;
        case '7': g[0]=0x01; g[1]=0x71; g[2]=0x09; g[3]=0x05; g[4]=0x03; break;
        case '8': g[0]=0x36; g[1]=0x49; g[2]=0x49; g[3]=0x49; g[4]=0x36; break;
        case '9': g[0]=0x06; g[1]=0x49; g[2]=0x49; g[3]=0x29; g[4]=0x1E; break;

        case ':': g[0]=0x00; g[1]=0x36; g[2]=0x36; g[3]=0x00; g[4]=0x00; break;
        case '.': g[0]=0x00; g[1]=0x40; g[2]=0x60; g[3]=0x00; g[4]=0x00; break;
        case '%': g[0]=0x63; g[1]=0x13; g[2]=0x08; g[3]=0x64; g[4]=0x63; break;
        case '!': g[0]=0x00; g[1]=0x00; g[2]=0x5F; g[3]=0x00; g[4]=0x00; break;
        case '-': g[0]=0x08; g[1]=0x08; g[2]=0x08; g[3]=0x08; g[4]=0x08; break;
        case '_': g[0]=0x40; g[1]=0x40; g[2]=0x40; g[3]=0x40; g[4]=0x40; break;
        case ' ': break;
        default:
            g[0]=0x02; g[1]=0x01; g[2]=0x51; g[3]=0x09; g[4]=0x06;
            break;
    }
}

static esp_err_t send_command(ssd1306_t *dev, const uint8_t *cmd, size_t len)
{
    uint8_t packet[16];
    if (len + 1 > sizeof(packet)) {
        return ESP_ERR_INVALID_SIZE;
    }

    packet[0] = SSD1306_CTRL_CMD;
    memcpy(&packet[1], cmd, len);

    return i2c_master_transmit(dev->dev_handle, packet, len + 1, 1000);
}

esp_err_t ssd1306_init(ssd1306_t *dev,
                       gpio_num_t sda_gpio,
                       gpio_num_t scl_gpio,
                       uint8_t address)
{
    if (!dev) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(dev, 0, sizeof(*dev));
    dev->address = address;

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = sda_gpio,
        .scl_io_num = scl_gpio,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &dev->bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus: %s", esp_err_to_name(err));
        return err;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = 400000,
    };

    err = i2c_master_bus_add_device(dev->bus_handle, &dev_cfg, &dev->dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device: %s", esp_err_to_name(err));
        i2c_del_master_bus(dev->bus_handle);
        dev->bus_handle = NULL;
        return err;
    }

    const uint8_t init1[] = {0xAE, 0xD5, 0x80, 0xA8, 0x3F};
    const uint8_t init2[] = {0xD3, 0x00, 0x40, 0x8D, 0x14};
    const uint8_t init3[] = {0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12};
    const uint8_t init4[] = {0x81, 0x7F, 0xD9, 0xF1, 0xDB, 0x40};
    const uint8_t init5[] = {0xA4, 0xA6, 0x22, 0x00, 0x07, 0xAF};

    if ((err = send_command(dev, init1, sizeof(init1))) != ESP_OK) goto fail;
    if ((err = send_command(dev, init2, sizeof(init2))) != ESP_OK) goto fail;
    if ((err = send_command(dev, init3, sizeof(init3))) != ESP_OK) goto fail;
    if ((err = send_command(dev, init4, sizeof(init4))) != ESP_OK) goto fail;
    if ((err = send_command(dev, init5, sizeof(init5))) != ESP_OK) goto fail;

    ssd1306_clear(dev);
    return ssd1306_display(dev);

fail:
    ESP_LOGE(TAG, "SSD1306 init failed: %s", esp_err_to_name(err));
    ssd1306_deinit(dev);
    return err;
}

void ssd1306_clear(ssd1306_t *dev)
{
    if (dev) {
        memset(dev->buffer, 0, sizeof(dev->buffer));
    }
}

void ssd1306_draw_text(ssd1306_t *dev,
                       int x,
                       int y,
                       const char *text)
{
    if (!dev || !text || y < 0 || y > 56) {
        return;
    }

    int cursor_x = x;

    while (*text && cursor_x < SSD1306_WIDTH) {
        uint8_t glyph[5];
        get_glyph(*text, glyph);

        for (int col = 0; col < 5; col++) {
            int px = cursor_x + col;
            if (px < 0 || px >= SSD1306_WIDTH) {
                continue;
            }

            for (int row = 0; row < 7; row++) {
                if (glyph[col] & (1U << row)) {
                    int py = y + row;
                    if (py >= 0 && py < SSD1306_HEIGHT) {
                        dev->buffer[(py / 8) * SSD1306_WIDTH + px] |=
                            (1U << (py % 8));
                    }
                }
            }
        }

        cursor_x += 6;
        text++;
    }
}

esp_err_t ssd1306_display(ssd1306_t *dev)
{
    if (!dev || !dev->dev_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    const uint8_t set_page[] = {0x22, 0x00, 0x07};
    esp_err_t err = send_command(dev, set_page, sizeof(set_page));
    if (err != ESP_OK) {
        return err;
    }

    uint8_t packet[SSD1306_WIDTH + 1];
    packet[0] = SSD1306_CTRL_DATA;

    for (int page = 0; page < 8; page++) {
        memcpy(&packet[1],
               &dev->buffer[page * SSD1306_WIDTH],
               SSD1306_WIDTH);

        err = i2c_master_transmit(dev->dev_handle,
                                  packet,
                                  sizeof(packet),
                                  1000);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "display page %d: %s",
                     page, esp_err_to_name(err));
            return err;
        }
    }

    return ESP_OK;
}

esp_err_t ssd1306_deinit(ssd1306_t *dev)
{
    if (!dev) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;

    if (dev->dev_handle) {
        err = i2c_master_bus_rm_device(dev->dev_handle);
        dev->dev_handle = NULL;
    }

    if (dev->bus_handle) {
        esp_err_t err2 = i2c_del_master_bus(dev->bus_handle);
        if (err == ESP_OK) {
            err = err2;
        }
        dev->bus_handle = NULL;
    }

    return err;
}
