# enso_ex
extended enso installer for PSP2 on firmware 3.60 or 3.65

![ref0](https://github.com/SKGleba/enso_ex/raw/master/20190309_172545.jpg)

This is a mod of https://github.com/henkaku/enso, all credits go to team molecule for their awesome tool.

The following features/functions were added:
 - Compatibility with both fw3.60 and fw3.65.
 - Ability to keep the original Playstation Boot Logo or use a custom Boot Logo/Animation.
 - SafeBoot system with auto-config-rebuild feature.
 - sd2vita driver (optional).
 - Update blocker (optional).
 - Installer - ability to skip checks (force flash).
 
The following features/functions were removed:
 - Ability to download and update the firmware via enso installer.
 
# Functions usage
 - BootLogo:
   - location: ur0:tai/boot_splash.img
   - file format: raw (rgba) or gzipped raw (rgba) 960x544 image.
 - BootAnimation:
   - location: ur0:tai/boot_animation.img
   - file format: custom (see /cbanim/).
 - Util
   - sd2vita wakeup fix: self explanatory i think
   - sleep fd fix by TheFlow: fixes common sleep-related homebrew errors
   - delay boot: also self explanatory. Useful if you want to test your own boot_animation.