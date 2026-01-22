PROJ    := game
TARGET  := $(PROJ)
OBJS    := source/main.o

# Use the devkitARM toolchain
PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
LD      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy

# Architecture flags for GBA
ARCH    := -mthumb-interwork -mthumb
SPECS   := -specs=gba.specs

# Compiler flags
CFLAGS  := $(ARCH) -O2 -Wall -fno-strict-aliasing -I$(DEVKITPRO)/libgba/include
# Linker flags (Link libgba and math library)
LDFLAGS := $(ARCH) $(SPECS) -L$(DEVKITPRO)/libgba/lib -lgba -lm

.PHONY: all clean

all: $(TARGET).gba

$(TARGET).elf: $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -o $@

$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	gbafix $@

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).gba
