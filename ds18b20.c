//-----------------------------------------------------------------------------
// Experiments with the Maxim DS18B20 temperature sensor
//
// Author : Bruce E. Hall <bhall66@gmail.com>
// Website : w8bh.net
// Version : 1.0
// Date : 22 Sep 2013
// Target : ATTmega328P microcontroller
// Language : C, using AVR studio 6
// Size : 1340 bytes, using -O1 optimization
//
// Fuse settings: 8 MHz osc with 65 ms Delay, SPI enable; *NO* clock/8


// ---------------------------------------------------------------------------
// GLOBAL DEFINES

#define F_CPU 16000000L // run CPU at 16 MHz
#define LED 5 // Boarduino LED on PB5
#define ClearBit(x,y) x &= ~_BV(y) // equivalent to cbi(x,y)
#define SetBit(x,y) x |= _BV(y) // equivalent to sbi(x,y)
#define ReadBit(x,y) x & _BV(y) // call with PINx and bit#


// ---------------------------------------------------------------------------
// INCLUDES

#include <avr/io.h> // deal with port registers
#include <util/delay.h> // used for _delay_us function
#include <stdlib.h>
#include <stdio.h>


// ---------------------------------------------------------------------------
// TYPEDEFS

typedef uint8_t byte; // I just like byte & sbyte better
typedef int8_t sbyte;


// ---------------------------------------------------------------------------
// MISC ROUTINES

void InitAVR()
{
    DDRB = 0x3F; // 0011.1111; set B0-B5 as outputs
    DDRC = 0x00; // 0000.0000; set PORTC as inputs
}

void msDelay(int delay) // put into a routine
{ // to remove code inlining
    for (int i=0;i<delay;i++) // at cost of timing accuracy
    _delay_ms(1);
}

void FlashLED()
{
    SetBit(PORTB,LED);
    msDelay(250);
    ClearBit(PORTB,LED);
    msDelay(250);
}


// ---------------------------------------------------------------------------
// HD44780-LCD DRIVER ROUTINES
//
// Routines:
// LCD_Init initializes the LCD controller
// LCD_Cmd sends LCD controller command
// LCD_Char sends single ascii character to display
// LCD_Clear clears the LCD display & homes cursor
// LCD_Home homes the LCD cursor
// LCD_Goto puts cursor at position (x,y)
// LCD_Line puts cursor at start of line (x)
// LCD_Hex displays a hexadecimal value
// LCD_Integer displays an integer value
// LCD_String displays a string
//
// The LCD module requires 6 I/O pins: 2 control lines & 4 data lines.
// PortB is used for data communications with the HD44780-controlled LCD.
// The following defines specify which port pins connect to the controller:


#define LCD_RS 0 // pin for LCD R/S (eg PB0)
#define LCD_E 1 // pin for LCD enable
#define DAT4 2 // pin for d4
#define DAT5 3 // pin for d5
#define DAT6 4 // pin for d6
#define DAT7 5 // pin for d7

// The following defines are HD44780 controller commands
#define CLEARDISPLAY 0x01
#define SETCURSOR 0x80

void PulseEnableLine ()
{
    SetBit(PORTB,LCD_E); // take LCD enable line high
    _delay_us(40); // wait 40 microseconds
    ClearBit(PORTB,LCD_E); // take LCD enable line low
}

void SendNibble(byte data)
{
    PORTB &= 0xC3; // 1100.0011 = clear 4 data lines
    if (data & _BV(4)) SetBit(PORTB,DAT4);
    if (data & _BV(5)) SetBit(PORTB,DAT5);
    if (data & _BV(6)) SetBit(PORTB,DAT6);
    if (data & _BV(7)) SetBit(PORTB,DAT7);
    PulseEnableLine(); // clock 4 bits into controller
}

void SendByte (byte data)
{
    SendNibble(data); // send upper 4 bits
    SendNibble(data<<4); // send lower 4 bits
    ClearBit(PORTB,5); // turn off boarduino LED

}
void LCD_Cmd (byte cmd)
{
    ClearBit(PORTB,LCD_RS); // R/S line 0 = command data
    SendByte(cmd); // send it
}

void LCD_Char (byte ch)
{
    SetBit(PORTB,LCD_RS); // R/S line 1 = character data
    SendByte(ch); // send it
}

void LCD_Init()
{
    LCD_Cmd(0x33); // initialize controller
    LCD_Cmd(0x32); // set to 4-bit input mode
    LCD_Cmd(0x28); // 2 line, 5x7 matrix
    LCD_Cmd(0x0C); // turn cursor off (0x0E to enable)
    LCD_Cmd(0x06); // cursor direction = right
    LCD_Cmd(0x01); // start with clear display
    msDelay(3); // wait for LCD to initialize
}

