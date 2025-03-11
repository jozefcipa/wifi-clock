#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "vfd.h"

void app_main() {
    vfd_init();

    // Test the display by turning all pixels on first
    vfd_cmd(VFD_CMD_ALL_PIXELS_ON);
    vTaskDelay(pdMS_TO_TICKS(1000));


    vfd_write_scroll("Hello world, how are you?");

    // Init WiFi
    // - set provisioning state if needed and print on display

    // Wait for WiFi

    // Fetch current time

    // create task to show time every second
}
