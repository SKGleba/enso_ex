TARGET	= rblob
SRCDIR	= src
INCDIR	= include
OBJS	= main.o bm_ext.o paper.o stage2.o stage3.o

PREFIX	= arm-vita-eabi
CC	= $(PREFIX)-gcc
AS	= $(PREFIX)-as
OBJCOPY	= $(PREFIX)-objcopy
CFLAGS	= -I$(INCDIR) -mcpu=cortex-a9 -mthumb-interwork -mno-unaligned-access \
	  -O2 -g3 -Wall -Wno-unused-const-variable -ffreestanding -Wno-unused-variable
LDFLAGS	= -T linker.ld -nostartfiles -nostdlib -lgcc -lm -lbaremetal
ASFLAGS	=
DEPS	= $(OBJS:.o=.d)

all: $(TARGET).e2xp

%.e2xp: %.elf
	$(OBJCOPY) -S -O binary $^ $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

%.o: $(SRCDIR)/%.s
	$(CC) $(ASFLAGS) -MMD -MP -c $< -o $@

.PHONY: clean send

clean:
	@rm -f $(TARGET).e2xp $(TARGET).elf $(OBJS) $(DEPS)

-include $(DEPS)
