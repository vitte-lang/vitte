// Exemple STM32F4 Discovery — LED verte GPIOD12.
// NOTE: Adapte ces adresses/bitfields à TON modèle précis (F401, F407, etc.).
#include <stdint.h>
#include "../../ffi/vitte_ffi.h"

#define PERIPH_BASE     0x40000000UL
#define AHB1PERIPH_BASE 0x40020000UL
#define RCC_BASE        (0x40023800UL)
#define GPIOD_BASE      (AHB1PERIPH_BASE + 0x0C00UL)

#define RCC_AHB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define GPIOD_MODER     (*(volatile uint32_t*)(GPIOD_BASE + 0x00))
#define GPIOD_BSRR      (*(volatile uint32_t*)(GPIOD_BASE + 0x18))

static int led_pin = 12;

void gpio_setup(int pin){
    (void)pin; led_pin = 12;
    /* enable clock GPIOD */
    RCC_AHB1ENR |= (1u << 3);
    /* MODER: pin 12 en sortie (01) */
    GPIOD_MODER &= ~(3u << (12*2));
    GPIOD_MODER |=  (1u << (12*2));
    /* état bas */
    GPIOD_BSRR = (1u << (12+16));
}

void gpio_write(int pin, int value){
    (void)pin;
    if (value) GPIOD_BSRR = (1u << 12); else GPIOD_BSRR = (1u << (12+16));
}

void sleep_ms(int ms){
    /* Boucle d'attente approximative — calibre selon ta fréquence */
    volatile uint32_t n = (uint32_t)ms * 16000u; // ~16MHz hypothétique
    while (n--) __asm__ volatile ("nop");
}

/* main minimal pour tester sans VM : blink 500ms */
int main(void){
    gpio_setup(12);
    for(;;){
        gpio_write(12, 1); sleep_ms(500);
        gpio_write(12, 0); sleep_ms(500);
    }
}