void LCD_Clear() // clear the LCD display
{
    LCD_Cmd(CLEARDISPLAY);
    msDelay(3); // wait for LCD to process command
}

void LCD_Home() // home LCD cursor (without clearing)
{
    LCD_Cmd(SETCURSOR);
}

void LCD_Goto(byte x, byte y) // put LCD cursor on specified line
{
    byte addr = 0; // line 0 begins at addr 0x00
    switch (y)
    {
        case 1: addr = 0x40; break; // line 1 begins at addr 0x40
        case 2: addr = 0x14; break;
        case 3: addr = 0x54; break;
    }
    LCD_Cmd(SETCURSOR+addr+x); // update cursor with x,y position
}

void LCD_Line(byte row) // put cursor on specified line
{
    LCD_Goto(0,row);
}

void LCD_String(const char *text) // display string on LCD
{
    while (*text) // do until /0 character
    LCD_Char(*text++); // send char & update char pointer
}

void LCD_Hex(int data)
// displays the hex value of DATA at current LCD cursor position
{
    char st[8] = ""; // save enough space for result
    itoa(data,st,16); // convert to ascii hex
    //LCD_String("0x"); // add prefix "0x" if desired
    LCD_String(st); // display it on LCD
}

void HexDigit(byte data)
// lower 4 bits of input -> output hex digit
// helper function for other LCD hex routines
{
    if (data<10) data+='0';
    else data+='A'-10;
    LCD_Char(data);
}

void LCD_HexByte(byte data)
// displays a two-character hexadecimal value of data @ current LCD cursor
{
    HexDigit(data>>4);
    HexDigit(data & 0x0F);
}

void LCD_Integer(int data)
// displays the integer value of DATA at current LCD cursor position
{
    char st[8] = ""; // save enough space for result
    itoa(data,st,10); // convert to ascii
    LCD_String(st); // display in on LCD
}

void LCD_PadInteger(int data, byte size, char padChar)
// displays right-justifed integer on LCD, padding with specified character
// using this instead of sprintf will save you about 1400 bytes of codespace
{
    char st[8] = ""; // save enough space for result
    itoa(data,st,10); // convert to ascii
    byte len = strlen(st); // string length of converted integer
    sbyte blanks = size-len; // required spaces to pad on left
    if (blanks>=0) // do we need to pad at all?
    while(blanks--) // add padding characters...
    LCD_Char(padChar);
    LCD_String(st); // then the converted integer
}


// ---------------------------------------------------------------------------
// "ONE-WIRE" ROUTINES

#define THERM_PORT PORTC
#define THERM_DDR DDRC
#define THERM_PIN PINC
#define THERM_IO 0
#define THERM_INPUT() ClearBit(THERM_DDR,THERM_IO) // make pin an input
#define THERM_OUTPUT() SetBit(THERM_DDR,THERM_IO) // make pin an output
#define THERM_LOW() ClearBit(THERM_PORT,THERM_IO) // take (output) pin low
#define THERM_HIGH() SetBit(THERM_PORT,THERM_IO) // take (output) pin high
#define THERM_READ() ReadBit(THERM_PIN,THERM_IO) // get (input) pin value
#define THERM_CONVERTTEMP 0x44
#define THERM_READSCRATCH 0xBE
#define THERM_WRITESCRATCH 0x4E
#define THERM_COPYSCRATCH 0x48
#define THERM_READPOWER 0xB4
#define THERM_SEARCHROM 0xF0
#define THERM_READROM 0x33
#define THERM_MATCHROM 0x55
#define THERM_SKIPROM 0xCC
#define THERM_ALARMSEARCH 0xEC

// the following arrays specify the addresses of *my* ds18b20 devices
// substitute the address of your devices before using.
byte rom0[] = {0x28,0xE1,0x21,0xA3,0x02,0x00,0x00,0x5B};
byte rom1[] = {0x28,0x1B,0x21,0x30,0x05,0x00,0x00,0xF5};

byte therm_Reset()
{
    byte i;
    THERM_OUTPUT(); // set pin as output
    THERM_LOW(); // pull pin low for 480uS
    _delay_us(480);
    THERM_INPUT(); // set pin as input
    _delay_us(60); // wait for 60uS
    i = THERM_READ(); // get pin value
    _delay_us(420); // wait for rest of 480uS period
    return i;
}

void therm_WriteBit(byte bit)
{
    THERM_OUTPUT(); // set pin as output
    THERM_LOW(); // pull pin low for 1uS
    _delay_us(1);
    if (bit) THERM_INPUT(); // to write 1, float pin
    _delay_us(60);
    THERM_INPUT(); // wait 60uS & release pin
}

