(
# core
cd core/ &&
make &&
cd ../;
if [ $? -ne 0 ]; then echo "$0: create CORE failed!"; exit 0; fi

# default recovery
cd recovery/internal/default/blob/ &&
make &&
cd ../ &&
make &&
cd ../../../;
if [ $? -ne 0 ]; then echo "$0: create RECOVERY failed!"; exit 0; fi

# plugins & plugin loader
cd plugins/loader/ &&
make &&
cd ../hencfg/ &&
make &&
cd ../culogo/ &&
make &&
cd ../../;
if [ $? -ne 0 ]; then echo "$0: create PLUGINS failed!"; exit 0; fi

# prepare external installer resources
mkdir installer/res_ext &&
cp core/fat.bin installer/res_ext/fat.bin &&
echo "#define FATCHECK 0x$(crc32 core/fat.bin)" > installer/src/fatcheck.h &&
cp recovery/internal/default/rbootstrap.e2xp installer/res_ext/rbootstrap.e2xp &&
cp recovery/internal/default/blob/rblob.e2xp installer/res_ext/rblob.e2xp &&
cp plugins/loader/e2x_ckldr.skprx installer/res_ext/e2x_ckldr.skprx &&
cp plugins/loader/example_list.txt installer/res_ext/boot_list.txt &&
cp plugins/hencfg/e2xhencfg.skprx installer/res_ext/e2xhencfg.skprx &&
cp plugins/culogo/e2xculogo.skprx installer/res_ext/e2xculogo.skprx &&
cp plugins/culogo/example_logo.raw installer/res_ext/bootlogo.raw;
if [ $? -ne 0 ]; then echo "$0: prepare RESOURCES failed!"; exit 0; fi

# installer & package
cd installer/ &&
cmake ./ && make &&
cd ../;
if [ $? -ne 0 ]; then echo "$0: create INSTALLER failed!"; exit 0; fi

# copyout the package
cp installer/enso_installer.vpk enso_ex.vpk;
if [ $? -ne 0 ]; then echo "$0: copyout PACKAGE failed!"; exit 0; fi

# cleanup
cd installer/;
rm -rf CMakeFiles && rm cmake_install.cmake && rm CMakeCache.txt && rm Makefile;
rm -rf emmc_* && rm enso_* && rm kernel2* && rm version.h && rm libemmc*;
rm -rf res_ext;
cd ../plugins/hencfg/;
rm e2xhencfg.*;
rm kernel.o;
cd ../culogo/;
rm e2xculogo.*;
rm kernel.o;
cd ../loader/;
rm e2x_ckldr.*;
rm kernel.o;
cd ../../core/;
rm *.bin;
cd ../recovery/internal/default/;
rm *.elf && rm *.o && rm *.e2xp;
cd blob/;
rm *.elf && rm *.o && rm *.e2xp;

echo "";
echo "ALL DONE! [ enso_ex.vpk ]";
echo "";
)