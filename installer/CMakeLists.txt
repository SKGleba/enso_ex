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

add_executable(${SHORT_NAME}
  src/main.c
  src/graphics.c
  src/font.c
)

add_dependencies(${SHORT_NAME} emmc_helper_365.skprx)
add_dependencies(${SHORT_NAME} emmc_helper_360.skprx)

target_link_libraries(${SHORT_NAME}
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

vita_create_self(${SHORT_NAME}.self ${SHORT_NAME} UNSAFE)

# emmc_helper: kernel version 360
add_executable(emmc_helper_360
  src/kernel_360.c
  src/crc32.c
)
set_target_properties(emmc_helper_360
  PROPERTIES LINK_FLAGS
  -nostdlib
)
target_link_libraries(emmc_helper_360
  k
  gcc
  SceThreadmgrForDriver_stub
  SceModulemgrForDriver_stub
  SceModulemgrForKernel_stub
  SceSblAIMgrForDriver_stub
  SceIofilemgrForDriver_stub
  SceSysmemForKernel_stub
  SceSysmemForDriver_stub
  taihenForKernel_stub
)
vita_create_self(emmc_helper_360.skprx emmc_helper_360
  CONFIG ${CMAKE_SOURCE_DIR}/src/kernel_exports.yml
  UNSAFE
)
vita_create_stubs(emmc_helper_stubs emmc_helper_360 ${CMAKE_SOURCE_DIR}/src/kernel_exports.yml KERNEL)

# emmc_helper: kernel version 365
add_executable(emmc_helper_365
  src/kernel_365.c
  src/crc32.c
)
set_target_properties(emmc_helper_365
  PROPERTIES LINK_FLAGS
  -nostdlib
)
target_link_libraries(emmc_helper_365
  k
  gcc
  SceThreadmgrForDriver_stub
  SceModulemgrForDriver_stub
  SceModulemgrForKernel_367_stub
  SceSblAIMgrForDriver_stub
  SceIofilemgrForDriver_stub
  SceSysmemForKernel_stub
  SceSysmemForDriver_stub
  taihenForKernel_stub
)
vita_create_self(emmc_helper_365.skprx emmc_helper_365
  CONFIG ${CMAKE_SOURCE_DIR}/src/kernel_exports.yml
  UNSAFE
)

# second kernel helper to work around unloading a kernel module with exported funcs
add_executable(kernel2
  src/kernel2.c
)
set_target_properties(kernel2
  PROPERTIES LINK_FLAGS
  -nostdlib
)
target_link_libraries(kernel2
  k
  gcc
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
add_dependencies(${SHORT_NAME} emmc_helper_user_stubs)
add_dependencies(${SHORT_NAME} kernel2.skprx)
add_dependencies(${SHORT_NAME} emmc_helper_360.skprx)
add_dependencies(${SHORT_NAME} emmc_helper_365.skprx)
add_dependencies(${SHORT_NAME} emmc_helper_user.suprx)

vita_create_vpk(${SHORT_NAME}.vpk ${VITA_TITLEID} ${SHORT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE ${CMAKE_SOURCE_DIR}/../enso/fat_365.bin gudfw/fat.bin
  FILE ${CMAKE_SOURCE_DIR}/../enso/fat_360.bin oldfw/fat.bin
  FILE ${CMAKE_SOURCE_DIR}/../patches/clogo_365.e2xp gudfw/clogo.e2xp
  FILE ${CMAKE_SOURCE_DIR}/../patches/clogo_360.e2xp oldfw/clogo.e2xp
  FILE ${CMAKE_SOURCE_DIR}/../patches/rconfig_365.e2xp gudfw/rconfig.e2xp
  FILE ${CMAKE_SOURCE_DIR}/../patches/rconfig_360.e2xp oldfw/rconfig.e2xp
  FILE ${CMAKE_SOURCE_DIR}/../data/bootlogo.raw bootlogo.raw
  FILE ${CMAKE_BINARY_DIR}/emmc_helper_360.skprx oldfw/emmc_helper.skprx
  FILE ${CMAKE_BINARY_DIR}/emmc_helper_365.skprx gudfw/emmc_helper.skprx
  FILE ${CMAKE_BINARY_DIR}/emmc_helper_user.suprx emmc_helper.suprx
  FILE ${CMAKE_BINARY_DIR}/kernel2.skprx kernel2.skprx
  FILE ${CMAKE_SOURCE_DIR}/res/icon.png sce_sys/icon0.png
  FILE ${CMAKE_SOURCE_DIR}/res/template.xml sce_sys/livearea/contents/template.xml
  FILE ${CMAKE_SOURCE_DIR}/res/bg.png sce_sys/livearea/contents/bg.png
  FILE ${CMAKE_SOURCE_DIR}/res/startup.png sce_sys/livearea/contents/startup.png
)
