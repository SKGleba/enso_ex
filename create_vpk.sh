(
cd enso/;
make FW=360;
make FW=365;
cd ../patches/;
make FW=360;
make FW=365;
cd ../installer/;
cmake ./ && make;
mv enso_installer.vpk ../enso_ex.vpk;
rm -rf CMakeFiles && rm cmake_install.cmake && rm CMakeCache.txt && rm Makefile;
rm -rf emmc_* && rm enso_* && rm kernel2* && rm version.h;
cd ../patches/;
rm *.e2xp;
cd ../enso/;
rm *.bin;
echo "";
echo "DONE! [ enso_ex.vpk ]";
echo "";
)