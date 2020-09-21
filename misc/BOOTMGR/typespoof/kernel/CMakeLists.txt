cmake_minimum_required(VERSION 2.8)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER "arm-vita-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-vita-eabi-g++")

project(HENkaku)

set(CMAKE_C_FLAGS "-Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(typespoof.elf
	main.c
)

target_link_libraries(typespoof.elf
	SceSblSmCommForKernel_stub
	SceSysclibForDriver_stub
	SceSysmemForDriver_stub
	SceSysrootForKernel_stub
	taihenForKernel_stub
	taihenModuleUtils_stub
	SceIofilemgrForDriver_stub
	SceKernelSuspendForDriver_stub
)

set_target_properties(typespoof.elf
	PROPERTIES LINK_FLAGS "-nostdlib"
	COMPILE_FLAGS "-D__VITA_KERNEL__"
)

add_custom_target(typespoof.skprx ALL
	COMMAND vita-elf-create -e ${CMAKE_SOURCE_DIR}/typespoof.yml typespoof.elf typespoof.velf
	COMMAND vita-make-fself -c typespoof.velf typespoof.skprx
)
add_dependencies(typespoof.skprx typespoof.elf)