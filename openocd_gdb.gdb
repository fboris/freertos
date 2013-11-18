file main.elf
target remote :3333
b main.c:24
monitor reset halt
c
