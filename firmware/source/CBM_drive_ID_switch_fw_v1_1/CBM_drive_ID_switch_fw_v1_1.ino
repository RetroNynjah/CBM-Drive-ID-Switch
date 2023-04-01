/*
#########################################################################################
#                                                                                       #
#  ID switch sketch for CBM disk drives                                                 #
#  To be used with the Retroninja CBM Drive ID Switch in all CBM IEC drives             #
#                                                                                       #
#  Version 1.1                                                                          #
#  https://github.com/retronynjah                                                       #
#                                                                                       #
#########################################################################################
*/

#include <EEPROM.h>

// searchString is the command that is used for switching ROM.
// It is passed by the 1541-II in reverse order so the variable should be reversed too.
// The command should be preceded by a ROM number between 1 and 4 when used.
// The below reversed searchString in hex ascii is MORNR@ which is specified like this on the C64: 1@RNROM, 2@RNROM and so on.
//byte searchString[] = {0x4D,0x4F,0x52,0x4E,0x52,0x40};
//1@RNDRVID
//DIVRDNR@
byte searchString[] = {0x44,0x49,0x56,0x52,0x44,0x4E,0x52,0x40};

// pin defitions
#define RESETPIN 10
#define CLOCKPIN 13
#define LEDPIN A0
#define LOWBIT 8
#define HIGHBIT 9

int commandLength = sizeof(searchString);
int bytesCorrect = 0;
volatile bool state;
static byte byteCurr;

void pciSetup(byte pin)
{
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}



ISR (PCINT0_vect) // handle pin change interrupt for D8 to D13 here
{    
       // read state of pin 13
       state = PINB & B00100000;
}
 

void cleareeprom(){
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
}


void resetdrive(){

  digitalWrite(RESETPIN, LOW);
  pinMode(RESETPIN, OUTPUT);
  delay(50);
  digitalWrite(RESETPIN, HIGH);
  pinMode(RESETPIN, INPUT);

}


void switchid(byte deviceid){
  switch (deviceid){
    case 8:
      // device 8
      digitalWrite(LOWBIT , LOW);
      digitalWrite(HIGHBIT , LOW);
      break;
    case 9:
      // device 9
      digitalWrite(LOWBIT , HIGH);
      digitalWrite(HIGHBIT , LOW);
      break;
    case 0:
      // device 10
      digitalWrite(LOWBIT , LOW);
      digitalWrite(HIGHBIT , HIGH);
      break;
    case 1:
      // device 11
      digitalWrite(LOWBIT , HIGH);
      digitalWrite(HIGHBIT , HIGH);
      break;
    default:
      break;      
  }

  byte savedid = EEPROM.read(0);
  if (savedid != deviceid){
    EEPROM.write(0, deviceid);
  }
  delay(50);
  resetdrive();
  /*
  for (int x = 0; x <= deviceid; x++){
    digitalWrite (LEDPIN, HIGH);
    delay(30);
    digitalWrite (LEDPIN, LOW);
    delay(250);
  }
  delay (200);
  */  
}


void setup() {

  pinMode(LOWBIT, OUTPUT); // device ID low bit (6522 pin 15)
  pinMode(HIGHBIT, OUTPUT); // device ID high bit (6522 pin 16)
  digitalWrite(LOWBIT,HIGH);
  digitalWrite(HIGHBIT,HIGH);
  pinMode(CLOCKPIN, INPUT); // R/!W
  pinMode(RESETPIN, INPUT); // Keep reset pin as input while not performing reset.

  pinMode (LEDPIN, OUTPUT);

  // set data pins as inputs
  DDRD = B00000000;

  // retrieve last used device ID from ATmega EEPROM and switch ID using CIA pin 15/16
  int lastid = EEPROM.read(0);
  if (lastid > 9){
    cleareeprom();
    lastid = 8;
  }
  switchid(lastid);
  
  // enable pin change interrupt on pin D13(PB5) connected to R/W on 6522
  pciSetup(CLOCKPIN); 
  
}


void loop() {
  
  if (state == HIGH){
    byteCurr = PIND;
    state=LOW;
      
    // we don't have full search string yet. check if current byte is what we are looking for
    if (byteCurr == searchString[bytesCorrect]){
      // It is the byte we're waiting for. increase byte counter
      bytesCorrect++;
	    if (bytesCorrect == commandLength){
        // we have our full search string. wait for next byte
		    while(state == LOW){}
    		byteCurr = PIND;
    		// This byte should be the id number
    		// valid numbers are 1-4 (ASCII 49-52)
    		if ((byteCurr >= 48)&&(byteCurr<=57)){
              // rom number within valid range. Switch rom
              switchid(byteCurr - 48);
    		}
    		else if(byteCurr == searchString[0]){
              // it was the first byte in string, starting with new string
              bytesCorrect = 1;
    		}
        else{
          bytesCorrect = 0;
        }
  	  }
    }
    // byte isn't what we are looking for, is it the first byte in the string then?
    else if(byteCurr == searchString[0]){
        // it was the first byte in string, starting with new string
        bytesCorrect = 1;
    }
    else {
      // byte not correct at all. Start over from the beginning
      bytesCorrect = 0;
    }
  }
  
}
