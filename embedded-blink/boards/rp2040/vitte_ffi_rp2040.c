#include "pico/stdlib.h"
#include "../../ffi/vitte_ffi.h"
static int led_pin = 25; // LED onboard sur Pico

void gpio_setup(int pin){
    led_pin = pin;
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_put(led_pin, 0);
}

void gpio_write(int pin, int value){
    (void)pin;
    gpio_put(led_pin, value ? 1 : 0);
}

void sleep_ms(int ms){
    sleep_ms(ms);
}

// runner basique
int main(){
    stdio_init_all();
    gpio_setup(25);
    while(1){
        gpio_write(25, 1); sleep_ms(500);
        gpio_write(25, 0); sleep_ms(500);
    }
    return 0;
}
