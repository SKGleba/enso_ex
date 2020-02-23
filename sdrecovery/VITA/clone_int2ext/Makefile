PLUGIN_OBJS = kernel.o
HEADERS = $(wildcard *.h)

PLUGIN_LIBS = -lSceSysmemForDriver_stub -lSceSdifForDriver_stub

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -O3
ASFLAGS = $(CFLAGS)

all: emmclone.skprx

emmclone.skprx: emmclone.velf
	vita-make-fself -c $< $@

emmclone.velf: emmclone.elf
	vita-elf-create -e exports.yml $< $@

emmclone.elf: $(PLUGIN_OBJS)
	$(CC) $(CFLAGS) $^ $(PLUGIN_LIBS) -o $@ -nostdlib
