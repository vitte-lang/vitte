#pragma once
#ifdef __cplusplus
extern "C" {
#endif

void gpio_setup(int pin);
void gpio_write(int pin, int value);
void sleep_ms(int ms);

#ifdef __cplusplus
}
#endif
