TARGET_BLOB	= rblob
BLOB_BASE = 0x51fc0000
TARGET_BOOTMGR = bootmgr
BOOTMGR_BASE = 0x51f00000
TARGET_LIB = libe2xr
SRCDIRS := src FatFs
INCDIRS := include FatFs lv0p
SRCFILES := $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
OBJS := $(SRCFILES:.c=.o)

PREFIX	= arm-vita-eabi
CC	= $(PREFIX)-gcc
AS	= $(PREFIX)-as
AR	= $(PREFIX)-ar
OBJCOPY	= $(PREFIX)-objcopy
CFLAGS	= $(foreach dir,$(INCDIRS),-I$(dir)) -mcpu=cortex-a9 -mthumb-interwork -mno-unaligned-access \
	  -O3 -g3 -Wall -Wno-unused-const-variable -ffreestanding -Wno-unused-variable -Wno-multichar -fdiagnostics-color=always
LDFLAGS	= -nostartfiles -nostdlib -lgcc -lm -lbaremetal

all: $(TARGET_BLOB).e2xp $(TARGET_BOOTMGR).e2xp $(TARGET_LIB).a
	@rm -f $(OBJS)

$(TARGET_LIB).a: $(OBJS)
	$(AR) rcs $@ $^

%.e2xp: %.elf
	$(OBJCOPY) -S -O binary $^ $@

$(TARGET_BLOB).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) -T linker.ld -Wl,--defsym,BASE_ADDR=$(BLOB_BASE)

$(TARGET_BOOTMGR).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) -T linker.ld -Wl,--defsym,BASE_ADDR=$(BOOTMGR_BASE)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean send

clean:
	@rm -f $(TARGET_BLOB).e2xp $(TARGET_BLOB).elf $(OBJS) $(TARGET_BOOTMGR).e2xp $(TARGET_BOOTMGR).elf
