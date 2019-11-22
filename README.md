# SmartLINK

SmartLink is an STM32duino based project for acting as a bridge between retro add-on devices with SPI over SD capability and a client PC, the name was based upon the original device it was created for, the Retroleum Smart Card.

Currently, the project supports the following devices :

- [Retroleum Smart Card](http://blog.retroleum.co.uk/smart-card-for-zx-spectrum/ "Retroleum Smart Card")
<br><br>

#### Client Software Setup

You will need to follow the instructions [here](https://github.com/rogerclarkmelbourne/Arduino_STM32/wiki/Installation "here"), but instead of downloading the linked STM32 files zip from there, download it from [here](https://github.com/HexTank/Arduino_STM32 "here") instead, as this is a known working branch.

The project will eventually migrate to the official STM branch once Maple Mini SPI support is stable.

You will need a compiler to build the client side tools, Visual Studio on Windows, G++ on Linux and Clang on OSX have all been tested.

#### Target Software Setup
###### Retroleum Smart Card
The spectrum assembly files will require Pasmo.




