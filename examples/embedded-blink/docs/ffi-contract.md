# Contrat FFI (Vitte ↔ C)

Trois symboles C exportés côté cible :
```c
void gpio_setup(int pin);
void gpio_write(int pin, int value);
void sleep_ms(int ms);
```
Le runtime Vitte les appelle quand un programme `.vitte` utilise `extern fn` correspondants
(ou une couche wrapper qui renomme vers ces symboles).

**ABI** : C, 32-bit safe. Sur MCU 32-bit ARM/RISC-V, passe par registres/stack standard.
