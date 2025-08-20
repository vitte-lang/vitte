# Notes de portage

- **Pins** : définis `LED` selon ta carte (ex: STM32F4 Discovery = GPIOD12, RP2040 Pico = 25, ESP32 DevKitC = 2).
- **Clocks** : adapte `sleep_ms` (busy-loop) si pas d'OS/RTOS. Idéalement utilise SysTick/Timers.
- **Lien avec Vitte VM** : insère ton loader/VM et fais pointer ses résolveurs FFI sur ces 3 symboles.
- **Linker/Startup** : fournis ici des stubs minimaux. Pour prod, utilise les fichiers vendor/CMSIS de ta famille MCU.
