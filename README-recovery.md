# enso_ex recovery
## Recovery key combinations
 - `select`: run sd2vita recovery
 - `start`: run emmc recovery
 - `start+circle`: disable emmc recovery, enable boot options toggle
   - `+triangle`: make the bootloaders and enso_ex read-only
   - `+dpad up`: use emuMBR from block 3 instead of block 1
   - `+dpad right`: skip os0 intialization
 - `volume down`: skip bootmgr and the custom kernel loader
 - `volume up`: skip custom kernel loader's custom plugins
 - `nothing on PSTV`: run sd2vita recovery
<br>
<br>

## SD2VITA recovery
GC-SD recovery mode is activated by holding SELECT at boot. In this mode enso_ex will read the first sector from sd2vita and depending on the contents follow their respective recovery routes.<br>
### RAW mode
 - If the first 0x10 bytes contain a specific *magic* value, then they are treated as a [recovery block structure](https://github.com/SKGleba/enso_ex/blob/master/core/ex_defs.h#L38) for enso_ex to load and execute a raw code blob<br>
### FAT16 mode
 - If the first sector is a fat16 partition PBR then enso_ex will mount it as `os0:` for the base kernel.<br>
### SCE MBR mode
 - If the first sector is a SCE MBR then enso_ex will mount `os0:` from sd2vita offset based on EMMC MBR.
 - It is mounted from sd2vita only for the base kernel.<br>
### Acknowledging errors
 - If no error occured, or the device is a PSTV, enso_ex will continue boot
 - If sd2vita could not be read, the user will need to press TRIANGLE to continue boot
 - If unknown data was found, the user will need to press CIRCLE to continue boot
 - If os0 mount or custom code failed, the user will need to press CROSS to continue boot <br>
### Examples
 - [heartbeat](recovery/external/heartbeat) - overwrites the first sd2vita sector with KBL param
 - [gui-usb-mount](recovery/external/gui-usb-mount) - menu that lets you USB-mount EMMC and its partitions
 - [enso_flash](recovery/external/enso_flash) - updates enso_ex from sd2vita
<br>
<br>

## EMMC recovery
EMMC recovery mode is activated by holding START at boot. In this mode enso_ex will load data from EMMC sector 4 and execute it as a raw code blob.<br>
 - It can be updated via the enso_ex installer's "Synchronize" option after placing the blob in `ux0:eex/recovery/rconfig.e2xp`.
 - It is intended to be used as a bootstrap for a bigger code blob located at sector 0x30+ that can be updated via the enso_ex installer's "Synchronize" option after placing the blob in `ux0:eex/recovery/rblob.e2xp`.<br>
### Examples
 - [default](recovery/internal/default) - loads a larger bootstrap that simply initializes `os0:`
<br>
<br>
