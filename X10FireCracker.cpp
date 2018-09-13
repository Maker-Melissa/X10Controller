/* 
 * Arduino Interface to the CM17A Wireless X10 dongle. BroHogan 7/19/08
 * Arduino Library Conversion by DaveK AC0KG
 * Updated to Compile With Arduino 1.8.5 by Melissa LeBlanc-Williams
 * Additional Commands added by Melissa LeBlanc-Williams
 *
 * The CM17A gets it power and data using only the RTS, CTS, & Gnd lines.
 * A MAX232 is not req. (0/+5V work OK) If MAX232 IS used reverse all HIGHs & LOWS
 * Signal      RTS DTR        Standby | '1' | Wait | '0' | Wait | '1' | Wait...
 * Reset        0   0         _____________________       _____________________
 * Logical '1'  1   0   RTS _|                     |_____|
 * Logical '0'  0   1         ________       ___________________       ________
 * Standby      1   1   DTR _|        |_____|                   |_____|
 *
 * MINIMUM time for the '1', '0' and 'Wait' states is 0.5ms.
 *
 * At least one signal must be high to keep CM17A powered while transmitting.
 *
 * Each xmit is 40 bits -> "Header" 16 bits,  "Data" 16 bits, "Footer" 8 bits
 *
 * CONNECTION: RTS -> DB9 pin 7  
 *             DTR -> DB9 pin 4 
 *             Gnd -> DB9 pin 5
 */

#include <Arduino.h>

#include "X10FireCracker.h"

// An instance of the EventFuse class
X10FireCracker X10 = X10FireCracker();

// This will generate a warning "only initialized variables can 
// be placed into program memory area". Apparently this is a compiler
// bug, can't do much about it.
uint16_t houseCode[] PROGMEM = {
  0x6000,  // A
  0x7000,  // B
  0x4000,  // C
  0x5000,  // D
  0x8000,  // E
  0x9000,  // F
  0xA000,  // G
  0xB000,  // H
  0xE000,  // I
  0xF000,  // J
  0xC000,  // K
  0xD000,  // L
  0x0000,  // M
  0x1000,  // N
  0x2000,  // O
  0x3000,  // P
};

// This will generate a warning "only initialized variables can 
// be placed into program memory area". Apparently this is a compiler
// bug, can't do much about it.
uint16_t deviceCode[] PROGMEM = {
  0x0000,  // 1
  0x0010,  // 2
  0x0008,  // 3
  0x0018,  // 4
  0x0040,  // 5
  0x0050,  // 6
  0x0048,  // 7
  0x0058,  // 8
  0x0400,  // 9
  0x0410,  // 10
  0x0408,  // 11
  0x0418,  // 12
  0x0440,  // 13
  0x0450,  // 14
  0x0448,  // 15
  0x0458,  // 16
};

// This will generate a warning "only initialized variables can 
// be placed into program memory area". Apparently this is a compiler
// bug, can't do much about it.
uint16_t cmndCode[] PROGMEM = {
  0x0000,  // cmdOn
  0x0020,  // cmdOff
  0x0088,  // 20% cmdBright (0x00A8=5%)
  0x0098,  // 20% cmdDim    (0x00B8=5%)
  0x0080,  // cmdAllOff
  0x0091,  // cmdAllOn
  0x0084,  // cmdLampsOff
  0x0094,  // cmdLampsOn
};


X10FireCracker::X10FireCracker()
{
}

void X10FireCracker::init( int _RTS_Pin, int _DTR_Pin, int _Bit_Delay )
{
  RTS_pin = _RTS_Pin;
  DTR_pin = _DTR_Pin;
  Bit_delay = _Bit_Delay;
  
  pinMode(RTS_pin, OUTPUT);             // RTS -> DB9 pin 7
  pinMode(DTR_pin, OUTPUT);             // DTR -> DB9 pin 4
}

void X10FireCracker::sendCmd(HouseCode house, int device, CommandCode cmnd)
{
  unsigned int dataBuff = 0;
  byte messageBuff[5];

  // Build message by ORing the parts together. No device if Bright or Dim,
  // the bright and dim codes operate on the last-addressed device.
  // Also no device for addressing All
  if ((cmnd == cmdOn) | (cmnd == cmdOff)) {
    dataBuff =  pgm_read_word_near(houseCode  + house     )
              | pgm_read_word_near(deviceCode + (device-1)) 
              | pgm_read_word_near(cmndCode   + cmnd      );
  } else { 
    dataBuff =  pgm_read_word_near(houseCode  + house) 
              | pgm_read_word_near(cmndCode   + cmnd );
  }

  // Build a string for the whole message 
  messageBuff[0] = 0xD5;               // Header byte 0 11010101 = 0xD5 
  messageBuff[1] = 0xAA;               // Header byte 1 10101010 = 0xAA 
  messageBuff[2] = dataBuff >> 8;      // MSB of dataBuff
  messageBuff[3] = dataBuff & 0xFF;    // LSB of dataBuff
  messageBuff[4] = 0xAD;               // Footer byte 10101101 = 0xAD

  // Now send it out to CM17A 
  digitalWrite(DTR_pin, LOW);  // reset device - both low is power off
  digitalWrite(RTS_pin, LOW);
  delay(Bit_delay);

  digitalWrite(DTR_pin, HIGH); // standby mode - supply power
  digitalWrite(RTS_pin, HIGH);
  delay(35);                   // need extra time for it to settle

  for  (byte i = 0; i < 5; i++) {
    for (byte mask = 0x80; mask; mask >>= 1) {
      if (mask & messageBuff[i]) 
        digitalWrite(DTR_pin, LOW);   // 1 = RTS HIGH/DTR-LOW
      else 
        digitalWrite(RTS_pin, LOW);   // 0 = DTR-HIGH/RTS-LOW
      delay(Bit_delay);                 // delay between bits

      digitalWrite(DTR_pin,HIGH);       // wait state between bits
      digitalWrite(RTS_pin,HIGH);
      delay(Bit_delay);
    }  
  }
  delay(1000);                           // wait required before next xmit
}
