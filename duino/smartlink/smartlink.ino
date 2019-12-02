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

enum { ZX_WAIT_FOR_CLIENT_CONNECT, ZX_IDLE, ZX_RECEIVE_FROM_PC, ZX_SEND_TO_TARGET, ZX_RECEIVE_FROM_TARGET, ZX_SEND_TO_PC, ZX_ACK_TO_PC };  
volatile int targetState = ZX_WAIT_FOR_CLIENT_CONNECT;



// Names might be confusing as we're a conduit, input and output will mean different things.
uint8_t rambuffer[1024*10];

uint8_t *inputData = rambuffer;
int inputDataSize = 0;
int inputDataIndex = 0;

//uint8_t *outputData = rambuffer + (sizeof(*rambuffer) / 2);
//int outputDataSize = 0;
//int outputDataIndex = 0;




const uint8_t *command_buffer = cmdpacket;
size_t command_buffer_size = cmdpacket_size;
size_t command_buffer_index = 0;

inline bool SERIAL_CONNECTED() __attribute__((always_inline));
bool SERIAL_CONNECTED()
{
    return Serial && (Serial.getDTR() || Serial.getRTS() );
}

inline void LED_ON(bool on) __attribute__((always_inline));
void LED_ON(bool on)
{ 
    digitalWrite(33, on ? HIGH : LOW);
}

inline void LED_TOGGLE() __attribute__((always_inline));
void LED_TOGGLE()
{ 
    digitalWrite(33, !digitalRead(33));
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
    if (Serial)
    {
        while(SERIAL_CONNECTED() && Serial.available() > 0)
        {
            Serial.read();
        }
    }

    SPI.setModule(1);
    SPI.beginSlave();
    spi_irq_enable(SPI.dev(), SPI_RXNE_INTERRUPT);

    pinMode(32, INPUT);
    pinMode(33, OUTPUT);
    pinMode(20, INPUT);
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
    if(!SERIAL_CONNECTED())
    {
        targetState = ZX_WAIT_FOR_CLIENT_CONNECT;
    }
    switch(targetState)
    {
        case ZX_WAIT_FOR_CLIENT_CONNECT:
            while (!SERIAL_CONNECTED())
            {
                LED_TOGGLE();
                delay(500);
            }
            while(SERIAL_CONNECTED() && Serial.available() > 0)
            {
                Serial.read();
            }
            LED_ON(false);
            inputDataSize = 0;
            inputDataIndex = 0;
            targetState = ZX_IDLE;
            break;


        case ZX_IDLE:
            // SL packet format : 
            //  8bit : 0x9f - 'S' + 'L'
            // 16bit : arduino command or data size to send to target, top bit set means arduino command, else data size
            //  8bit : crc7 of previous 3 bytes        
            // Read input until we get the expected SL packet from the client. (SL packet is 'S' + 'L' + length hi + length lo + crc7)
            while(inputDataIndex < 4 && SERIAL_CONNECTED() && Serial.available())
            {
                byte s = Serial.read();
                inputData[inputDataIndex++] = s;
            }
            if(inputDataIndex == 4)
            {
                if(inputData[0] == ('S' + 'L') && inputData[3] == crc7(inputData, 3))
                {
                    inputDataIndex = 0;
                    const uint16_t transactionData = inputData[1] | (inputData[2] << 8);
                    if (transactionData & 0x8000)
                    {
                        // Arduino command
                        switch  (transactionData & 0x7fff)
                        {
                          case 1:
                            LED_ON(false);
                            pinMode(20, OUTPUT);
                            digitalWrite(20, LOW);
                            delay(1000);
                            pinMode(20, INPUT);
                            LED_ON(true);
                            break;
                        }
                    }
                    else
                    {
                        // Transmit payload to target
                        LED_ON(true);
                        inputDataSize = transactionData;
                        targetState = ZX_RECEIVE_FROM_PC;                   
                    }                       
                }
            }
            break;

        case ZX_RECEIVE_FROM_PC:
            if(SERIAL_CONNECTED() && Serial.available())
            {
                inputDataIndex += Serial.readBytes(reinterpret_cast<char*>(inputData + inputDataIndex), inputDataSize - inputDataIndex);
                if(inputDataIndex == inputDataSize)
                {
                    inputDataIndex = 0;
                    targetState = ZX_SEND_TO_TARGET;
                }
                LED_TOGGLE();
            }
            break;

        case ZX_ACK_TO_PC:
            Serial.write((char)0xaa);
            LED_ON(true);            
            inputDataSize = 0;
            inputDataIndex = 0;
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
                        command_buffer = inputData;
                        command_buffer_size = inputDataSize;
                        command_buffer_index = 0;
                    }
                    else
                    {
                        //LED_ON(false);
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
                    //LED_ON(false);
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
