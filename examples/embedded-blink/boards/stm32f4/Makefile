# Minimal Makefile — à adapter à ton MCU (F401/F407...). Nécessite arm-none-eabi-gcc.
CC=arm-none-eabi-gcc
AS=arm-none-eabi-gcc
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy
CFLAGS=-mcpu=cortex-m4 -mthumb -O2 -ffreestanding -nostdlib -Wall -Wextra
LDFLAGS=-T linker.ld -nostartfiles

all: blink.bin

blink.elf: start.o gpio.o
	$(CC) $(CFLAGS) -Wl,-Map=blink.map $(LDFLAGS) -o $@ $^

start.o: start.S
	$(AS) -c -mcpu=cortex-m4 -mthumb start.S -o $@

gpio.o: gpio.c ../../ffi/vitte_ffi.h
	$(CC) $(CFLAGS) -c gpio.c -o $@

blink.bin: blink.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o *.elf *.bin *.map
