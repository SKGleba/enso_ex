PLUGIN_OBJS = kernel.o
HEADERS = $(wildcard *.h)

PLUGIN_LIBS = -lSceKblForKernel_365_stub

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -O3
ASFLAGS = $(CFLAGS)

all: e2xhencfg.skprx

e2xhencfg.skprx: e2xhencfg.velf
	vita-make-fself -c $< $@

e2xhencfg.velf: e2xhencfg.elf
	vita-elf-create -n -e exports.yml $< $@

e2xhencfg.elf: $(PLUGIN_OBJS)
	$(CC) $(CFLAGS) $^ $(PLUGIN_LIBS) -o $@ -nostdlib