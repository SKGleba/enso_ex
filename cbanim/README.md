# enso_ex-compatible boot animation creator.
Usage:
 "./cbanim -loop? filename%d -opt1"
 - loop: play the animation once or loop until the device boots [ -y | -n ]
 - filename%d: the base file name where the %d is instead of the frame number (e.g frame_%d.bin for frame_0.bin, frame_1.bin, ..)
 - opt1: no compression [ -nc ]
 - example: "./cbanim -y boot_splash.bin(%d)"
# Notes:
 - It is recommended to compress the animation (the non-compressed version is very slow)
 - The output file is called "boot_animation.img" and can be used directly with enso_ex (put it in ur0:tai/)
