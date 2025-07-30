TARGET	= rblob
SRCDIRS := src FatFs
INCDIRS := include FatFs
SRCFILES := $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
OBJS := $(SRCFILES:.c=.o)

PREFIX	= arm-vita-eabi
CC	= $(PREFIX)-gcc
AS	= $(PREFIX)-as
OBJCOPY	= $(PREFIX)-objcopy
CFLAGS	= $(foreach dir,$(INCDIRS),-I$(dir)) -mcpu=cortex-a9 -mthumb-interwork -mno-unaligned-access \
	  -O2 -g3 -Wall -Wno-unused-const-variable -ffreestanding -Wno-unused-variable
LDFLAGS	= -T linker.ld -nostartfiles -nostdlib -lgcc -lm -lbaremetal

all: $(TARGET).e2xp
	@rm -f $(OBJS)

%.e2xp: %.elf
	$(OBJCOPY) -S -O binary $^ $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean send

clean:
	@rm -f $(TARGET).e2xp $(TARGET).elf $(OBJS)
