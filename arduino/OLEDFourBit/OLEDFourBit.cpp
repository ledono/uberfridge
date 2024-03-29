#include "OLEDFourBit.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "WProgram.h"

OLEDFourBit::OLEDFourBit(uint8_t rs, uint8_t rw, uint8_t enable,
			     uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
{
  init(rs, rw, enable, d4, d5, d6, d7);
}

void OLEDFourBit::init(uint8_t rs, uint8_t rw, uint8_t enable,
			 uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
{
  _rs_pin = rs;
  _rw_pin = rw;
  _enable_pin = enable;
  _busy_pin = d7;
  
  _data_pins[0] = d4;
  _data_pins[1] = d5;
  _data_pins[2] = d6;
  _data_pins[3] = d7; 


  pinMode(_rs_pin, OUTPUT);
  pinMode(_rw_pin, OUTPUT);
  pinMode(_enable_pin, OUTPUT);
  
  _displayfunction = LCD_FUNCTIONSET | LCD_4BITMODE;
   
  begin(20, 4);  
}

void OLEDFourBit::begin(uint8_t cols, uint8_t lines) {
  _numlines = lines;
  _currline = 0;
  
  pinMode(_rs_pin, OUTPUT);
  pinMode(_rw_pin, OUTPUT);
  pinMode(_enable_pin, OUTPUT);
  
  digitalWrite(_rs_pin, LOW);
  digitalWrite(_enable_pin, LOW);
  digitalWrite(_rw_pin, LOW);
  
  // SEE PAGE 20 of NHD-0420DZW-AY5 
  delayMicroseconds(50000); // wait 50 ms just to be sure tha the lcd is initialized
  
  // Now we pull both RS and R/W low to begin commands
  
  for (int i = 0; i < 4; i++) {
    pinMode(_data_pins[i], OUTPUT);
    digitalWrite(_data_pins[i], LOW);
  }

  delayMicroseconds(100000);
  write4bits(0x03);
  delayMicroseconds(100000);
  write4bits(0x02);
  delayMicroseconds(10000);
  write4bits(0x02);
  delayMicroseconds(10000);
  write4bits(0x08);
   
  
  //command(0x28);
  delayMicroseconds(10000);
  
  command(0x08);	// Display off
  delayMicroseconds(10000);
	
  command(0x01);	// display clear
  delayMicroseconds(10000);

  command(0x06);	// Entry Mode Set:
  delayMicroseconds(10000);

	
  command(0x02);	// Home
  delayMicroseconds(10000);

  command(0x0C);	// display on/ cursor on/ cursor blink
  delayMicroseconds(10000);
	
	
}

/********** high level commands, for the user! */
void OLEDFourBit::clear()
{
  command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
}

void OLEDFourBit::home()
{
  command(LCD_RETURNHOME);  // set cursor position to zero
}

void OLEDFourBit::setCursor(uint8_t col, uint8_t row)
{
  uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if ( row >= _numlines ) {
    row = 0;  //write to first line if out off bounds
  }
  
  command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void OLEDFourBit::noDisplay() {
  _displaycontrol &= ~LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void OLEDFourBit::display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void OLEDFourBit::noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void OLEDFourBit::cursor() {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void OLEDFourBit::noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void OLEDFourBit::blink() {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void OLEDFourBit::scrollDisplayLeft(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void OLEDFourBit::scrollDisplayRight(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void OLEDFourBit::leftToRight(void) {
  _displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void OLEDFourBit::rightToLeft(void) {
  _displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void OLEDFourBit::autoscroll(void) {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void OLEDFourBit::noAutoscroll(void) {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void OLEDFourBit::createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++) {
    write(charmap[i]);
  }
}

/*********** mid level commands, for sending data/cmds */

inline void OLEDFourBit::command(uint8_t value) {
  send(value, LOW);
  waitBusy();
}

inline void OLEDFourBit::write(uint8_t value) {
  send(value, HIGH);
  waitBusy();
}

/************ low level data pushing commands **********/

// write either command or data
void OLEDFourBit::send(uint8_t value, uint8_t mode) {
  digitalWrite(_rs_pin, mode);
  pinMode(_rw_pin, OUTPUT);
  digitalWrite(_rw_pin, LOW);
  
  write4bits(value>>4);
  write4bits(value);
}

void OLEDFourBit::pulseEnable(void) {
  digitalWrite(_enable_pin, HIGH);
  delayMicroseconds(100);    // enable pulse must be >450ns
  digitalWrite(_enable_pin, LOW);
}

void OLEDFourBit::write4bits(uint8_t value) {
  for (int i = 0; i < 4; i++) {
    pinMode(_data_pins[i], OUTPUT);
    digitalWrite(_data_pins[i], (value >> i) & 0x01);
  }
  delayMicroseconds(100);
  pulseEnable();
}

void OLEDFourBit::waitBusy(void) {
  //delayMicroseconds(5000);
  unsigned char busy = 1;
  pinMode(_busy_pin, INPUT);
  digitalWrite(_rs_pin, LOW);	
  digitalWrite(_rw_pin, HIGH);      
  do{
  	digitalWrite(_enable_pin, LOW);
  	digitalWrite(_enable_pin, HIGH);
  	delayMicroseconds(10);
  	busy = digitalRead(_busy_pin);
  	digitalWrite(_enable_pin, LOW);
  	
  	pulseEnable();		// get remaining 4 bits, which are not used.
  	
  }while(busy);
  
  pinMode(_busy_pin, OUTPUT);
  digitalWrite(_rw_pin, LOW);
}

char OLEDFourBit::readChar(void){
  char value=0x00;
  for (int i = 0; i < 4; i++) {
    pinMode(_data_pins[i], INPUT);
  }
  digitalWrite(_rs_pin, HIGH);	
  digitalWrite(_rw_pin, HIGH); 
  pulseEnable();
  delayMicroseconds(600);
  for (int i = 0; i < 4; i++) {
    value = value | (digitalRead(_data_pins[i]) << (i+4));
  }
  pulseEnable();
  delayMicroseconds(600);
  for (int i = 0; i < 4; i++) {
    value = value | (digitalRead(_data_pins[i]) << (i));
  }
  return value;
}