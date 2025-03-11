#include "vfd.h"

#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void spi_write(uint8_t w_data) {
  for (uint8_t i = 0; i < 8; i++) {
    gpio_set_level(PIN_NUM_SCK, 0);
    gpio_set_level(PIN_NUM_MOSI, (w_data & 0x01));
    w_data >>= 1;
    gpio_set_level(PIN_NUM_SCK, 1);
  }
}

static void vfd_config(uint8_t key, uint8_t val) {
  gpio_set_level(PIN_NUM_CS, 0);
  spi_write(key);
  vTaskDelay(pdMS_TO_TICKS(1));
  spi_write(val);
  gpio_set_level(PIN_NUM_CS, 1);
  vTaskDelay(pdMS_TO_TICKS(1));
}

static void vfd_init_reset() {
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(PIN_NUM_RESET, 0);
  vTaskDelay(pdMS_TO_TICKS(1));
  gpio_set_level(PIN_NUM_RESET, 1);
}

void vfd_cmd(uint8_t cmd) {
  gpio_set_level(PIN_NUM_CS, 0);
  spi_write(cmd);
  gpio_set_level(PIN_NUM_CS, 1);
  vTaskDelay(pdMS_TO_TICKS(1));
}

void vfd_init() {
  // Configure GPIO pins
  gpio_config_t conf = {
      .pin_bit_mask = (1ULL << PIN_NUM_MOSI) | (1ULL << PIN_NUM_SCK) | (1ULL << PIN_NUM_CS) | (1ULL << PIN_NUM_RESET),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&conf);

  vfd_init_reset(); // Init VFD display by sending a RESET signal
  vfd_config(VFD_CONF_DISPLAY_SIZE, 0x07); // 8 digits
  vfd_config(VFD_CONF_BRIGHTNESS, 0xFF); // min 0x01 - max 0xFF
}

void vfd_write(uint8_t x, const char *str) {
  gpio_set_level(PIN_NUM_CS, 0);
  spi_write(VFD_DISPLAY_START_ADDR + x);
  while (*str) {
    spi_write(*str);
    str++;
  }
  gpio_set_level(PIN_NUM_CS, 1);

  vfd_cmd(VFD_CMD_PRINT);
}

void vfd_write_scroll(const char *str) {
  int str_len = strlen(str);

  while (1) {
    for (int i = 0; i < str_len; i++) {
      char display_text[DISPLAY_NUM_CHARS + 1];
      int j;

      for (j = 0; j < DISPLAY_NUM_CHARS; j++) {
        display_text[j] = (i + j < str_len) ? str[i + j] : ' ';
      }
      display_text[j] = '\0';

      vfd_write(0, display_text);
      vTaskDelay(pdMS_TO_TICKS(150));
    }
  }
}
