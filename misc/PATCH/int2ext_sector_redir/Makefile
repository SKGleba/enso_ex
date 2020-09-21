PLUGIN_OBJS = kernel.o
HEADERS = $(wildcard *.h)

PLUGIN_LIBS = -lSceKblForKernel_365_stub

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -O3
ASFLAGS = $(CFLAGS)

all: e2xintext.skprx

e2xintext.skprx: e2xintext.velf
	vita-make-fself -c $< $@

e2xintext.velf: e2xintext.elf
	vita-elf-create -n -e exports.yml $< $@

e2xintext.elf: $(PLUGIN_OBJS)
	$(CC) $(CFLAGS) $^ $(PLUGIN_LIBS) -o $@ -nostdlib