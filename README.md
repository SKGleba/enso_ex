# enso_ex
A mod of https://github.com/henkaku/enso for PSP2 Vita.

# Features
 - pre-nskernel recovery from a GC-SD device.
 - easy low-level code exec for custom kernel patches.
 - support for firmwares 3.60 and 3.65.
 
# Usage

## Building
 Just run "create_vpk.sh", it will build everything and copy the package to the root directory.

## Installing
 0) If you currently have enso, make sure that it is either https://github.com/henkaku/enso or https://github.com/TheOfficialFloW/enso.
 1) Install the VPK, run the "enso_ex" app and agree to the """ToS""".
	- If you get an error, reboot the device holding LTRIGGER and try again.
 2) Select "Install/reinstall the hack", press CROSS, the installer will install enso_ex.
	- All non-critical errors can be skipped by pressing CROSS, it is safe to do so.
 
## Patches
 - The "os0:patches.e2xd" file is a set of code blobs that are executed at low-level module load.
 - Custom patches can be skipped by holding VOLDOWN at boot.
 
### Adding patches
 - Patches are position independent code blobs that are run each in their own memblocks.
 - Patches are located in the "ux0:eex/payloads/" directory.
 - Data files for patches are located in the "ux0:eex/data/" directory.
 - You can "sync"/"compile" patches with the enso_ex app.
 
### Default patches
 - clogo.e2xp: Use a raw RGBA 960x544 image as the bootlogo, it is located @"os0:bootlogo.raw".
	- In safe/update mode the PS logo is used.
	- You can change it by replacing the file in "ux0:eex/data" and syncing.
	- If the file is not found there will be no logo, useful for boot animations.
 - rconfig.e2xp: If VOLUP is held at boot, "ux0:eex/boot_config.txt" is used instead of the ur0: one.
	- It also works in safe/update mode.
	
## BootMgr
 - The "os0:bootmgr.e2xp" file is a code blob that is executed just before psp2bootconfig load.
 - BootMgr can be skipped by holding VOLDOWN at boot.
 - To add/change it you need to add/change "ux0:eex/data/bootmgr.e2xp" and sync.

## Recovery
 - The "recovery" is a code blob loaded from a GC-SD device in GC slot.
 - Recovery can be loaded by holding SELECT at boot, the device must be connected to a power source.
 
### Supported recovery types
1) RAW recovery (recommended)
	- Use the tool in /sdrecovery/ to flash your recovery blob to the SD card.
	- If the SD card contains the EMMC image you can set a flag to use its os0 for low-level modules.
	- You can force it to use the FILE recovery found in GC-SD's os0.
2) FILE recovery
	- Format your SD card to FAT16 and put your recovery blob in "SD:recovery.e2xp".
	- By default the device will NOT continue the boot process after this recovery method is used.
	- TODO: add a cleanup sample to continue boot.
 
### Recovery errors
 - If the recovery returned 0, the console will continue the boot process.
 - If an error happened the user will need to confirm that he is aware of it by pressing the correct key:
	- "No recovery found" - press TRIANGLE.
	- "Error running GC-SD RAW recovery" - press CIRCLE.
	- "Error running GC-SD os0 recovery" - press SQUARE.
	- "Recovery did NOT return 0" - press CROSS.
	
### "dual nand"
 - You can use the SD's os0 partition instead of EMMC's os0 partition by holding START at boot.
 - If an error happened the user will need to confirm that he is aware of it by pressing the correct key:
	- "Error reading GC-SD" - press TRIANGLE.
	- "Incorrect SD magic (not SCE magic)" - press CIRCLE.

# Credits
 - Team molecule for henkaku, taihen, enso, and HenKaku wiki entries.
 - xerpi for his work on baremetal stuff.
 - CelesteBlue and PrincessOfSleeping for help with NSKBL RE.
 - Testers from the HenKaku discord server.