byte therm_ReadBit()
{
    byte bit=0;
    THERM_OUTPUT(); // set pin as output
    THERM_LOW(); // pull pin low for 1uS
    _delay_us(1);
    THERM_INPUT(); // release pin & wait 14 uS
    _delay_us(14);
    if (THERM_READ()) bit=1; // read pin value
    _delay_us(45); // wait rest of 60uS period
    return bit;
}

void therm_WriteByte(byte data)
{
    byte i=8;
    while(i--) // for 8 bits:
    {
        therm_WriteBit(data&1); // send least significant bit
        data >>= 1; // shift all bits right
    }
}

byte therm_ReadByte()
{
    byte i=8, data=0;
    while(i--) // for 8 bits:
    {
        data >>= 1; // shift all bits right
        data |= (therm_ReadBit()<<7); // get next bit (LSB first)
    }
    return data;
}

void therm_MatchRom(byte rom[])
{
    therm_WriteByte(THERM_MATCHROM);
    for (byte i=0;i<8;i++)
    therm_WriteByte(rom[i]);
}

void therm_ReadTempRaw(byte id[], byte *t0, byte *t1)
// Returns the two temperature bytes from the scratchpad
{
    therm_Reset(); // skip ROM & start temp conversion
    if (id) therm_MatchRom(id);
    else therm_WriteByte(THERM_SKIPROM);
    therm_WriteByte(THERM_CONVERTTEMP);
    while (!therm_ReadBit()); // wait until conversion completed

    therm_Reset(); // read first two bytes from scratchpad
    if (id) therm_MatchRom(id);
    else therm_WriteByte(THERM_SKIPROM);
    therm_WriteByte(THERM_READSCRATCH);
    *t0 = therm_ReadByte(); // first byte
    *t1 = therm_ReadByte(); // second byte
}

void therm_ReadTempC(byte id[], int *whole, int *decimal)
// returns temperature in Celsius as WW.DDDD, where W=whole & D=decimal
{
    byte t0,t1;
    therm_ReadTempRaw(id,&t0,&t1); // get temperature values
    *whole = (t1 & 0x07) << 4; // grab lower 3 bits of t1
    *whole |= t0 >> 4; // and upper 4 bits of t0
    *decimal = t0 & 0x0F; // decimals in lower 4 bits of t0
    *decimal *= 625; // conversion factor for 12-bit resolution
}

void therm_ReadTempF(byte id[], int *whole, int *decimal)
// returns temperature in Fahrenheit as WW.D, where W=whole & D=decimal
{
    byte t0,t1;
    therm_ReadTempRaw(id,&t0,&t1); // get temperature values
    int t16 = (t1<<8) + t0; // result is temp*16, in celcius
    int t2 = t16/8; // get t*2, with fractional part lost
    int f10 = t16 + t2 + 320; // F=1.8C+32, so 10F = 18C+320 = 16C + 2C + 320
    *whole = f10/10; // get whole part
    *decimal = f10 % 10; // get fractional part
}

inline __attribute__ ((gnu_inline)) void quickDelay(int delay)
// this routine will pause 0.25uS per delay unit
// for testing only; use _us_Delay() routine for >1uS delays
{
    while(delay--) // uses sbiw to subtract 1 from 16bit word
        asm volatile("nop"); // nop, sbiw, brne = 4 cycles = 0.25 uS
}

// ---------------------------------------------------------------------------
// ROM READER PROGRAM

void RomReaderProgram()
// Read the ID of the attached Dallas 18B20 device
// Note: only ONE device should be on the bus.
{
    LCD_String("ID (ROM) Reader:");
    while(1)
    {
        LCD_Line(1);
        // write 64-bit ROM code on first LCD line
        therm_Reset();
        therm_WriteByte(THERM_READROM);
        for (byte i=0; i<8; i++)
        {
            byte data = therm_ReadByte();
            LCD_HexByte(data);
        }
        msDelay(1000); // do a read every second
    }
}

// ---------------------------------------------------------------------------
// DUAL TEMPERATURE PROGRAM

void DualTempProgram()
{
    while(1)
    {
        int whole=0, decimal=0;

        LCD_Line(1);
        LCD_String("Temp1 = ");
        //therm_ReadTempC(&whole,&decimal);
        //LCD_Integer(whole); LCD_Char('.');
        //LCD_PadInteger(decimal,4,'0');
        therm_ReadTempF(rom0,&whole,&decimal);
        LCD_Integer(whole); LCD_Char('.');
        LCD_Integer(decimal);
        
        msDelay(1000);
        FlashLED();
    }
}

// ---------------------------------------------------------------------------
// MAIN PROGRAM

int main(void)
{
    InitAVR();
    LCD_Init();
    //RomReaderProgram();
    DualTempProgram();
}