#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define PIN_NUM_DIN   GPIO_NUM_10  // MOSI (DIN)
#define PIN_NUM_CLK   GPIO_NUM_8  // SCLK (CLK)

// See here https://github.com/espressif/arduino-esp32/blob/master/variants/XIAO_ESP32C3/pins_arduino.h
#define PIN_NUM_CS    GPIO_NUM_3 // D1   // Chip Select (CS)
#define PIN_NUM_RESET GPIO_NUM_2 // D0   // Reset (Reset)

#define CHARACTERS_COUNT 8

void gpio_init() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_NUM_DIN) | (1ULL << PIN_NUM_CLK) | (1ULL << PIN_NUM_CS) | (1ULL << PIN_NUM_RESET),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

void spi_write_data(uint8_t w_data) {
    for (uint8_t i = 0; i < 8; i++) {
        gpio_set_level(PIN_NUM_CLK, 0);

        // VFD SPI clock frequency is 0.5MHz, so we add delay if needed
        gpio_set_level(PIN_NUM_DIN, (w_data & 0x01));
        w_data >>= 1;

        gpio_set_level(PIN_NUM_CLK, 1);
    }
}

void VFD_cmd(uint8_t command) {
    gpio_set_level(PIN_NUM_CS, 0);
    spi_write_data(command);
    gpio_set_level(PIN_NUM_CS, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
}

void VFD_show(void) {
    gpio_set_level(PIN_NUM_CS, 0);
    spi_write_data(0xe8);  // Open Display Command
    gpio_set_level(PIN_NUM_CS, 1);
}

void VFD_init() {
    gpio_set_level(PIN_NUM_CS, 0);
    spi_write_data(0xe0);
    vTaskDelay(pdMS_TO_TICKS(1));
    spi_write_data(0x07);  // 8 digits
    gpio_set_level(PIN_NUM_CS, 1);
    vTaskDelay(pdMS_TO_TICKS(1));

    gpio_set_level(PIN_NUM_CS, 0);
    spi_write_data(0xe4);
    vTaskDelay(pdMS_TO_TICKS(1));
    spi_write_data(0xff);  // Max brightness
    gpio_set_level(PIN_NUM_CS, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
}

void VFD_WriteStr(uint8_t x, const char *str) {
    gpio_set_level(PIN_NUM_CS, 0);
    spi_write_data(0x20 + x);
    while (*str) {
        spi_write_data(*str);
        str++;
    }
    gpio_set_level(PIN_NUM_CS, 1);
    VFD_show();
}

void scrollText(const char *text) {
    int text_length = strlen(text);

    while (1) {
        for (int i = 0; i < text_length; i++) {
            char display_text[CHARACTERS_COUNT + 1];
            int j;

            for (j = 0; j < CHARACTERS_COUNT; j++) {
                display_text[j] = (i + j < text_length) ? text[i + j] : ' ';
            }
            display_text[j] = '\0';

            VFD_WriteStr(0, display_text);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

void app_main() {
    gpio_init();

    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_NUM_RESET, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(PIN_NUM_RESET, 1);

    VFD_init();
    scrollText("Hello world!");
}
