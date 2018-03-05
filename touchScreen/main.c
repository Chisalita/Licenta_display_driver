#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <pigpio.h>

#define REFERENCE_VOLTAGE (3.3f)


int handle; //make it local!!!

void readChannel(uint8_t channel);

int main(int argc, char *argv[])
{
    printf("Touch me!\n");

    int res = gpioInitialise();
    if (res < 0)
    {
        return 1;
    }

    uint8_t cnt = 0, data;

    handle = spiOpen(0, 96000, 0);
    if (handle < 0)
    {
        //bad!
        printf("Error opening SPI!\n");
    }

    while (1)
    {
        sleep(1);
        printf("do\n");
        readChannel(0);
    }

    spiClose(handle);

    return 0;
}

void readChannel(uint8_t channel)
{

    uint8_t command = 0b11 << 6;
    command |= (channel & 0x07) << 3;
    const int commandLen = 3;
    char txBuf[commandLen];
    char rxBuf[commandLen];

    memset(txBuf, 0, commandLen);
    memset(rxBuf, 0, commandLen);

    txBuf[0] = command;

    printf("read %d bytes:\n", spiXfer(handle, txBuf, rxBuf, commandLen));

    for (int i = 0; i < commandLen; ++i)
    {
        printf("0x%X ", rxBuf[i]);
    }
    printf("\n");

    uint16_t result = (rxBuf[0] & 0x01) << 9;
    result |= (rxBuf[1] & 0xFF) << 1;
    result |= (rxBuf[2] & 0x80) >> 7;
    result &= 0x3FF;
    printf("value = %d(%fV)\n", result, (REFERENCE_VOLTAGE * (float) result)/1024.f);



}
