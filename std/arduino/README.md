# std/arduino

Arduino facade for Vitte stdlib. This module re-exports the per-feature modules:

- `std/arduino/gpio`
- `std/arduino/serial`
- `std/arduino/i2c`
- `std/arduino/spi`
- `std/arduino/timer`

## Usage

```vitte
use std/arduino

entry main at core/app {
  pin_mode(13, PinMode.Output)
  loop {
    digital_write(13, PinState.High)
    delay_ms(500)
    digital_write(13, PinState.Low)
    delay_ms(500)
  }
  return 0
}
```

## Build (Arduino)

```sh
vitte build --target arduino-uno -o build/firmware.hex src/main.vit
```

## Upload (Arduino)

```sh
vitte build --target arduino-uno --upload --port /dev/ttyACM0 -o build/firmware.hex src/main.vit
```

If `--port` is not provided, the toolchain tries to auto-detect a port using
`arduino-cli board list`, filtered by the expected FQBN when possible. If an
FQBN is known and no matching board is found, upload fails. You can also set
`ARDUINO_PORT`.

## Targets

Targets are mapped in:

- `/Users/vincent/Documents/Github/vitte/target/arduino/boards.txt`

It includes a broad set of ESP32/ESP8266/STM32 presets and can be customized.

## I2C Drivers

- `std/arduino/i2c/bh1750` (light sensor)

## Examples

- `/Users/vincent/Documents/Github/vitte/examples/arduino_blink.vit`
- `/Users/vincent/Documents/Github/vitte/examples/arduino_serial_echo.vit`
- `/Users/vincent/Documents/Github/vitte/examples/arduino_i2c_scan.vit`
- `/Users/vincent/Documents/Github/vitte/examples/arduino_spi_transfer.vit`
- `/Users/vincent/Documents/Github/vitte/examples/arduino_i2c_spi_sensor.vit`
- `/Users/vincent/Documents/Github/vitte/examples/arduino_bh1750_lux.vit`
