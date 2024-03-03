# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/shado/esp/v5.2/esp-idf/components/bootloader/subproject"
  "C:/Users/shado/Data/Projects/GitHub/ESP32/T_Beam_Bluetooth_SPP_SSP/build/bootloader"
  "C:/Users/shado/Data/Projects/GitHub/ESP32/T_Beam_Bluetooth_SPP_SSP/build/bootloader-prefix"
  "C:/Users/shado/Data/Projects/GitHub/ESP32/T_Beam_Bluetooth_SPP_SSP/build/bootloader-prefix/tmp"
  "C:/Users/shado/Data/Projects/GitHub/ESP32/T_Beam_Bluetooth_SPP_SSP/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/shado/Data/Projects/GitHub/ESP32/T_Beam_Bluetooth_SPP_SSP/build/bootloader-prefix/src"
  "C:/Users/shado/Data/Projects/GitHub/ESP32/T_Beam_Bluetooth_SPP_SSP/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/shado/Data/Projects/GitHub/ESP32/T_Beam_Bluetooth_SPP_SSP/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/shado/Data/Projects/GitHub/ESP32/T_Beam_Bluetooth_SPP_SSP/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
