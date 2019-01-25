////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Virtual SD controller for SmartLINK by Paul Tankard - 2018
// 
// 
//
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <SPI.h>
#include <type_traits>
#include <array>
#include "sdimg.h"
#include "sdcommands.h"    

extern "C"
{
  void __irq_spi1(void);
}

enum { ZX_IDLE, ZX_RECEIVE_FROM_PC, ZX_SEND_TO_TARGET, ZX_RECEIVE_FROM_TARGET, ZX_SEND_TO_PC, ZX_ACK_TO_PC };  
volatile int targetState = ZX_IDLE;
volatile int targetDataSize = 0;
volatile int targetDataIndex = 0;

byte targetData[1024*10];

const uint8_t *command_buffer = cmdpacket;
size_t command_buffer_size = cmdpacket_size;
size_t command_buffer_index = 0;

inline void LED_ON(bool on) __attribute__((always_inline));
void LED_ON(bool on)
{ 
  digitalWrite(33, on ? HIGH : LOW);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
    Serial.begin(115200);
    while (!Serial) {};
    while(Serial.available() > 0 && Serial.getDTR() && Serial.getRTS())
    {
        Serial.read();
    }

    SPI.setModule(1);
    SPI.beginSlave();
    spi_irq_enable(SPI.dev(), SPI_RXNE_INTERRUPT);

    pinMode(32, INPUT);
    pinMode(33, OUTPUT);
    LED_ON(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop()
{
    switch(targetState)
    {
        case ZX_IDLE:
            targetDataSize = 0;
            while(Serial.available() > 0 && Serial.getDTR() && Serial.getRTS())
            {
                byte s = Serial.read();
                targetData[targetDataSize++] = s;
            }
            if(targetDataSize > 0)
            {
                targetDataIndex = 0;
                targetState = ZX_SEND_TO_TARGET;
            }
            break;

        case ZX_ACK_TO_PC:
            Serial.write((char)0xaa);
            targetState = ZX_IDLE;
            break;

        default:
            break;
    }
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void __irq_spi1(void)
{
    uint8_t data = spi_rx_reg(SPI.dev());                // read data which should flush RXNE
    if(command_buffer==cmdpacket)
    {
        if(command_buffer_index > 0 || (command_buffer_index==0 && (data & 0xc0) == 0x40))
        {
            cmdpacket[command_buffer_index++] = data;
        }
        if(command_buffer_index == command_buffer_size)
        {
            if(cmdPacketValid())
            {
                const std::pair<const uint8_t*, size_t>& returnCommand = sd_response_table[command_buffer[0]&0x3f];
                command_buffer = returnCommand.first;
                command_buffer_size = returnCommand.second;
            }
            command_buffer_index = 0;
        }
        data=0xff;
    } 
    else
    {
        data = command_buffer[command_buffer_index++];
        if(command_buffer_index==command_buffer_size)
        {
            if(command_buffer==cmdSmartLink)
            {
                if(cmdpacket[1]==0)
                {
                    // we're asking to do stuff.
                    if(targetState==ZX_SEND_TO_TARGET)
                    {
                        LED_ON(true);
                        command_buffer = targetData;
                        command_buffer_size = targetDataSize;
                        command_buffer_index = 0;
                    }
                    else
                    {
                        LED_ON(false);
                        command_buffer = cmdpacket;
                        command_buffer_size = cmdpacket_size;
                        command_buffer_index = 0;
                    }

                }
                else if(cmdpacket[1]==1)
                {
                    // we're responding with an ack.
                    targetState = ZX_ACK_TO_PC;
                    LED_ON(false);
                    command_buffer = cmdpacket;
                    command_buffer_size = cmdpacket_size;
                    command_buffer_index = 0;
                }
                else
                {
                    LED_ON(false);
                    command_buffer = cmdpacket;
                    command_buffer_size = cmdpacket_size;
                    command_buffer_index = 0;        
                }
            }
            else if(command_buffer==cmd17)
            {
                //LED_ON(true);
                command_buffer = &virtual_smartlink_sdcard[(cmdpacket[1] << 24) | (cmdpacket[2] << 16) | (cmdpacket[3] << 8) | (cmdpacket[4])];
                command_buffer_size = 512;
                command_buffer_index = 0;
            }
            else
            {
                LED_ON(false);
                command_buffer = cmdpacket;
                command_buffer_size = cmdpacket_size;
                command_buffer_index = 0;
            }
        }
    }
    spi_tx_reg(SPI.dev(), data);                       // echo data back to master  
}