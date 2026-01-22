PROJ    := game
TARGET  := $(PROJ)
OBJS    := source/main.o

# Use the tools provided by the docker container
PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
LD      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy

# GBA Specs
ARCH    := -mthumb-interwork -mthumb
SPECS   := -specs=gba.specs

# Compiler Flags
CFLAGS  := $(ARCH) -O2 -Wall -fno-strict-aliasing
LDFLAGS := $(ARCH) $(SPECS)

.PHONY: all clean

all: $(TARGET).gba

$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -v -O binary $< $@
	-@gbafix $@

$(TARGET).elf: $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).gba
