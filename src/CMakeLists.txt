# set minimum cmake requirements
cmake_minimum_required(VERSION 3.18)

set(CMAKE_C_COMPILER "x86_64-w64-mingw32-gcc")
set(CMAKE_CXX_COMPILER "x86_64-w64-mingw32-g++")
project(m2000win)

# add custom directory to look for .cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules )

# set specific cflags
set(CMAKE_C_FLAGS "-Wall -fomit-frame-pointer -O2 -DLSB_FIRST -DHAVE_FTRUNCATE -DALLEGRO -DMSDOS")

# set linking flags
set(CMAKE_STATIC_LINKER_FLAGS "-s")

# Enable release build
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# ensure Allegro 5 binaries for Windows are downloaded
include(FetchContent)
FetchContent_Declare(
  allegro5
  #URL https://github.com/liballeg/allegro5/releases/download/5.2.8.0/allegro-x86_64-w64-mingw32-gcc-12.1.0-posix-seh-dynamic-5.2.8.0.zip
  #URL_HASH MD5=53ee801345c191eed5de01b2b9554475
  URL https://github.com/liballeg/allegro5/releases/download/5.2.8.0/allegro-x86_64-w64-mingw32-gcc-12.1.0-posix-seh-static-5.2.8.0.zip
  URL_HASH MD5=d671903c6d3af3dfccd63f79f7508f80
)
FetchContent_MakeAvailable(allegro5)
FetchContent_GetProperties(allegro5)

# Set include folders
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${allegro5_SOURCE_DIR}/include)

# Add sources
set(SOURCES M2000.c P2000.c Z80.c Z80Debug.c Unix.c allegro.c)
add_executable(m2000win ${SOURCES})

# link libraries
target_link_libraries(
    m2000win 
    -L${allegro5_SOURCE_DIR}/lib 
    -lallegro
    -lallegro_primitives
    -lallegro_image
    -lallegro_audio
)

# copy all dependencies to build a self-contained zip file 
set(DEPDLL allegro allegro_primitives allegro_image allegro_audio)
FOREACH(FDLL ${DEPDLL})
    file(COPY ${allegro5_SOURCE_DIR}/bin/${FDLL}-5.2.dll DESTINATION ${CMAKE_BINARY_DIR})
ENDFOREACH()

set(SUPPORTFILES Default.fnt P2000ROM.bin BASIC.bin)
FOREACH(SFILE ${SUPPORTFILES})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../${SFILE} DESTINATION ${CMAKE_BINARY_DIR})
ENDFOREACH()

file(ARCHIVE_CREATE
    OUTPUT "m2000win.zip"
    PATHS 
        "${CMAKE_BINARY_DIR}/m2000win.exe"
        "${CMAKE_BINARY_DIR}/Default.fnt"
        "${CMAKE_BINARY_DIR}/P2000ROM.bin"
        "${CMAKE_BINARY_DIR}/BASIC.bin"
        "${CMAKE_BINARY_DIR}/allegro-5.2.dll"
        "${CMAKE_BINARY_DIR}/allegro_primitives-5.2.dll"
        "${CMAKE_BINARY_DIR}/allegro_image-5.2.dll"
        "${CMAKE_BINARY_DIR}/allegro_audio-5.2.dll"
    FORMAT "zip")

###
# Installing
##
install (TARGETS m2000win DESTINATION bin)
