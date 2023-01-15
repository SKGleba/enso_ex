PLUGIN_OBJS = kernel.o
HEADERS = $(wildcard *.h)

PLUGIN_LIBS = -lSceKblForKernel_365_stub

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -O3
ASFLAGS = $(CFLAGS)

all: e2xrecovr.skprx

e2xrecovr.skprx: e2xrecovr.velf
	vita-make-fself -c $< $@

e2xrecovr.velf: e2xrecovr.elf
	vita-elf-create -n -e exports.yml $< $@

e2xrecovr.elf: $(PLUGIN_OBJS)
	$(CC) $(CFLAGS) $^ $(PLUGIN_LIBS) -o $@ -nostdlib