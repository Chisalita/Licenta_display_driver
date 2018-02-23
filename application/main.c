#include <stdio.h>
#include <pigpio.h>
#include <unistd.h>
#include "registers.h"

#define delay(t) usleep(t*1000)
#define delayMicroseconds(t) usleep(t)


#define TFTWIDTH   240
#define TFTHEIGHT  320

#define RESET_PIN (26)
#define RD_PIN (19)
#define WR_PIN (2)
#define CD_PIN (06)
#define CS_PIN (05)




   // These are single-instruction operations and always inline
   #define RD_ACTIVE  gpioWrite(RD_PIN, 0)
   #define RD_IDLE    gpioWrite(RD_PIN, 1)
   #define WR_ACTIVE  gpioWrite(WR_PIN, 0)
   #define WR_IDLE    gpioWrite(WR_PIN, 1)
   #define CD_COMMAND gpioWrite(CD_PIN, 0)
   #define CD_DATA    gpioWrite(CD_PIN, 1)
   #define CS_ACTIVE  gpioWrite(CS_PIN, 0)
   #define CS_IDLE    gpioWrite(CS_PIN, 1)

// Data write strobe, ~2 instructions and always inline
#define WR_STROBE { WR_ACTIVE; WR_IDLE; }



/*
 #define write8inline(d) { \
   (d & 0xff)<<10
   WR_STROBE; }
*/

void write8(uint8_t value){

   uint32_t mask = 0x3FC00; //for bits 10 to 17
   uint32_t data = ((uint32_t)value)<<10;
   gpioWrite_Bits_0_31_Set(data);
   gpioWrite_Bits_0_31_Clear(data ^ mask);
   WR_STROBE;
}


// Set value of TFT register: 8-bit address, 8-bit value
#define writeRegister8(a, d) { \
  CD_COMMAND; write8(a); CD_DATA; write8(d); }

// Set value of TFT register: 16-bit address, 16-bit value
// See notes at top about macro expansion, hence hi & lo temp vars
#define writeRegister16(a, d) { \
  uint8_t hi, lo; \
  hi = (a) >> 8; lo = (a); CD_COMMAND; write8(hi); write8(lo); \
  hi = (d) >> 8; lo = (d & 0xff); CD_DATA   ; write8(hi); write8(lo); }


int _reset = RESET_PIN;



void drawPixel(int16_t x, int16_t y, uint16_t color);
void writeRegister32(uint8_t r, uint32_t d);
void setAddrWindow(int x1, int y1, int x2, int y2);
void reset(void);
void flood(uint16_t color, uint32_t len);
void fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h, uint16_t fillcolor);
void setLR(void) ;

void writeRegister32(uint8_t r, uint32_t d) {
  CS_ACTIVE;
  CD_COMMAND;
  write8(r);
  CD_DATA;
  delayMicroseconds(10);
  write8(d >> 24);
  delayMicroseconds(10);
  write8(d >> 16);
  delayMicroseconds(10);
  write8(d >> 8);
  delayMicroseconds(10);
  write8(d);
  CS_IDLE;

}


void setAddrWindow(int x1, int y1, int x2, int y2) {
  CS_ACTIVE;
    uint32_t t;

    t = x1;
    t <<= 16;
    t |= x2;
    writeRegister32(ILI9341_COLADDRSET, t);  // HX8357D uses same registers!
    t = y1;
    t <<= 16;
    t |= y2;
    writeRegister32(ILI9341_PAGEADDRSET, t); // HX8357D uses same registers!


  CS_IDLE;
}



int main()
{

    int res = gpioInitialise();
    if (res < 0)
    {
        return 1;
    }

/*
    int mask = 0x8020000;
    while (1)
    {
        time_sleep(1);
        printf("toggle\n");
        //gpioWrite(17, 1);
gpioWrite_Bits_0_31_Set(mask);
        time_sleep(1);
        printf("toggle\n");
gpioWrite_Bits_0_31_Clear(mask);
        //gpioWrite(17, 0);
    }
*/

    reset();
    printf("reset\n");
  delay(200);


   // uint16_t a, d;
    CS_ACTIVE;
    writeRegister8(ILI9341_SOFTRESET, 0);
    delay(50);
    writeRegister8(ILI9341_DISPLAYOFF, 0);

    writeRegister8(ILI9341_POWERCONTROL1, 0x23);
    writeRegister8(ILI9341_POWERCONTROL2, 0x10);
    writeRegister16(ILI9341_VCOMCONTROL1, 0x2B2B);
    writeRegister8(ILI9341_VCOMCONTROL2, 0xC0);
    writeRegister8(ILI9341_MEMCONTROL, ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR);
    writeRegister8(ILI9341_PIXELFORMAT, 0x55);
    writeRegister16(ILI9341_FRAMECONTROL, 0x001B);
    
    writeRegister8(ILI9341_ENTRYMODE, 0x07);
    /* writeRegister32(ILI9341_DISPLAYFUNC, 0x0A822700);*/

    writeRegister8(ILI9341_SLEEPOUT, 0);
    delay(150);
    writeRegister8(ILI9341_DISPLAYON, 0);
    delay(500);
	// *** SPFD5408 change -- Begin
	// Not tested yet
	//writeRegister8(ILI9341_INVERTOFF, 0);
	//delay(500);
    // *** SPFD5408 change -- End
    setAddrWindow(0, 0, TFTWIDTH-1, TFTHEIGHT-1);
  
    printf("setAddr\n");
    int i=0;

    for(i=0; i< 150; i++){
        drawPixel(i,i,0xF800);
    }

    printf("pixels\n");


    fillRect(50, 50, 50, 50, 0x07E0);
    while(1){

    }


    return 0;
}


