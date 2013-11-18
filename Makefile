EXECUTABLE=main.elf
BIN_IMAGE=main.bin

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
ARCH=CM4F

LIB = ./lib
FREERTOS_SRC = $(LIB)/FreeRTOS
FREERTOS_INC = $(FREERTOS_SRC)/include/  
FREERTOS_PORT_INC = $(FREERTOS_SRC)/portable/GCC/ARM_$(ARCH)/

TOOLCHAIN_PATH ?= /usr/local/csl/arm-2012.03/arm-none-eabi
C_LIB= $(TOOLCHAIN_PATH)/lib/thumb2

CFLAGS=-g -O2 -mlittle-endian 
CFLAGS+=-mcpu=cortex-m4
CFLAGS+=-ffreestanding -nostdlib
CFLAGS+=-mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16
CFLAGS+=-fno-common -O0 \
        -ffreestanding\
        -gdwarf-2 -g3 \
        -DUSE_SEMIHOST\
        -Wall\
        -std=c99 -pedantic

# to run from FLASH
CFLAGS+=-Wl,-T,stm32_flash.ld

CFLAGS+=-I./

# stm32f4_discovery lib
CFLAGS+=-I$(LIB)/STM32F4xx_StdPeriph_Driver/inc
CFLAGS+=-I$(LIB)/STM32F4xx_StdPeriph_Driver/inc/device_support
CFLAGS+=-I$(LIB)/STM32F4xx_StdPeriph_Driver/inc/core_support


#STM32F4xx_StdPeriph_Driver\inc
CFLAGS+=-I$(LIB)/STM32F4xx_StdPeriph_Driver/inc

#Utilities
CFLAGS+=-I$(LIB)/Utilities/STM32F4-Discovery

#include RTOS
CFLAGS+=-I$(FREERTOS_INC)
CFLAGS+=-I$(FREERTOS_PORT_INC)

#Source Files
SRC += system_stm32f4xx.c startup_stm32f4xx.s string.c $(LIB)/Utilities/STM32F4-Discovery/stm32f4_discovery.c\
		$(FREERTOS_SRC)/croutine.c\
        $(FREERTOS_SRC)/list.c\
        $(FREERTOS_SRC)/queue.c\
		$(FREERTOS_SRC)/tasks.c\
		$(FREERTOS_SRC)/portable/MemMang/heap_1.c\
		$(FREERTOS_SRC)/portable/GCC/ARM_$(ARCH)/port.c\
		main.c util.c serial_io.c romfs.c hash-djb2.c filesystem.c fio.c \
        osdebug.c host.c linenoise.c shell.c
	

all: $(BIN_IMAGE)

libstm_build:
	$(MAKE) -C $(LIB)/STM32F4xx_StdPeriph_Driver/build

$(BIN_IMAGE): $(EXECUTABLE)
	$(OBJCOPY) -O binary $^ $@

$(EXECUTABLE): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@  -L$(LIB)/STM32F4xx_StdPeriph_Driver/build \
		-lSTM32F4xx_StdPeriph_Driver -L$(C_LIB)

clean:
	rm -rf $(EXECUTABLE)
	rm -rf $(BIN_IMAGE)

flash:
	st-flash write $(BIN_IMAGE) 0x8000000
openocd: flash 
	openocd -s /opt/openocd/share/openocd/scripts/
gdbauto:
	arm-none-eabi-gdb -x openocd_gdb.gdb
.PHONY: all clean openocd gdbauto
