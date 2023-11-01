 #*************************************************************************
 #   CMakeLists.txt  --  This file is part of M2000.                      *
 #                                                                        *
 #   Author: Ivo Filot <i.a.w.filot@tue.nl>                               *
 #                                                                        *
 #   M2000 is a P2000T emulator originally designed by Marcel de Kogel    *
 #                                                                        *
 #   This file is free software: you can redistribute it and/or modify    *
 #   it under the terms of the GNU General Public License as published    *
 #   by the Free Software Foundation, either version 3 of the License,    *
 #   or (at your option) any later version.                               *
 #                                                                        *
 #   This file is distributed in the hope that it will be useful,         *
 #   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 #   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 #   See the GNU General Public License for more details.                 *
 #                                                                        *
 #   You should have received a copy of the GNU General Public License    *
 #   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 #                                                                        *
 #*************************************************************************/

# set minimum cmake requirements
cmake_minimum_required(VERSION 3.18)

if(${CMAKE_VERSION} VERSION_GREATER "3.24.0") 
    cmake_policy(SET CMP0135 NEW) # update timestamps of Allegro5 to moment of extraction
endif()

if(USE_WIN32)
    set(CMAKE_C_COMPILER "i686-w64-mingw32-gcc")
    set(CMAKE_CXX_COMPILER "i686-w64-mingw32-g++")
else()
    set(CMAKE_C_COMPILER "x86_64-w64-mingw32-gcc")
    set(CMAKE_CXX_COMPILER "x86_64-w64-mingw32-g++")
endif()
project(M2000)

# add custom directory to look for .cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules )

# set specific cflags
set(CMAKE_C_FLAGS "-Wall -fomit-frame-pointer -O2 -DLSB_FIRST -DHAVE_FTRUNCATE -DALLEGRO -DALLEGRO_STATICLINK -DJOYSTICK -DSOUND")
set(CMAKE_EXE_LINKER_FLAGS  "-s -mwindows")

# Enable release build
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# ensure Allegro 5 binaries for Windows are downloaded
include(FetchContent)

if(USE_WIN32)
FetchContent_Declare(
  allegro5
  URL https://github.com/liballeg/allegro5/releases/download/5.2.8.0/allegro-i686-w64-mingw32-gcc-12.1.0-posix-dwarf-static-5.2.8.0.zip
  URL_HASH MD5=119474cd3b3484e0a9d0ce446e7a64d2
)
else()
FetchContent_Declare(
  allegro5
  URL https://github.com/liballeg/allegro5/releases/download/5.2.8.0/allegro-x86_64-w64-mingw32-gcc-12.1.0-posix-seh-static-5.2.8.0.zip
  URL_HASH MD5=d671903c6d3af3dfccd63f79f7508f80
)
endif()

FetchContent_MakeAvailable(allegro5)
FetchContent_GetProperties(allegro5)

# Set include folders
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${allegro5_SOURCE_DIR}/include)

# Add sources
set(SOURCES M2000.c P2000.c Z80.c Z80Debug.c Allegro.c)
add_executable(M2000 ${SOURCES})

if(USE_WIN32)
    message(STATUS "Setting compilation for 32 bit")
    set_target_properties(M2000 PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
endif()

# link libraries
target_link_libraries(
    M2000 
    -L${allegro5_SOURCE_DIR}/lib 
    -lallegro
    -lallegro_primitives
    -lallegro_image
    -lallegro_audio
    -lallegro_dialog
    -lallegro_memfile
)

# copy all dependencies to build a self-contained zip file 
set(DEPDLL allegro-5.2.dll allegro_primitives-5.2.dll allegro_image-5.2.dll allegro_audio-5.2.dll allegro_dialog-5.2.dll allegro_memfile-5.2.dll)
FOREACH(FDLL ${DEPDLL})
    file(COPY ${allegro5_SOURCE_DIR}/bin/${FDLL} DESTINATION ${CMAKE_BINARY_DIR})
ENDFOREACH()

set(SUPPORTFILES Default.fnt P2000ROM.bin BASIC.bin)
FOREACH(SFILE ${SUPPORTFILES})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../${SFILE} DESTINATION ${CMAKE_BINARY_DIR})
ENDFOREACH()
