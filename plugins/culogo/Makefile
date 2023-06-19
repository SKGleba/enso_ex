PLUGIN_OBJS = kernel.o
HEADERS = $(wildcard *.h)

PLUGIN_LIBS = -lSceKblForKernel_365_stub

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -O3
ASFLAGS = $(CFLAGS)

all: e2xculogo.skprx

e2xculogo.skprx: e2xculogo.velf
	vita-make-fself -c $< $@

e2xculogo.velf: e2xculogo.elf
	vita-elf-create -n -e exports.yml $< $@

e2xculogo.elf: $(PLUGIN_OBJS)
	$(CC) $(CFLAGS) $^ $(PLUGIN_LIBS) -o $@ -nostdlib