#!/bin/bash
cd targets/spectrum/smartcard/
pasmoNext --alocal smartcard_rom.asm smartlink.rom
cp smartlink.rom ../../../Tools/VirtualSDImage
cd ../../../Tools/VirtualSDImage
./smartsd smartlink.rom sdimg.h
cp sdimg.h ../../duino/smartlink/
cd ../../duino/smartlink
/Applications/Arduino.app/Contents/MacOS/Arduino --upload smartlink.ino
