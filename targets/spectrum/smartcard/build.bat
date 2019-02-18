pushd "%~dp0"
pasmo --alocal smartcard_rom.asm smartcard.rom
..\..\..\bin\smartsd smartcard.rom ..\..\..\duino\smartlink\sdimg.h
popd
