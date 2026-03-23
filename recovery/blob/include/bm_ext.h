#ifndef __BM_EXT_H__
#define __BM_EXT_H__

#include <baremetal/cdram.h>
#include <baremetal/ctrl.h>
#include <baremetal/display.h>
#include <baremetal/draw.h>
#include <baremetal/dsi.h>
#include <baremetal/font.h>
#include <baremetal/gpio.h>
#include <baremetal/hdmi.h>
#include <baremetal/i2c.h>
#include <baremetal/iftu.h>
#include <baremetal/lcd.h>
#include <baremetal/libc.h>
#include <baremetal/mmc.h>
#include <baremetal/msif.h>
#include <baremetal/oled.h>
#include <baremetal/pervasive.h>
#include <baremetal/sdhci.h>
#include <baremetal/sdif.h>
#include <baremetal/spi.h>
#include <baremetal/syscon.h>
#include <baremetal/sysroot.h>
#include <baremetal/touch.h>
#include <baremetal/uart.h>
#include <baremetal/utils.h>

enum DISPLAYMGR_NSTATE {
	DISPLAYMGR_NSTATE_OFF = 0,
	DISPLAYMGR_NSTATE_ON,
	DISPLAYMGR_NSTATE_TOGGLE
};

enum DISPLAYMGR_OPT {
	DISPLAYMGR_OPT_INCLUDE_FB = 0,
	DISPLAYMGR_OPT_DISP_ONLY,
};

// workaround for libbaremetal's broken ctrl_read function
#define BMX_CTRL_BUTTON_HELD(ctrl, button) !((ctrl) & (button))
#define BMX_CTRL_BUTTON_PRESSED(ctrl, old, button) !(((ctrl) & ~(old)) & (button))

// -- GLOBALS --
#ifndef RXP_PIE
void bmx_ctrl_read(uint32_t *buttons);
uint32_t bmx_ctrl_wait(uint32_t exp_buttons, uint32_t poll_rate, uint32_t depress);
uint32_t bmx_get_time(int *since_reset);
int bmx_displaymgr(enum DISPLAYMGR_NSTATE enable, enum DISPLAYMGR_OPT opt);
#else
#define r_bmx_ctrl_read(...) _r->bmx->bmx_ctrl_read(__VA_ARGS__)
#define r_bmx_ctrl_wait(...) _r->bmx->bmx_ctrl_wait(__VA_ARGS__)
#define r_bmx_get_time(...) _r->bmx->bmx_get_time(__VA_ARGS__)
#define r_bmx_displaymgr(...) _r->bmx->bmx_displaymgr(__VA_ARGS__)
#endif

struct exports_bmx_s {
    void (*bmx_ctrl_read)(uint32_t *buttons);
    uint32_t (*bmx_ctrl_wait)(uint32_t exp_buttons, uint32_t poll_rate, uint32_t depress);
    uint32_t (*bmx_get_time)(int *since_reset);
    int (*bmx_displaymgr)(enum DISPLAYMGR_NSTATE enable, enum DISPLAYMGR_OPT opt);
};

