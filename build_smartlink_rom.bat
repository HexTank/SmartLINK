pushd %~dp0\targets\spectrum\smartcard
pasmo -v --alocal smartcard_rom.asm smartcard.rom
popd

%~dp0\bin\smartsd %~dp0\targets\spectrum\smartcard\smartcard.rom %~dp0\duino\smartlink\sdimg.h
pause


