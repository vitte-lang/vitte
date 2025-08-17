// Simulation POSIX : imprime l'état LED dans la console.
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include "../../ffi/vitte_ffi.h"

static int g_pin = 12;
static int g_state = 0;

void gpio_setup(int pin) {
    g_pin = pin;
    g_state = 0;
    fprintf(stdout, "[posix] gpio_setup(pin=%d)\n", pin);
    fflush(stdout);
}

void gpio_write(int pin, int value) {
    (void)pin;
    if (value != g_state) {
        g_state = value;
        fprintf(stdout, "LED[%d] = %s\n", g_pin, g_state ? "ON" : "OFF");
        fflush(stdout);
    }
}

void sleep_ms(int ms) {
    struct timespec ts; ts.tv_sec = ms/1000; ts.tv_nsec = (ms%1000)*1000000L;
    nanosleep(&ts, NULL);
}

int main(int argc, char** argv) {
    int period = 500;
    if (argc > 1) period = atoi(argv[1]);
    gpio_setup(12);
    for (;;) {
        gpio_write(12, 1);
        sleep_ms(period);
        gpio_write(12, 0);
        sleep_ms(period);
    }
    return 0;
}
