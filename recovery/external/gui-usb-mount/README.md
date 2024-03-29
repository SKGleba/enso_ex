# PSP2 recovery for enso_ex v5+; setup guide.
1) Create a FAT16 partition on the sd card, make sure it has 'FAT16' in the boot sector @ 0x34.
 - You can flash a clean vs0 / os0 image from PUP to ensure compatibility.
2) Copy 3.65 os0 files to the root of the sd card.
 - It does not matter what type/image.
3) Copy the following files to the root of the sd card:
 - enso_ex's custom kernel loader (in vpk) as `e2x_ckldr.skprx`
 - [psp2config](files/psp2config.skprx) as `psp2config_vita.skprx`
 - [recoVery menu](https://github.com/SKGleba/VitaTools/tree/main/recoVery) as `recovery.skprx`
 - [vlog](https://github.com/SKGleba/VitaTools/tree/main/vlog) as `vlog.skprx`
4) Copy the following files to the `ex` directory:
 - [`boot_list.txt`](files/boot_list.txt)
 - [e2xrecovr patch](https://github.com/SKGleba/VitaTools/tree/main/recoVery) as `e2xrecovr.skprx`
 - (optional) [`rcvr_logo.raw`](files/rcvr_logo.raw)
<br>

### Booting the PS Vita while holding SELECT will load recoVery from the sd2vita.

## Custom command (cmd.recovery) struct (bytes):
 - 'F': Flash 'toflash.img' to an emmc partition
  - '0' or '1' - active partition flag
   - partition code master - '1' if partition code > 9
    - partition code slave
 - 'D': Dump an emmc partition to 'dumped.img'
  - '0' or '1' - active partition flag
   - partition code master - '1' if partition code > 9
    - partition code slave
 - 'M': Mount/Unmount a mountpoint
  - 'U' if should unmount, otherwise '0'
   - mountpoint id master - '1' if mountpoint id > 9
    - mountpoint id slave
<br>
	
### partition codes: {
		"empty",
		"first_partition",
		"slb2",
		"os0",
		"vs0",
		"vd0",
		"tm0",
		"ur0",
		"ux0",
		"gro0",
		"grw0",
		"ud0",
		"sa0",
		"some_data",
		"pd0",
		"invalid"
	};