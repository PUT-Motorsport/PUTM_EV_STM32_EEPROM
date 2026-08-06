EEPROM emulation library for STM32, based on X-CUBE-EEPROM.
Supported MCUs:
- STM32G0
- STM32G4
- STM32L4
- STM32H5

For now to set your MCU you should change include_directories and sources files in CMakeLists.txt.
You also need to configure defines needed by C library in eeprom_emul_conf.h
