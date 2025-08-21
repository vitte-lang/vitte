# Embedded Blink — Vitte (ultra complet)

Démo **blink** multi-cibles pour relier des programmes **Vitte** au hardware via **FFI C**.
Tu écris en `.vitte`, on mappe à des fonctions C (`gpio_setup`, `gpio_write`, `sleep_ms`) que
chaque plateforme implémente à sa sauce (POSIX sim, STM32F4, RP2040, ESP32).

## Arborescence
```
embedded-blink/
├── main.vitte                # blink simple
├── examples/
│   ├── sos.vitte             # SOS en Morse
│   ├── pwm.vitte             # PWM soft (duty cycle)
├── ffi/
│   ├── vitte_ffi.h           # signatures FFI communes
├── boards/
│   ├── posix/                # simulation terminal
│   │   ├── runner.c
│   │   ├── Makefile
│   ├── stm32f4/              # GPIOD12 (LED verte sur Discovery)
│   │   ├── gpio.c            # wrappers FFI + toggles reg-level (exemple)
│   │   ├── linker.ld         # stub (à adapter à ton MCU précis)
│   │   ├── start.S           # minimal reset/vec (exemple)
│   │   ├── Makefile
│   ├── rp2040/
│   │   ├── CMakeLists.txt    # nécessite PICO_SDK_PATH
│   │   ├── vitte_ffi_rp2040.c
│   ├── esp32/
│   │   ├── CMakeLists.txt    # nécessite ESP-IDF
│   │   ├── main.c
├── scripts/
│   ├── build-posix.sh
│   ├── build-stm32f4.sh
│   ├── build-rp2040.sh
│   ├── build-esp32.sh
└── docs/
    ├── ports.md              # notes de portage (clocks, pins)
    └── ffi-contract.md       # convention d’ABI
```

## Convention FFI
- `void gpio_setup(int pin)` : configure la broche en sortie (niveau bas initial).
- `void gpio_write(int pin, int value)` : écrit 0/1.
- `void sleep_ms(int ms)` : délai bloquant en millisecondes.

> Les symboles **doivent** être exportés côté C avec ces noms exacts.

## Démarrage rapide
### POSIX (simulation)
```bash
cd embedded-blink/boards/posix
make
./blink 500    # période 500ms
```

### STM32F4 (exemple Discovery, GPIOD12)
- Installe `arm-none-eabi-gcc` + OpenOCD.
```bash
cd embedded-blink/boards/stm32f4
make
# flash via OpenOCD (adapter cfg nécessaire selon ta carte)
```

### RP2040 (Raspberry Pi Pico)
- Installe le Pico SDK et exporte `PICO_SDK_PATH`.
```bash
cd embedded-blink/boards/rp2040
cmake -S . -B build && cmake --build build
```

### ESP32 (ESP-IDF)
- Source `export.sh` de l’ESP-IDF, puis :
```bash
cd embedded-blink/boards/esp32
idf.py build flash monitor
```

## Intégrer avec le runtime Vitte
- L’exécution réelle d’un fichier `.vitte` sur cible suppose un **runtime Vitte** embarqué
  qui appelle ces FFI. Ici, on fournit **les wrappers** (coté C) + des programmes `.vitte`
  de référence. Branche ton VM/bytecode loader et call les FFI pour être full-stack.
