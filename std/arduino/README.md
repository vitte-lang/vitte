# Arduino Standard Library

The Arduino standard library provides interfaces for controlling Arduino boards and compatible microcontrollers.

## Modules

### Core Module (`arduino`)
Basic digital and analog I/O operations for Arduino boards.

**Key Types:**
- `PinMode` - Pin configuration (Input, Output, InputPullup)
- `PinState` - Digital pin state (Low, High)

**Key Functions:**
- `pin_mode(pin, mode)` - Configure pin mode
- `digital_write(pin, state)` - Write digital value
- `digital_read(pin)` - Read digital value
- `analog_write(pin, value)` - PWM output
- `analog_read(pin)` - Read analog value
- `delay_ms(ms)` - Millisecond delay
- `delay_us(us)` - Microsecond delay

### I2C Module (`arduino.i2c`)
Inter-Integrated Circuit (I2C) communication protocol support.

**Key Types:**
- `Address` - I2C device address

**Key Functions:**
- `begin(address)` - Initialize I2C
- `write(buffer)` - Write to device
- `read(length)` - Read from device
- `request_from(address, quantity)` - Request data from slave

### Sensors

#### BH1750 Light Sensor (`arduino.i2c.bh1750`)
ROHM BH1750 ambient light sensor over I2C.

**Default Addresses:**
- `BH1750_ADDR_LOW = 0x23` - When ADDR pin = GND
- `BH1750_ADDR_HIGH = 0x5C` - When ADDR pin = VCC

**Key Functions:**
- `init(sensor, address, mode)` - Initialize sensor
- `read_lux(sensor)` - Read light intensity
- `set_mode(sensor, mode)` - Change measurement mode
- `power_down(sensor)` - Power down
- `power_up(sensor)` - Power up

## Example Usage

```vitte
use std/arduino
use std/arduino.i2c.bh1750

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
