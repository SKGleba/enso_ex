# enso_ex for developers
## Base kernel plugins
'Base kernel plugins' are kernel modules without imports located in `os0:ex/*`. They are loaded after the base kernel, but started before the base kernel - this allows the modules to function as 'plugins' and patch the base kernel in a pristine state.<br>
The custom kernel loader passes a [structure](https://github.com/SKGleba/enso_ex/blob/master/plugins/plugins.h#L90) as the second argument to plugin's module_start. It contains useful data, pointers, and functions.<br>
Since enso_ex v5.0 the max amount of custom kernel plugins is 32 (arbitrary) and there is no set naming convention.<br>
To add a custom base kernel plugin put it in `ux0:eex/custom/`, add it to the `ux0:eex/custom/boot_list.txt` and "Synchronize" via the enso_ex installer.<br>
### Examples
 - [e2xhencfg](plugins/hencfg) - a homebrew enabler, also redirects `os0:psp2config.skprx` to `ur0:tai/boot_config.txt`
 - [e2xculogo](plugins/culogo) - replaces the stock Playstation bootlogo with `os0:ex/bootlogo.raw`<br>
<br>


## Boot Manager
'Boot Manager' or 'bootmgr' is a raw code blob located at `os0:bootmgr.e2xp`. It is loaded and executed before the kernel loader, after enso_ex is done with exploit cleanup. It can be used as an enso_ex extension that alters core information or functionality such as Firmware version, ConsoleID, QA flags, security coprocessor behavior etc. If possible, base kernel plugins should be used instead.<br>
Boot Manager should be compiled as a position-independent code, there are no size limits. If bootmgr return value is &1, bootmgr will get unloaded and the memory freed.<br>
To add/change the Boot Manager put it in `ux0:eex/boot/bootmgr.e2xp` and "Synchronize" via the enso_ex installer.<br>
### Examples
 - [0syscall6](bootmgr/lv0-0syscall6) - a full psp2spl + 0syscall6 reimplementation for non-secure bootloader
 - [typespoof](bootmgr/lv0-typespoof) - psp2spl implementation with an added 'perfect' device type (dev/test/emu) spoofer <br>
<br>


## RAW recovery
Both EMMC and SD recovery are raw code blobs that, when recovery mode is triggered, are loaded and executed during stage 2 after enso_ex patches the non-secure kernel bootloader - this allows recovery to function as a enso_ex stage 2 extension.<br>
For more information, see the [recovery readme](README-recovery.md).<br>
### EMMC Recovery
 - EMMC Recovery is a single 0x200 bytes raw code blob loaded and executed from EMMC sector 4.
 - Arguments are a [get_hardware_config](https://github.com/SKGleba/enso_ex/blob/master/core/second.c#L342), [initialize_os0](https://github.com/SKGleba/enso_ex/blob/master/core/second.c#L392) and [load_executable](https://github.com/SKGleba/enso_ex/blob/master/core/second.c#L229) function pointers.
 - It should be compiled as a position-independent code, max size of 1 sector so 0x200 bytes.
 - After execution, the executable memory will be freed.
 - To add/change recovery put it in `ux0:eex/recovery/rconfig.e2xp` and "Synchronize" via the enso_ex installer.
 - It is intended to be used as a bootstrap for a bigger code blob located at sector 0x30+ that can be added/changed via the enso_ex installer's "Synchronize" option after placing the blob in `ux0:eex/recovery/rblob.e2xp`.<br>
### SD Recovery
 - SD recovery is a raw code blob loaded and executed from sd2vita based on [information](https://github.com/SKGleba/enso_ex/blob/master/core/ex_defs.h#L38) from sector 0.
 - Arguments are KBL param and key/ctrl data.
 - It should be compiled as a position-independent code. There is no size limit.<br>
### Examples
 - [EMMC - default](recovery/internal/default) - a simple bootstrap + blob EMMC recovery example
 - [SD - heartbeat](recovery/external/heartbeat) - overwrites the first sd2vita sector, lets you know that enso_ex is alive
 - [SD - enso_flash](recovery/external/enso_flash) - updates enso_ex from sd2vita <br>
<br>


## Bootarea read/write-lock
You can bypass MBR redirect by calling `sdif_read_mmc('GRB5', ..)`<br>
You can lock/unlock EMMC writes to the bootloaders, enso_ex and MBR by calling `sdif_write_mmc('CGB5', 'CGB5', NULL, new_lock_status)`<br>
<br>
