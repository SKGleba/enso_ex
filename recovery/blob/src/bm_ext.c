// Extensions to libbaremetal for nskbl-level functionality
#include <baremetal/display.h>
#include <baremetal/gpio.h>
#include <baremetal/pervasive.h>
#include <baremetal/syscon.h>
#include <baremetal/sysroot.h>
#include <baremetal/utils.h>
#include "utils.h"
#include "bm_ext.h"

static int bmx_fb_uid = -1;

static inline void pervasive_mask_or_my(uint32_t addr, uint32_t val) {
    volatile unsigned long tmp;

    asm volatile(
        "ldr %0, [%1]\n\t"
        "orr %0, %2\n\t"
        "str %0, [%1]\n\t"
        "dmb\n\t"
        "ldr %0, [%1]\n\t"
        "dsb\n\t"
        : "=&r"(tmp)
        : "r"(addr), "r"(val));
}

static inline void pervasive_mask_and_not_my(uint32_t addr, uint32_t val) {
    volatile unsigned long tmp;

    asm volatile(
        "ldr %0, [%1]\n\t"
        "bic %0, %2\n\t"
        "str %0, [%1]\n\t"
        "dmb\n\t"
        "ldr %0, [%1]\n\t"
        "dsb\n\t"
        : "=&r"(tmp)
        : "r"(addr), "r"(val));
}

void bmx_display_deinit(void) { // required for OS to be able to reset the display
    if (!sysroot_model_is_dolce()) {
        gpio_port_clear(0, GPIO_PORT_OLED_LCD);
        pervasive_mask_or_my(0xE3101000 + 0x80, 7);
        pervasive_mask_and_not_my(0xE3102000 + 0x80, 0xF);
    }
    if (bmx_fb_uid >= 0) {
        if (sceKernelFreeMemBlock(bmx_fb_uid) < 0)
            LOG("Failed to free framebuffer memblock\n");
        else
            LOG("Framebuffer memblock freed\n");
        bmx_fb_uid = -1;
    }
}

int bmx_display_init(enum display_type type, int view_count) {
    display_init(type);

    struct display_config *ds_config = display_get_current_config();
    SceKernelAllocMemBlockKernelOpt opt = {.size = 0x58, .attr = 2, .paddr = 0x20000000};
    bmx_fb_uid = sceKernelAllocMemBlock("e2x_recovr_vram", 0x60208006, (ds_config->pitch * ds_config->height * 4) * (1 + view_count), &opt);
    if (bmx_fb_uid < 0) {
        LOG("Failed to alloc framebuffer memblock: 0x%08X\n", bmx_fb_uid);
        return -1;
    }

    void *fb_base = NULL;
    if (sceKernelGetMemBlockBase(bmx_fb_uid, &fb_base) < 0) {
        LOG("Failed to get framebuffer memblock base\n");
        sceKernelFreeMemBlock(bmx_fb_uid);
        return -1;
    }

    ds_config->addr = (uint32_t)fb_base;
    LOG("Framebuffer allocated at %08X\n", (uint32_t)fb_base);
    return 0;
}

void bmx_ctrl_read(uint32_t *buttons) {
    uint8_t buffer[SYSCON_RX_HEADER_SIZE + 10];
    syscon_command_read(0x101, buffer, sizeof(buffer));
    *buttons = (buffer[4] | (buffer[5] << 8) |
                (buffer[6] << 16) | (buffer[7] << 24));
}

uint32_t bmx_ctrl_wait(uint32_t exp_buttons, uint32_t poll_rate, uint32_t depress) {
    uint32_t current_buttons = -1;
    do {
        bmx_ctrl_read(&current_buttons);
        delay(poll_rate);
    } while (!(CTRL_BUTTON_HELD(~current_buttons, exp_buttons)));
    if (depress) {
        depress = current_buttons;
        do {
            bmx_ctrl_read(&depress);
            delay(poll_rate);
        } while (CTRL_BUTTON_HELD(~depress, exp_buttons));
    }
    return current_buttons;
}

#define SC_RTC_SHIFT 19
#define SC_RTC_OFFSET 62135596800 // 1970-01-01T00:00:00Z in seconds
static uint64_t syscon_reset_tick = 0;
uint32_t bmx_get_time(int *since_reset) {
    uint64_t sec;
    if (!syscon_reset_tick) {
        if (syscon_scratchpad_read(0x10, &syscon_reset_tick, 8) < 0) {
            LOG("Failed to read syscon reset tick\n");
            return 0;
        }
        syscon_reset_tick *= (uint64_t)(1 << SC_RTC_SHIFT);
        syscon_reset_tick /= 1000000; // convert to seconds
        syscon_reset_tick -= SC_RTC_OFFSET; // adjust to epoch
        LOG("Got syscon reset tick: %08X%08X\n",
            (uint32_t)(syscon_reset_tick >> 32), (uint32_t)syscon_reset_tick);
    }
    uint8_t syscon_on_hsec_cmd_rx[SYSCON_RX_HEADER_SIZE + sizeof(uint32_t) + 1];
    syscon_command_read(0x11, syscon_on_hsec_cmd_rx, sizeof(syscon_on_hsec_cmd_rx));
    sec = (uint64_t)*(uint32_t *)(syscon_on_hsec_cmd_rx + SYSCON_RX_DATA);
    sec >>= 1; // 1/2 second resolution
    if (since_reset)
        *since_reset = (int)sec;
    sec += syscon_reset_tick; // add reset tick offset
    return (uint32_t)sec;
}