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
 - "os0:e2x_ckldr.skprx" will load custom modules after all base kernel modules are loaded (but not started)
 - It can be skipped by holding VOLDOWN at boot, custom modules load can be skipped by holding VOLUP.
 - To change it you need to change "ux0:eex/boot/e2x_ckldr.skprx" and sync.
 
### Adding patches
 - Patches are *.skprx kernel modules with no imports.
 - Patches are located in the "ux0:eex/custom/" directory ("os0:ex/" after sync).
 - Patches load list is located in "ux0:eex/custom/boot_list.txt" ("os0:ex/boot_list.txt" after sync).
 - A list entry should be 15 characters long and have a line break (0xA) as the 16th character.
 - Max custom module count in boot_list.txt is 15.
 
### Default patches
 - e2xculogo.skprx: Use a raw RGBA 960x544 image as the bootlogo, it is located @"os0:ex/bootlogo.raw".
	- In safe/update mode the PS logo is used.
	- You can change it by replacing the file in "ux0:eex/custom/" and syncing.
	- If the file is not found there will be no logo, useful for boot animations.
 - e2xhencfg.skprx:
	- Adds FSELF patches allowing homebrews at boot.
	- If not in safe/update mode, plaintext "ur0:tai/boot_config.txt" is used instead of "os0:psp2config.skprx".
	- If SQUARE is held at boot, "ux0:eex/boot_config.txt" is used instead of the ur0: one.
		- It works in safe/update mode.
	
## BootMgr
 - The "os0:bootmgr.e2xp" is a code blob that is executed just before psp2bootconfig load.
 - BootMgr can be skipped by holding VOLDOWN at boot.
 - To add/change it you need to add/change "ux0:eex/boot/bootmgr.e2xp" and sync.

## Recovery
 - The "recovery" is a code blob loaded from a GC-SD device in GC slot or the os0 partition.
 - Recovery can be loaded by holding SELECT at boot, the device must be connected to a power source.
 
### Supported recovery types
1) RAW recovery (recommended)
	- Use the tool in /sdrecovery/ to flash your recovery blob to the SD card.
	- If the SD card contains the EMMC image you can set a flag to use its os0 for low-level modules.
	- You can force it to use the FILE recovery found in GC-SD's os0.
2) FILE recovery
	- To add a os0 recovery you need to put "recovery.e2xp" in "ux0:eex/boot/" and sync.
	- To use the SD one you need to FAT16-format your SD card and put the recovery in "SD:recovery.e2xp".
	- By default the device will NOT continue the boot process after this recovery method is used.
	- TODO: add a cleanup sample to continue boot.
 
### Recovery errors
 - If the recovery returned 0, the console will continue the boot process.
 - If an error happened the user will need to confirm that he is aware of it by pressing the correct key:
	- "No recovery found" - press TRIANGLE.
	- "Error running recovery" - press CIRCLE.
	- "Recovery did NOT return 0" - press CROSS.
	
### "dual nand"
 - You can use the SD's os0 partition instead of EMMC's os0 partition by holding START at boot.
 - You can also format the SD card to FAT16, holding START will mount the SD card as os0.
 - If an error happened the user will need to confirm that he is aware of it by pressing the correct key:
	- "Error reading GC-SD" - press TRIANGLE.
	- "Incorrect SD magic (not SCE/FAT16)" - press CIRCLE.

# Credits
 - Team molecule for henkaku, taihen, enso, and HenKaku wiki entries.
 - xerpi for his work on baremetal stuff.
 - CelesteBlue and PrincessOfSleeping for help with NSKBL RE.
 - Testers from the HenKaku discord server.