#include <stdio.h>

// For more information about configuring a VFD display
// See the official code example and datasheet from the manufacturer
// https://drive.google.com/drive/folders/1ZXyel2LAcHdFSkiIZ5ASejlG-4toeGsX?usp=sharing

#define PIN_NUM_MOSI GPIO_NUM_10
#define PIN_NUM_SCK GPIO_NUM_8
// XIAO_ESP32C uses different pin numbers for GPIO pins
// See here https://github.com/espressif/arduino-esp32/blob/master/variants/XIAO_ESP32C3/pins_arduino.h
#define PIN_NUM_CS GPIO_NUM_3     // D1
#define PIN_NUM_RESET GPIO_NUM_2  // D0

#define DISPLAY_NUM_CHARS 8

#define VFD_CONF_BRIGHTNESS 0xe4
#define VFD_CONF_DISPLAY_SIZE 0xe0

#define VFD_DISPLAY_START_ADDR 0x20

#define VFD_CMD_ALL_PIXELS_ON 0xe9
#define VFD_CMD_PRINT 0xe8

void vfd_init();
void vfd_write(uint8_t x, const char *str);
void vfd_write_scroll(const char *str);
void vfd_cmd(uint8_t cmd);