void drawPixel(int16_t x, int16_t y, uint16_t color) {

  // Clip
  if((x < 0) || (y < 0) || (x >= TFTWIDTH) || (y >= TFTHEIGHT)) return;

  CS_ACTIVE;
    setAddrWindow(x, y, TFTWIDTH-1, TFTHEIGHT-1);
    CS_ACTIVE;
    CD_COMMAND; 
    write8(0x2C);
    CD_DATA; 
    write8(color >> 8); write8(color);
  

  CS_IDLE;
}



void reset(void) {

  CS_IDLE;
//  CD_DATA;
  WR_IDLE;
  RD_IDLE;


  if(_reset) {
    gpioWrite(_reset, 0);
    usleep(2*1000);
    gpioWrite(_reset, 1);
  }

  // Data transfer sync
  CS_ACTIVE;
  CD_COMMAND;
  write8(0x00);
  uint8_t i=0;
  for(; i<3; i++) WR_STROBE; // Three extra 0x00s
  CS_IDLE;
}


void fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h, 
  uint16_t fillcolor) {
  int16_t  x2, y2;

  // Initial off-screen clipping
  if( (w            <= 0     ) ||  (h             <= 0      ) ||
      (x1           >= TFTWIDTH) ||  (y1            >= TFTHEIGHT) ||
     ((x2 = x1+w-1) <  0     ) || ((y2  = y1+h-1) <  0      )) return;
  if(x1 < 0) { // Clip left
    w += x1;
    x1 = 0;
  }
  if(y1 < 0) { // Clip top
    h += y1;
    y1 = 0;
  }
  if(x2 >= TFTWIDTH) { // Clip right
    x2 = TFTWIDTH - 1;
    w  = x2 - x1 + 1;
  }
  if(y2 >= TFTHEIGHT) { // Clip bottom
    y2 = TFTHEIGHT - 1;
    h  = y2 - y1 + 1;
  }

  setAddrWindow(x1, y1, x2, y2);
  flood(fillcolor, (uint32_t)w * (uint32_t)h);
  setLR();
}

void setLR(void) {
  CS_ACTIVE;
  /*writeRegisterPair(HX8347G_COLADDREND_HI, HX8347G_COLADDREND_LO, _width  - 1);
  writeRegisterPair(HX8347G_ROWADDREND_HI, HX8347G_ROWADDREND_LO, _height - 1);
  */
  CS_IDLE;
}

void flood(uint16_t color, uint32_t len) {
  uint16_t blocks;
  uint8_t  i, hi = color >> 8,
              lo = color;

  CS_ACTIVE;
  CD_COMMAND;

    write8(0x2C);
  

  // Write first pixel normally, decrement counter by 1
  CD_DATA;
  write8(hi);
  write8(lo);
  len--;

  blocks = (uint16_t)(len / 64); // 64 pixels/block
  if(hi == lo) {
    // High and low bytes are identical.  Leave prior data
    // on the port(s) and just toggle the write strobe.
    while(blocks--) {
      i = 16; // 64 pixels/block / 4 pixels/pass
      do {
        WR_STROBE; WR_STROBE; WR_STROBE; WR_STROBE; // 2 bytes/pixel
        WR_STROBE; WR_STROBE; WR_STROBE; WR_STROBE; // x 4 pixels
      } while(--i);
    }
    // Fill any remaining pixels (1 to 64)
    for(i = (uint8_t)len & 63; i--; ) {
      WR_STROBE;
      WR_STROBE;
    }
  } else {
    while(blocks--) {
      i = 16; // 64 pixels/block / 4 pixels/pass
      do {
        write8(hi); write8(lo); write8(hi); write8(lo);
        write8(hi); write8(lo); write8(hi); write8(lo);
      } while(--i);
    }
    for(i = (uint8_t)len & 63; i--; ) {
      write8(hi);
      write8(lo);
    }
  }
  CS_IDLE;
}