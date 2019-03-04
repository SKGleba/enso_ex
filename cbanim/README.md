# enso_ex-compatible boot animation creator.
Usage:
 "./cbanim -intype -loop? filename%d -opt1"
 - intype: input file type [ -r: raw | -g: gif | -p: png/jpg]. Note: there is also a resize parameter "s", it can be used with -g/-p (-sg/-sp), if set, cbanim will resize the input data to 960x544 (the file remains untouched).
 - loop: play the animation once or loop until the device boots [ -y | -n ]
 - filename%d: the base file name where the %d is instead of the frame number (e.g frame_%d.bin for frame_0.bin, frame_1.bin, ..)
 - opt1: no compression [ -nc ]
 - example: "./cbanim -g -n anim.gif"
# Notes:
 - It is recommended to compress the animation (the non-compressed version is very slow)
 - The output file is called "boot_animation.img" and can be used directly with enso_ex (put it in ur0:tai/)
