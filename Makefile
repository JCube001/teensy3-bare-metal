TARGET = blink

# Configurable options
OPTIONS = -DF_CPU=48000000 -D__MK20DX128__

# CPPFLAGS = compiler options for C and C++
CPPFLAGS = -Wall -g -Os -mcpu=cortex-m4 -mthumb -nostdlib -MMD $(OPTIONS) -I.

# Compiler options for C++ only
CXXFLAGS = -std=gnu++0x -felide-constructors -fno-exceptions -fno-rtti

# Compiler options for C only
CFLAGS =

# Linker options
LDFLAGS = -Os -Wl,--gc-sections -mcpu=cortex-m4 -mthumb -Tmk20dx128.ld

# Additional libraries to link
LIBS = -lm

# Names for the compiler programs
PREFIX = arm-none-eabi
CC = $(PREFIX)-gcc
CXX = $(PREFIX)-g++
OBJCOPY = $(PREFIX)-objcopy
SIZE = $(PREFIX)-size

# Automatically create lists of the sources and objects
C_FILES := $(wildcard *.c)
CPP_FILES := $(wildcard *.cpp)
OBJS := $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o)

#
# Rules
#

all: $(TARGET).hex

$(TARGET).elf: $(OBJS) mk20dx128.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.hex: %.elf
	$(SIZE) $<
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# Compiler generated dependency info
-include $(OBJS:.o=.d)

clean:
	rm -f *.o *.d $(TARGET).elf $(TARGET).hex