struct exports_lbm_s {
    void (*cdram_enable)(void);
    void (*ctrl_read)(struct ctrl_data *data);
    void (*ctrl_set_analog_sampling)(int enable);
    void (*display_init)(enum display_type type);
    const struct display_config *(*display_get_current_config)(void);
    void (*draw_rectangle)(int x, int y, int w, int h, uint32_t color);
    void (*dsi_init)(void);
    int (*dsi_get_dimensions_for_vic)(uint32_t vic, uint32_t *width, uint32_t *height);
    int (*dsi_get_pixelclock_for_vic)(uint32_t vic, uint32_t bpp, uint32_t *pixelclock);
    void (*dsi_start_master)(enum dsi_bus bus, uint32_t vic);
    void (*dsi_stop_master)(enum dsi_bus bus);
    void (*dsi_start_display)(enum dsi_bus bus, uint32_t vic, uint32_t unk);
    void (*font_draw_char)(int x, int y, uint32_t color, char c);
    void (*font_draw_string)(int x, int y, uint32_t color, const char *s);
    void (*font_draw_stringf)(int x, int y, uint32_t color, const char *s, ...);
    void (*gpio_set_port_mode)(int bus, int port, int mode);
    int (*gpio_port_read)(int bus, int port);
    void (*gpio_port_set)(int bus, int port);
    void (*gpio_port_clear)(int bus, int port);
    void (*gpio_set_intr_mode)(int bus, int port, int mode);
    int (*gpio_query_intr)(int bus, int port);
    int (*gpio_acquire_intr)(int bus, int port);
    int (*hdmi_init)(void);
    int (*hdmi_get_hpd_state)(void);
    int (*hdmi_connect)(void);
    void (*i2c_init_bus)(int bus);
    void (*i2c_transfer_write)(int bus, uint8_t addr, const uint8_t *buffer, int size);
    void (*i2c_transfer_read)(int bus, uint8_t addr, uint8_t *buffer, int size);
    void (*i2c_transfer_write_read)(int bus, uint8_t write_addr, const uint8_t *write_buffer, int write_size, uint8_t read_addr, uint8_t *read_buffer,
                                    int read_size);
    void (*iftu_bus_enable)(enum iftu_bus bus);
    void (*iftu_bus_plane_config_select)(enum iftu_bus bus, enum iftu_plane plane, enum iftu_plane_config config);
    void (*iftu_bus_alpha_blending_control)(enum iftu_bus bus, int ctrl);
    void (*iftu_plane_set_alpha)(enum iftu_bus bus, enum iftu_plane plane, uint32_t alpha);
    void (*iftu_plane_set_csc_enabled)(enum iftu_bus bus, enum iftu_plane plane, bool enabled);
    void (*iftu_plane_set_csc0)(enum iftu_bus bus, enum iftu_plane plane, const struct iftu_csc_params *csc);
    void (*iftu_plane_set_csc1)(enum iftu_bus bus, enum iftu_plane plane, const struct iftu_csc_params *csc);
    void (*iftu_plane_config_set_config)(enum iftu_bus bus, enum iftu_plane plane, enum iftu_plane_config config, const struct iftu_plane_fb_config *fb,
                                         uint32_t dst_x, uint32_t dst_y, uint32_t dst_w, uint32_t dst_h);
    void (*iftu_plane_config_set_enabled)(enum iftu_bus bus, enum iftu_plane plane, enum iftu_plane_config config, bool enabled);
    int (*lcd_init)(void);
    void *(*memset)(void *s, int c, size_t n);
    void *(*memcpy)(void *dest, const void *src, size_t n);
    int (*memcmp)(const void *s1, const void *s2, size_t n);
    size_t (*strlen)(const char *str);
    void (*exit)(int status);
    void (*msif_init)(void);
    void (*msif_setup)(const uint8_t key[32]);
    void (*msif_get_info)(struct msif_info *info);
    void (*msif_read_sector)(uint32_t sector, void *buff);
    void (*msif_read_atrb)(uint32_t address, void *buff);
    void (*msif_read_short_data)(uint8_t cmd, void *buff, uint32_t size);
    void (*msif_write_short_data)(uint8_t cmd, const void *buff, uint32_t size);
    int (*oled_init)(void);
    uint32_t (*pervasive_get_soc_revision)(void);
    void (*pervasive_clock_enable_uart)(int bus);
    void (*pervasive_reset_exit_uart)(int bus);
    void (*pervasive_clock_enable_gpio)(void);
    void (*pervasive_reset_exit_gpio)(void);
    void (*pervasive_clock_enable_i2c)(int bus);
    void (*pervasive_reset_exit_i2c)(int bus);
    void (*pervasive_clock_enable_spi)(int bus);
    void (*pervasive_clock_disable_spi)(int bus);
    void (*pervasive_reset_exit_spi)(int bus);
    void (*pervasive_clock_enable_dsi)(int bus, int value);
    void (*pervasive_reset_exit_dsi)(int bus, int value);
    void (*pervasive_clock_enable_msif)(void);
    void (*pervasive_clock_disable_msif)(void);
    void (*pervasive_reset_exit_msif)(void);
    void (*pervasive_reset_enter_msif)(void);
    void (*pervasive_clock_enable_sdif)(int bus);
    void (*pervasive_clock_disable_sdif)(int bus);
    void (*pervasive_reset_exit_sdif)(int bus);
    void (*pervasive_reset_enter_sdif)(int bus);
    void (*pervasive_dsi_set_pixelclock)(int bus, int pixelclock);
    void (*pervasive_dsi_misc_unk_enable)(int bus);
    void (*pervasive_dsi_misc_unk_disable)(int bus);
    void (*pervasive_hdmi_clock_set_enabled)(int enable);
    int (*pervasive_msif_get_card_insert_state)(void);
    uint32_t (*pervasive_msif_unk)(void);
    void (*pervasive_msif_set_clock)(uint32_t clock);
    void (*pervasive_sdif_misc_0x110_0x11C)(int bus, uint32_t value);
    void (*pervasive_sdif_misc_0x124)(int bus, uint32_t value);
    void (*pervasive_sdif_misc_0x310)(int bus, uint32_t value);
    int (*sdif_init)(enum sdif_host host);
    bool (*sdif_is_card_inserted)(enum sdif_host host);
    int (*spi_init)(int bus);
    void (*spi_write_start)(int bus);
    void (*spi_write_end)(int bus);
    void (*spi_write)(int bus, uint32_t data);
    int (*spi_read_available)(int bus);
    int (*spi_read)(int bus);
    void (*spi_read_end)(int bus);
    int (*syscon_init)(void);
    void (*syscon_transfer)(const uint8_t *tx, int tx_size, uint8_t *rx, int max_rx_size);
    void (*syscon_command_read)(uint16_t cmd, void *buffer, int max_length);
    void (*syscon_short_command_write)(uint16_t cmd, uint32_t data, int length);
    int (*syscon_scratchpad_read)(uint16_t offset, void *buffer, int size);
    int (*syscon_scratchpad_write)(uint16_t offset, const void *buffer, int size);
    int (*syscon_get_baryon_version)(void);
    int (*syscon_get_hardware_info)(void);
    void (*syscon_reset_device)(int type, int mode);
    void (*syscon_set_hdmi_cdc_hpd)(int enable);
    void (*syscon_msif_set_power)(int enable);
    void (*syscon_ctrl_device_reset)(uint32_t param_1, uint32_t param_2);
    void (*syscon_get_touchpanel_device_info)(struct syscon_touchpanel_device_info *info);
    void (*syscon_get_touchpanel_device_info_ext)(struct syscon_touchpanel_device_info_ext *info);
    void (*syscon_get_touchpanel_unk_info_front)(uint16_t *data);
    void (*syscon_get_touchpanel_unk_info_back)(uint16_t *data);
    void (*syscon_touch_set_sampling_cycle)(int cycles_front, int cycles_back);
    void (*sysroot_init)(const struct sysroot_buffer *sysroot_buffer);
    uint32_t (*sysroot_get_hw_info)(void);
    int (*sysroot_model_is_vita)(void);
    int (*sysroot_model_is_dolce)(void);
    int (*sysroot_model_is_vita2k)(void);
    int (*sysroot_model_is_diag)(void);
    int (*sysroot_is_au_codec_ic_conexant)(void);
    void (*touch_init)(void);
    void (*touch_configure)(int port_mask, uint8_t max_report_front, uint8_t max_report_back);
    void (*touch_set_sampling_cycle)(int port_mask, uint8_t cycles_front, uint8_t cycles_back);
    void (*touch_read)(int port_mask, struct touch_data *data);
    int (*uart_init)(int bus, uint32_t baudrate);
    void (*uart_wait_ready)(int bus);
    void (*uart_write)(int bus, uint32_t data);
    uint32_t (*uart_read_fifo_data_available)(int bus);
    uint32_t (*uart_read)(int bus);
    void (*uart_putc)(int bus, char c);
    void (*uart_print)(int bus, const char *str);
    void (*uart_puts)(int bus, const char *str);
    void (*uart_printf)(int bus, const char *s, ...);
    void (*delay)(uint32_t n);
    uint32_t (*get_cpu_id)(void);
};

#endif