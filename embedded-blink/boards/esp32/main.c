#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../ffi/vitte_ffi.h"

static gpio_num_t led_pin = GPIO_NUM_2; // LED onboard (selon carte)

void gpio_setup(int pin){
    led_pin = (gpio_num_t)pin;
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<led_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(led_pin, 0);
}

void gpio_write(int pin, int value){
    (void)pin;
    gpio_set_level(led_pin, value ? 1 : 0);
}

void sleep_ms(int ms){
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

void app_main(void){
    gpio_setup((int)led_pin);
    while(1){
        gpio_write((int)led_pin, 1); sleep_ms(500);
        gpio_write((int)led_pin, 0); sleep_ms(500);
    }
}
