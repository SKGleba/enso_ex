cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

set(SHORT_NAME enso_installer)
project(${SHORT_NAME})
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "ensō_ex")
set(VITA_TITLEID  "MLCL00003")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -fno-builtin-printf")
if (DEFINED SAFETY)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
endif()
set(IGNORE_WARNING "${SAFETY}")

string(TIMESTAMP BUILD_DATE UTC)

set(CID "00000000000000000000000000000000" CACHE STRING "Customized CID")
set(PERSONALIZED 0 CACHE BOOL "Personalize build")

string(LENGTH ${CID} CID_LEN)

if(NOT ${CID_LEN} EQUAL 32)
  message(FATAL_ERROR "Invalid CID")
endif()

set(i 0)
while(i LESS ${CID_LEN})
  string(SUBSTRING ${CID} ${i} 2 ch)
  if(${i} EQUAL 0)
    set(BUILD_CID "0x${ch} ^ 0xAA")
  else()
    set(BUILD_CID "${BUILD_CID}, 0x${ch} ^ 0xAA")
  endif()
  math(EXPR i "${i} + 2")
endwhile()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
include_directories(${PROJECT_BINARY_DIR})

include_directories(
  src
)

link_directories(
  ${CMAKE_BINARY_DIR}/emmc_helper_stubs
  ${CMAKE_BINARY_DIR}/emmc_helper_user_stubs
  ${CMAKE_BINARY_DIR}/365_stubs
)

add_executable(${SHORT_NAME}.elf
  src/main.c
  src/graphics.c
  src/font.c
)

target_link_libraries(${SHORT_NAME}.elf
  SceDisplay_stub
  SceCtrl_stub
  SceShellSvc_stub
  SceProcessmgr_stub
  SceVshBridge_stub
  SceHttp_stub
  SceNet_stub
  SceNetCtl_stub
  SceSysmodule_stub
  SceSblUpdateMgr_stub
  taihen_stub
  emmc_helper_user_stub_weak
)

add_custom_target(${SHORT_NAME}.velf ALL
	COMMAND vita-elf-create ${SHORT_NAME}.elf ${SHORT_NAME}.velf
)

add_custom_target(${SHORT_NAME}.self ALL
	COMMAND vita-make-fself -a 0x2808000000000000 -c ${SHORT_NAME}.velf ${SHORT_NAME}.self
)

# emmc_helper: kernel
add_executable(emmc_helper
  src/kernel.c
  src/crc32.c
)
set_target_properties(emmc_helper
  PROPERTIES LINK_FLAGS
  -nostdlib
)
target_link_libraries(emmc_helper
  gcc
  SceSysclibForDriver_stub
  SceThreadmgrForDriver_stub
  SceModulemgrForDriver_stub
  SceModulemgrForKernel_367_stub
  SceSblAIMgrForDriver_stub
  SceIofilemgrForDriver_stub
  SceSysmemForKernel_stub
  SceSysmemForDriver_stub
  SceSdifForDriver_stub
  taihenForKernel_stub
)
vita_create_self(emmc_helper.skprx emmc_helper
  CONFIG ${CMAKE_SOURCE_DIR}/src/kernel_exports.yml
  UNSAFE
)

vita_create_stubs(emmc_helper_stubs emmc_helper ${CMAKE_SOURCE_DIR}/src/kernel_exports.yml KERNEL)

# second kernel helper to work around unloading a kernel module with exported funcs
add_executable(kernel2
  src/kernel2.c
)
set_target_properties(kernel2
  PROPERTIES LINK_FLAGS
  -nostdlib
)
target_link_libraries(kernel2
  gcc
  SceSysclibForDriver_stub
  SceIofilemgrForDriver_stub
  SceSysmemForDriver_stub
  taihenForKernel_stub
  SceThreadmgrForDriver_stub
)
vita_create_self(kernel2.skprx kernel2
  CONFIG ${CMAKE_SOURCE_DIR}/src/kernel2.yml
  UNSAFE
)

# emmc_helper: user version
add_executable(emmc_helper_user
  src/user.c
)
set_target_properties(emmc_helper_user
  PROPERTIES LINK_FLAGS
  -nostdlib
)
target_link_libraries(emmc_helper_user
  emmc_helper_stub
)
vita_create_self(emmc_helper_user.suprx emmc_helper_user
  CONFIG ${CMAKE_SOURCE_DIR}/src/user_exports.yml
  UNSAFE
)
vita_create_stubs(emmc_helper_user_stubs emmc_helper_user ${CMAKE_SOURCE_DIR}/src/user_exports.yml)

add_dependencies(emmc_helper_user emmc_helper_stubs)
add_dependencies(${SHORT_NAME}.elf emmc_helper_user_stubs)
add_dependencies(${SHORT_NAME}.elf kernel2.skprx)
add_dependencies(${SHORT_NAME}.elf emmc_helper.skprx)
add_dependencies(${SHORT_NAME}.elf emmc_helper_user.suprx)
add_dependencies(${SHORT_NAME}.velf ${SHORT_NAME}.elf)
add_dependencies(${SHORT_NAME}.self ${SHORT_NAME}.velf)

vita_create_vpk(${SHORT_NAME}.vpk ${VITA_TITLEID} ${SHORT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE ${CMAKE_BINARY_DIR}/emmc_helper.skprx emmc_helper.skprx
  FILE ${CMAKE_BINARY_DIR}/emmc_helper_user.suprx emmc_helper.suprx
  FILE ${CMAKE_BINARY_DIR}/kernel2.skprx kernel2.skprx
  FILE ${CMAKE_SOURCE_DIR}/res_int/icon.png sce_sys/icon0.png
  FILE ${CMAKE_SOURCE_DIR}/res_int/template.xml sce_sys/livearea/contents/template.xml
  FILE ${CMAKE_SOURCE_DIR}/res_int/bg.png sce_sys/livearea/contents/bg.png
  FILE ${CMAKE_SOURCE_DIR}/res_int/startup.png sce_sys/livearea/contents/startup.png
  FILE ${CMAKE_SOURCE_DIR}/res_ext/fat.bin fat.bin
  FILE ${CMAKE_SOURCE_DIR}/res_ext/rbootstrap.e2xp rconfig.e2xp
  FILE ${CMAKE_SOURCE_DIR}/res_ext/rblob.e2xp rblob.e2xp
  FILE ${CMAKE_SOURCE_DIR}/res_ext/e2xhencfg.skprx e2xhencfg.skprx
  FILE ${CMAKE_SOURCE_DIR}/res_ext/e2xculogo.skprx e2xculogo.skprx
  FILE ${CMAKE_SOURCE_DIR}/res_ext/e2x_ckldr.skprx e2x_ckldr.skprx
  FILE ${CMAKE_SOURCE_DIR}/res_ext/bootlogo.raw bootlogo.raw
  FILE ${CMAKE_SOURCE_DIR}/res_ext/boot_list.txt boot_list.txt
)
