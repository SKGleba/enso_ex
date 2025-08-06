#ifndef __EXPORTS_H__
#define __EXPORTS_H__

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

#include "bm_ext.h"
#include "bootstrap.h"
#include "fmgr.h"
#include "main.h"
#include "nskbl.h"
#include "paper.h"
#include "stage2.h"
#include "stage3.h"
#include "stor.h"
#include "utils.h"
#include "view.h"

#define EXPORTS_VERSION 1
#define EXPORTS_MAGIC_1 0xE2E2E2E2
#define EXPORTS_MAGIC_2 0x2E2E2E2E

struct recovery_export_s {
    uint32_t magic[2];
    uint32_t version;
	struct eex_param_s *eex_param;
	ex_ports_struct *eex_ports;
	struct {
        void (*bmx_ctrl_read)(uint32_t *buttons);
        uint32_t (*bmx_ctrl_wait)(uint32_t exp_buttons, uint32_t poll_rate, uint32_t depress);
        void (*bmx_display_deinit)(void);
        int (*bmx_display_init)(enum display_type type, int view_count);
        uint32_t (*bmx_get_time)(int *since_reset);
    } bmx;
	struct {
        int (*fmgr_move_dir)(const char *src_path, const char *dest_path);
        int (*fmgr_move_file)(const char *src_path, const char *dest_path);
        int (*fmgr_delete)(const char *path);
        uint32_t (*fmgr_copy_file)(const char *src_path, const char *dest_path);
        int (*fmgr_copy_dir)(const char *src_path, const char *dest_path);
        void *(*fmgr_get_file)(const char *path, void *buf, int size, int offset);
        int (*fmgr_raw_dump)(uint32_t sector_start, uint32_t sector_count, const char *dest_dir);
        int (*fmgr_scan_masters)(char *output_s, int entry_len, uint32_t *output_i, int start, int max);
        int (*fmgr_list_dir)(const char *path, char *output, int entry_len, int start, int max);
        int (*fmgr_init)(void);
        int (*fmgr_view_handler)(enum VIEW_ASSIGNS *next_uview);
        void *(*fmgr_square_handler)(int set, enum FMGR_ENTRY_TYPES exp_entypes,
                                  void (*handler)(enum FMGR_ENTRY_TYPES entype, char *path, char *entry, enum VIEW_ASSIGNS *next_uview));
        int (*fmgr_load_exec)(const char *path);
    } fmgr;
	struct {
        int (*main)(int stage);
		struct paper_s *info_paper;
		struct paper_s *status_paper;
		struct paper_s *menu_paper;
        int (*init)(struct eex_param_s *eex_params);
        int (*deinit)(struct sysroot_buffer *sysroot);
    } main;
	struct {
		struct paper_s *default_paper;
        void (*paper_write)(struct paper_s *paper, const char *text, int count);
        void (*paper_print)(struct paper_s *paper, const char *text, int align, int count);
        void (*paper_printf)(struct paper_s *paper, const char *fmt, ...);
        void (*paper_draw_rectangle)(struct paper_s *paper, int x, int y, int width, int height, uint32_t color, int fill_pixels);
    } paper;
	struct {
        int (*stage2_apply_config)(void);
        int (*stage2_menu)(int selection);
        int (*stage3_menu)(int selection);
        struct menu_s *stage2_menu_s;
		struct menu_s *stage3_menu_s;
        struct stage2_options *stage2_opts;
    } stage;
	struct {
        int *g_disable_bootarea_update;
        int (*write_sector_sd)(int *part_ctx, uint32_t sector, const void *buffer, int nsectors);
        int (*write_sector_mmc)(int *ctx, unsigned int block_offset, const void *target_buf, int block_count);
        int (*sd_init)(int tries, int init_sd0_part);
        const char *(*get_partition_name)(int part);
        enum MOUNT_MASTER_TYPES (*stor_init_master)(enum MOUNT_MASTERS mount_master);
        int (*stor_init_mount)(int idx, enum MOUNT_MASTERS mount_master, enum STOR_PARTITIONS partition_id, enum STOR_PART_ACTIVES active);
        int (*stor_ff_init_mount)(int idx);
        int (*stor_read_mount)(int idx, uint32_t sector, void *buffer, int nsectors);
        int (*stor_write_mount)(int idx, uint32_t sector, const void *buffer, int nsectors);
        enum MOUNT_MASTER_TYPES (*stor_get_master_info)(enum MOUNT_MASTERS mount_master, uint32_t *partitions);
        int (*stor_umount)(int idx);
        partition_t *(*stor_find_partition_by_id)(master_block_t *master, int part_id, enum STOR_PART_ACTIVES active);
	} stor;
	struct {
		int *g_log_targets;
        void (*dbg_log)(int targets, const char *fmt, ...);
        void (*dbg_hexdump)(void *addr, int size, bool show_addr, char delim);
        struct rmemblock_s *rmemblock_allocs;
        void *(*rmemblock_alloc)(int size, uint32_t opt_type, uint32_t opt_paddr);
        int (*rmemblock_free)(void *va);
        int (*rmemblock_remap)(void *va, uint32_t type);
        char *(*my_strchr)(const char *s, int c);
        char *(*my_strrchr)(const char *s, int c);
        int (*count_chs)(const char *s, char c);
        char *(*find_nth)(const char *s, char c, int n);
        char *(*find_rnth)(const char *s, char c, int n);
    } utils;
	struct {
        int *view_current;
        uint32_t **view_vas;
        struct frame_s *view_frame;
        int (*view_init)(void);
        void (*view_switch)(enum VIEW_ASSIGNS new_view);
        int (*view_copy)(enum VIEW_ASSIGNS src_view, enum VIEW_ASSIGNS dst_view);
	} view;
	struct {
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
    } lbm;
};

extern const volatile struct recovery_export_s recovery_export;

#endif // __EXPORTS_H__