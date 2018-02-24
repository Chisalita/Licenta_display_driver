#include <stdio.h>
#include <pigpio.h>
#include <unistd.h>
#include "registers.h"

#define delay(t) usleep((t)*1000)
#define delayMicroseconds(t) usleep(t)


// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

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
  //  printf("set = 0x%X\n", data);
  //  printf("clear = 0x%X\n", (data ^ mask));
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
void drawBorder ();
void init();

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

// write command
static void tft_command_write(char command)
{
  CD_COMMAND;
    write8(command);
}

// write data
static void tft_data_write(char data)
{
  CD_DATA;
  write8(data);
}

void init(){
	
	reset();
	
	tft_command_write(0x28); //display OFF
	tft_command_write(0x11); //exit SLEEP mode
	tft_data_write(0x00);
	tft_command_write(0xCB); //Power Control A
	tft_data_write(0x39); //always 0x39
	tft_data_write(0x2C); //always 0x2C
	tft_data_write(0x00); //always 0x
	tft_data_write(0x34); //Vcore = 1.6V
	tft_data_write(0x02); //DDVDH = 5.6V
	tft_command_write(0xCF); //Power Control B
	tft_data_write(0x00); //always 0x
	tft_data_write(0x81); //PCEQ off
	tft_data_write(0x30); //ESD protection
	tft_command_write(0xE8); //Driver timing control A
	tft_data_write(0x85); //non‐overlap
	tft_data_write(0x01); //EQ timing
	tft_data_write(0x79); //Pre‐charge timing
	tft_command_write(0xEA); //Driver timing control B
	tft_data_write(0x00); //Gate driver timing
	tft_data_write(0x00); //always 0x
	tft_command_write(0xED); //Power‐On sequence control
	tft_data_write(0x64); //soft start
	tft_data_write(0x03); //power on sequence
	tft_data_write(0x12); //power on sequence
	tft_data_write(0x81); //DDVDH enhance on
	tft_command_write(0xF7); //Pump ratio control
	tft_data_write(0x20); //DDVDH=2xVCI
	tft_command_write(0xC0); //power control 1
	tft_data_write(0x26);
	tft_data_write(0x04); //second parameter for ILI9340 (ignored by ILI9341)
	tft_command_write(0xC1); //power control 2
	tft_data_write(0x11);
	tft_command_write(0xC5); //VCOM control 1
	tft_data_write(0x35);
	tft_data_write(0x3E);
	tft_command_write(0xC7); //VCOM control 2
	tft_data_write(0xBE);
	tft_command_write(0x36); //memory access control = BGR
	tft_data_write(0x88);
	tft_command_write(0xB1); //frame rate control
	tft_data_write(0x00);
	tft_data_write(0x10);
	tft_command_write(0xB6); //display function control
	tft_data_write(0x0A);
	tft_data_write(0xA2);
	tft_command_write(0x3A); //pixel format = 16 bit per pixel
	tft_data_write(0x55);
	tft_command_write(0xF2); //3G Gamma control
	tft_data_write(0x02); //off
	tft_command_write(0x26); //Gamma curve 3
	tft_data_write(0x01);
	tft_command_write(0x2A); //column address set
	tft_data_write(0x00);
	tft_data_write(0x00); //start 0x00
	tft_data_write(0x00);
	tft_data_write(0xEF); //end 0xEF
	tft_command_write(0x2B); //page address set
	tft_data_write(0x00);
	tft_data_write(0x00); //start 0x00
	tft_data_write(0x01);
	tft_data_write(0x3F); //end 0x013F
	
	tft_command_write(0x29); //display ON
}

int main()
{

    int res = gpioInitialise();
    if (res < 0)
    {
        return 1;
    }


   gpioSetMode(RESET_PIN, PI_OUTPUT);
   gpioSetMode(RD_PIN, PI_OUTPUT);
   gpioSetMode(WR_PIN, PI_OUTPUT);
   gpioSetMode(CD_PIN, PI_OUTPUT);
   gpioSetMode(CS_PIN , PI_OUTPUT);
//set data to output
int i=10; 
  for(; i<18; ++i){
    gpioSetMode(i, PI_OUTPUT);
  }

  write8(0x0);//just set to 0

/*
 int mask = 0x8020000;

        uint8_t value = 0x0;
       while (1)
    {
        printf("value: x%X\n", value);
        write8(value);
        
        delay(1500);
        value++;
   
    }
*/







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
    writeRegister8(ILI9341_SLEEPOUT, 0);
    delay(150);
    writeRegister8(ILI9341_DISPLAYON, 0);
    
    //init();
    delay(500);
    setAddrWindow(0, 0, TFTWIDTH-1, TFTHEIGHT-1);
    CS_IDLE;

    printf("setAddr\n");
   // int i=0;

  /*  for(i=0; i< 150; i++){
        drawPixel(i,i,0xF800);
    }

    printf("pixels\n");


    fillRect(50, 50, 50, 50, 0x07E0);
    
    */

  drawBorder();

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


  if(_reset) {/*
    gpioWrite(_reset, 0);
    usleep(2*1000);
    gpioWrite(_reset, 1);
    */

    gpioWrite(_reset,0);
    delay(120);
    gpioWrite(_reset,1);
    delay(120);
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


void fillScreen(uint16_t color) {
  fillRect(0, 0, TFTWIDTH, TFTHEIGHT, color);
}

void drawBorder () {

  // Draw a border

  uint16_t width = TFTWIDTH - 1;
  uint16_t height = TFTHEIGHT - 1;
  uint8_t border = 10;

  fillScreen(RED);
  fillRect(border, border, (width - border * 2), (height - border * 2), WHITE);
  
}