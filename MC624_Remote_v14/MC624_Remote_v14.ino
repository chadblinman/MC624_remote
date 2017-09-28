#include <ResponsiveAnalogRead.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

// The Eye Socket desktop remote for the SoundSkulptor MC624 monitor controller
// by Chad Blinman
// www.chadblinman.com
// 2017
//
// Code for Arduinix 4 tube board by Jeremy Howa, mods by M. Keith Moore 
// www.robotpirate.com
// www.arduinix.com
// 2009/2016(MKM)
// fading transitions sketch for 6-tube IN-17 board with default connections.
// based on 6-tube sketch by Emblazed
// 02/27/2013 - modded for six bulb board, updated flicker fix by Brad L
//
// ResponsiveAnalogRead library by Damien Clarke - https://github.com/dxinteractive/ResponsiveAnalogRead
//
//////////////////////////////////////////////////////////////////////////////
// MC624 serial command reference:
// CMD_VOL = B00000000;    // VOL:b5...b0
// CMD_INOUT = B01000000;  // SUB:b5  OUT:b4-b3(0..3)  IN:b2-b1-b0(0..5)
// CMD_FN = B10000000;     // MUTEL:b5  MUTER:b4  INVR:b3  MONO:b2  DIM:b0
///////////////////////////////////////////////////////////////////////////////
// SN74141 truth table reference:
// D C B A #
// L,L,L,L 0
// L,L,L,H 1
// L,L,H,L 2
// L,L,H,H 3
// L,H,L,L 4
// L,H,L,H 5
// L,H,H,L 6
// L,H,H,H 7
// H,L,L,L 8
// H,L,L,H 9
///////////////////////////////////////////////////////////////////////////////

// SN74141 (1) pins:
int cathodePin_0_a = 2;                
int cathodePin_0_b = 3;
int cathodePin_0_c = 4;
int cathodePin_0_d = 5;
// SN74141 (2) pins:
int cathodePin_1_a = 6;                
int cathodePin_1_b = 7;
int cathodePin_1_c = 8;
int cathodePin_1_d = 9;
// anode pins:
int anodePin_1 = 10;  // marked anode #3 on Arduinix header
int anodePin_2 = 11;  // marked anode #4 on Arduinix header

// button and LED pins:
int in1_button = 22; int in1_LED = 37; 
int in2_button = 23; int in2_LED = 38;
int in3_button = 24; int in3_LED = 39;
int in4_button = 25; int in4_LED = 40;
int in5_button = 26; int in5_LED = 41;
int in6_button = 27; int in6_LED = 42;
int spk1_button = 28; int spk1_LED = 43;
int spk2_button = 29; int spk2_LED = 44;
int spk3_button = 30; int spk3_LED = 45;
int spk4_button = 31; int spk4_LED = 46;
int dim_button = 32; int dim_LED = 47;
int mono_button = 33; int mono_LED = 48;
int invR_button = 34; int invR_LED = 49;
int muteL_button = 35; int muteL_LED = 50;
int muteR_button = 36; int muteR_LED = 51;

// input and speaker variables:
int InOutAddress = 0;                       // EEPROM storage address
int InOut = EEPROM.read(InOutAddress);      // CMD_INOUT value for RS485, read from EEPROM
int previousInOut = InOut;                  // previous value for comparison within loop
int inSelect = InOut & B00000111;           // stored input selection (0-5)
int spkSelect = (InOut & B00011000) >> 3;   // stored speaker selection (0-3)
int previousInSelect = inSelect;            // previous value
int previousSpkSelect = spkSelect;          // previous value
// mute and other function variables:
boolean muteL = 1;                          // muteL and muteR are on at startup
boolean muteR = 1;
boolean mono = 0;                           // other functions are off at startup
boolean invR = 0;
boolean dim = 0;
// calculate the functions byte to send to MC624
int functions = B10000000 + (muteL << 5) + (muteR << 4) + (mono << 3) + (invR << 2) + dim;
int previousFunctions = functions;          // previous value

// status (pressed vs. not pressed) of button switches
boolean muteL_button_status = 0;
boolean muteR_button_status = 0;
boolean mono_button_status = 0;
boolean invR_button_status = 0;
boolean dim_button_status = 0;
boolean in1_button_status = 0;
boolean in2_button_status = 0;
boolean in3_button_status = 0;
boolean in4_button_status = 0;
boolean in5_button_status = 0;
boolean in6_button_status = 0;
boolean spk1_button_status = 0;
boolean spk2_button_status = 0;
boolean spk3_button_status = 0;
boolean spk4_button_status = 0;

int volume = 0;                     // the volume level to send to MC624
int displayVolume = 0;              // the volume level (attenuation) formatted for Nixie display

// make a ResponsiveAnalogRead object, pass in the pin, and either true or false depending on if you want sleep enabled
// enabling sleep will cause values to take less time to stop changing and potentially stop changing more abruptly,
// whereas disabling sleep will cause values to ease into their correct position smoothly and more accurately
// the next optional argument is snapMultiplier, which is set to 0.01 by default
// you can pass it a value from 0 to 1 that controls the amount of easing
// increase this to lessen the amount of easing (such as 0.1) and make the responsive values more responsive
// but doing so may cause more noise to seep through if sleep is not enabled
ResponsiveAnalogRead analog(A0, true, 0.1);

// RS485 serial interface pins:
int SSerialRX = 19;          // Serial Receive pin (Receive Out) - not connected but must be declared
int SSerialTX = 52;          // Serial Transmit pin (Data Input)
int SSerialTxControl = 53;   // RS485 Direction Control pin (Data Enable)
// Object declaration:
SoftwareSerial RS485Serial(SSerialRX, SSerialTX); // RX, TX

///////////////////////////////////////////////////////////////////////
void SetSN74141Chips(int num2, int num1)
{
  // set defaults:
  boolean a=0;
  boolean b=0;
  boolean c=0;
  boolean d=0;
  
  // load a,b,c,d to write to SN74141 (1):
  switch (num1)
  {
    case 0: a=0; b=0; c=0; d=0; break;
    case 1: a=1; b=0; c=0; d=0; break;
    case 2: a=0; b=1; c=0; d=0; break;
    case 3: a=1; b=1; c=0; d=0; break;
    case 4: a=0; b=0; c=1; d=0; break;
    case 5: a=1; b=0; c=1; d=0; break;
    case 6: a=0; b=1; c=1; d=0; break;
    case 7: a=1; b=1; c=1; d=0; break;
    case 8: a=0; b=0; c=0; d=1; break;
    case 9: a=1; b=0; c=0; d=1; break;
    default:
    break;
  }  
  
  // write to output pins:
  digitalWrite(cathodePin_0_a, a);
  digitalWrite(cathodePin_0_b, b);
  digitalWrite(cathodePin_0_c, c);
  digitalWrite(cathodePin_0_d, d);

  // load a,b,c,d to write to SN74141 (2):
  switch (num2)
  {
    case 0: a=0; b=0; c=0; d=0; break;
    case 1: a=1; b=0; c=0; d=0; break;
    case 2: a=0; b=1; c=0; d=0; break;
    case 3: a=1; b=1; c=0; d=0; break;
    case 4: a=0; b=0; c=1; d=0; break;
    case 5: a=1; b=0; c=1; d=0; break;
    case 6: a=0; b=1; c=1; d=0; break;
    case 7: a=1; b=1; c=1; d=0; break;
    case 8: a=0; b=0; c=0; d=1; break;
    case 9: a=1; b=0; c=0; d=1; break;
    default:
    break;
  }
  
  // write to output pins:
  digitalWrite(cathodePin_1_a, a);
  digitalWrite(cathodePin_1_b, b);
  digitalWrite(cathodePin_1_c, c);
  digitalWrite(cathodePin_1_d, d);
}
///////////////////////////////////////////////////////////////////////

float fadeMax = 8.0f;
float fadeStep = 0.9f;              // adjust for fade effect (smaller number = slower fade)
int NumberArray[4] = {0, 0, 0, 0};
int currNumberArray[4] = {0, 0, 0, 0};
float NumberArrayFadeInValue[2] = {0.0f, 0.0f};
float NumberArrayFadeOutValue[2] = {8.0f, 8.0f};

///////////////////////////////////////////////////////////////////////
void DisplayFadeNumberString()
{
 
  // Anode channel 1 - numerals 0,3
  SetSN74141Chips(currNumberArray[0], currNumberArray[3]);
  digitalWrite(anodePin_1, HIGH);
  delay(NumberArrayFadeOutValue[0]);
  SetSN74141Chips(NumberArray[0], NumberArray[3]);
  delay(NumberArrayFadeInValue[0]);
  digitalWrite(anodePin_1, LOW);
  
  // Anode channel 2 - numerals 1,2
  SetSN74141Chips(currNumberArray[1], currNumberArray[2]);
  digitalWrite(anodePin_2, HIGH);
  delay(NumberArrayFadeOutValue[1]);
  SetSN74141Chips(NumberArray[1], NumberArray[2]);
  delay(NumberArrayFadeInValue[1]);
  digitalWrite(anodePin_2, LOW);
  
  if( NumberArray[0] != currNumberArray[0] )
  {
    NumberArrayFadeInValue[0] += fadeStep;
    NumberArrayFadeOutValue[0] -= fadeStep;

    if( NumberArrayFadeInValue[0] >= fadeMax )
    {
      NumberArrayFadeInValue[0] = 0.0f;
      NumberArrayFadeOutValue[0] = fadeMax;
      currNumberArray[0] = NumberArray[0];
    }
  }

  if( NumberArray[3] != currNumberArray[3] )
  {
    NumberArrayFadeInValue[0] += fadeStep;
    NumberArrayFadeOutValue[0] -= fadeStep;

    if( NumberArrayFadeInValue[0] >= fadeMax )
    {
      NumberArrayFadeInValue[0] = 0.0f;
      NumberArrayFadeOutValue[0] = fadeMax;
      currNumberArray[3] = NumberArray[3];
    }
  }

  if( NumberArray[1] != currNumberArray[1] )
  {
    NumberArrayFadeInValue[1] += fadeStep;
    NumberArrayFadeOutValue[1] -= fadeStep;

    if( NumberArrayFadeInValue[1] >= fadeMax )
    {
      NumberArrayFadeInValue[1] = 0.0f;
      NumberArrayFadeOutValue[1] = fadeMax;
      currNumberArray[1] = NumberArray[1];
    }
  }

  if( NumberArray[2] != currNumberArray[2] )
  {
    NumberArrayFadeInValue[1] += fadeStep;
    NumberArrayFadeOutValue[1] -= fadeStep;

    if( NumberArrayFadeInValue[1] >= fadeMax )
    {
      NumberArrayFadeInValue[1] = 0.0f;
      NumberArrayFadeOutValue[1] = fadeMax;
      currNumberArray[2] = NumberArray[2];
    }
  }

}
///////////////////////////////////////////////////////////////////////

void setup() 
{

  // Nixie driver pin configuration:
  pinMode(cathodePin_0_a, OUTPUT);
  pinMode(cathodePin_0_b, OUTPUT);
  pinMode(cathodePin_0_c, OUTPUT);
  pinMode(cathodePin_0_d, OUTPUT);
  
  pinMode(cathodePin_1_a, OUTPUT);
  pinMode(cathodePin_1_b, OUTPUT);
  pinMode(cathodePin_1_c, OUTPUT);
  pinMode(cathodePin_1_d, OUTPUT);
  
  pinMode(anodePin_1, OUTPUT);
  pinMode(anodePin_2, OUTPUT);

  // button switch pin configuration:
  pinMode(in1_button, INPUT_PULLUP);
  pinMode(in2_button, INPUT_PULLUP);
  pinMode(in3_button, INPUT_PULLUP);
  pinMode(in4_button, INPUT_PULLUP);
  pinMode(in5_button, INPUT_PULLUP);
  pinMode(in6_button, INPUT_PULLUP);
  pinMode(spk1_button, INPUT_PULLUP);
  pinMode(spk2_button, INPUT_PULLUP);
  pinMode(spk3_button, INPUT_PULLUP);
  pinMode(spk4_button, INPUT_PULLUP);
  pinMode(dim_button, INPUT_PULLUP);
  pinMode(mono_button, INPUT_PULLUP);
  pinMode(invR_button, INPUT_PULLUP);
  pinMode(muteL_button, INPUT_PULLUP);
  pinMode(muteR_button, INPUT_PULLUP);
  
  // button LED pin configuration:
  pinMode(in1_LED, OUTPUT);
  pinMode(in2_LED, OUTPUT);
  pinMode(in3_LED, OUTPUT);
  pinMode(in4_LED, OUTPUT);
  pinMode(in5_LED, OUTPUT);
  pinMode(in6_LED, OUTPUT);
  pinMode(spk1_LED, OUTPUT);
  pinMode(spk2_LED, OUTPUT);
  pinMode(spk3_LED, OUTPUT);
  pinMode(spk4_LED, OUTPUT);
  pinMode(dim_LED, OUTPUT);
  pinMode(mono_LED, OUTPUT);
  pinMode(invR_LED, OUTPUT);
  pinMode(muteL_LED, OUTPUT);
  pinMode(muteR_LED, OUTPUT);

  // RS485 pin configuration:
  pinMode(SSerialTxControl, OUTPUT);
  digitalWrite(SSerialTxControl, HIGH);

  // start the RS485 software serial port:
  RS485Serial.begin(9600);

  // start the USB serial monitor port:
//  Serial.begin(9600);                               // uncomment if needed

  // set initial monitor functions (muteL and muteR):
  RS485Serial.write(functions);
  
  // run startup display sequence:
  int digit[4] = {0,2,3,1};
  int marquee_button[15] = {in1_LED, in2_LED, in3_LED, in4_LED, in5_LED, in6_LED, dim_LED, mono_LED, invR_LED, muteL_LED, muteR_LED, spk4_LED, spk3_LED, spk2_LED, spk1_LED}; 
  for (int blinky = 0; blinky <= 14; blinky++)   // blink button LEDs left to right
    {
    digitalWrite(marquee_button[blinky], HIGH);
    delay(60);
    digitalWrite(marquee_button[blinky], LOW);      
    }  
  for (int blinky = 13; blinky >= 0; blinky--)   // then right to left
  {
    digitalWrite(marquee_button[blinky], HIGH);
    delay(60);
    digitalWrite(marquee_button[blinky], LOW);
  }
  for (int number = 0; number <= 9; number++)    // cycle Nixie digits 0000 - 9999
  {
    for (int i = 0; i < 4; i++)
    {
      int rep = 0;
      while(rep < 7)
      {
        NumberArray[digit[i]] = number;
        DisplayFadeNumberString();
        rep++;
      }
    }
  }  
  int rep = 0;                                     // flash button LEDs three times
  while(rep < 3)
  {
    for (int blinky = 0; blinky <= 14; blinky++)   // on
      {
      digitalWrite(marquee_button[blinky], HIGH);
      }
    delay(80);
    for (int blinky = 0; blinky <= 14; blinky++)   // off
      {
      digitalWrite(marquee_button[blinky], LOW);
      }      
    delay(80);
    rep++;
  }
  
  // set Input and Speaker button LEDs according to stored InOut value:
  digitalWrite((in1_LED + inSelect), HIGH);
  digitalWrite((spk1_LED + spkSelect), HIGH);

  // set monitor function button LEDs (muteL and muteR):
  digitalWrite(muteL_LED, muteL);  
  digitalWrite(muteR_LED, muteR);  

  // read and set initial volume:
  volume = analog.getValue() / 16;                       // get responsive value and divide by 16 to scale to 0-63
  RS485Serial.write(volume);                             // write to RS485
    
}

void loop()     
{
 
  // input (source) select:
  if (digitalRead(in1_button) == LOW && in1_button_status == 0)  // input 1 button is pressed
  {
    inSelect = 0;                      // set input 1
    in1_button_status = 1;             // set button status
  }
  if (digitalRead(in1_button) == HIGH && in1_button_status == 1)
  {
    in1_button_status = 0;             // reset status
  }

  if (digitalRead(in2_button) == LOW && in2_button_status == 0)  // input 2 button is pressed
  {
    inSelect = 1;                      // set input 2
    in2_button_status = 1;             // set button status
  }
  if (digitalRead(in2_button) == HIGH && in2_button_status == 1)
  {
    in2_button_status = 0;             // reset status
  }

  if (digitalRead(in3_button) == LOW && in3_button_status == 0)  // input 3 button is pressed
  {
    inSelect = 2;                      // set input 3
    in3_button_status = 1;             // set button status
  }
  if (digitalRead(in3_button) == HIGH && in3_button_status == 1)
  {
    in3_button_status = 0;             // reset status
  }

  if (digitalRead(in4_button) == LOW && in4_button_status == 0)  // input 4 button is pressed
  {
    inSelect = 3;                      // set input 4
    in4_button_status = 1;             // set button status
  }
  if (digitalRead(in4_button) == HIGH && in4_button_status == 1)
  {
    in4_button_status = 0;             // reset status
  }

  if (digitalRead(in5_button) == LOW && in5_button_status == 0)  // input 5 button is pressed
  {
    inSelect = 4;                      // set input 5
    in5_button_status = 1;             // set button status
  }
  if (digitalRead(in5_button) == HIGH && in5_button_status == 1)
  {
    in5_button_status = 0;             // reset status
  }

  if (digitalRead(in6_button) == LOW && in6_button_status == 0)  // input 6 button is pressed
  {
    inSelect = 5;                      // set input 6
    in6_button_status = 1;             // set button status
  }
  if (digitalRead(in6_button) == HIGH && in6_button_status == 1)
  {
    in6_button_status = 0;             // reset status
  }

  // speaker select:
  if (digitalRead(spk1_button) == LOW && spk1_button_status == 0)  // speaker 1 button is pressed
  {
    spkSelect = 0;                      // set speaker 1
    spk1_button_status = 1;             // set button status
  }
  if (digitalRead(spk1_button) == HIGH && spk1_button_status == 1)
  {
    spk1_button_status = 0;             // reset status
  }

  if (digitalRead(spk2_button) == LOW && spk2_button_status == 0)  // speaker 2 button is pressed
  {
    spkSelect = 1;                      // set speaker 2
    spk2_button_status = 1;             // set button status
  }
  if (digitalRead(spk2_button) == HIGH && spk2_button_status == 1)
  {
    spk2_button_status = 0;             // reset status
  }

  if (digitalRead(spk3_button) == LOW && spk3_button_status == 0)  // speaker 3 button is pressed
  {
    spkSelect = 2;                      // set speaker 3
    spk3_button_status = 1;             // set button status
  }
  if (digitalRead(spk3_button) == HIGH && spk3_button_status == 1)
  {
    spk3_button_status = 0;             // reset status
  }

  if (digitalRead(spk4_button) == LOW && spk4_button_status == 0)  // speaker 4 button is pressed
  {
    spkSelect = 3;                      // set speaker 4
    spk4_button_status = 1;             // set button status
  }
  if (digitalRead(spk4_button) == HIGH && spk4_button_status == 1)
  {
    spk4_button_status = 0;             // reset status
  }

  // calculate InOut value:
  InOut = B01000000 + inSelect + (spkSelect << 3);

  // if input or output selection is changed, send to RS485 and update:
  if (InOut != previousInOut)
  {
    RS485Serial.write(InOut);                    // write to RS485
//    Serial.print("input select: ");
//    Serial.println(inSelect + 1);
//    Serial.print("speaker select: ");
//    Serial.println(spkSelect + 1); 
    if (inSelect != previousInSelect)                     // if the input select is different:
    {
      digitalWrite((in1_LED + inSelect), HIGH);           // turn on new input LED
      digitalWrite((in1_LED + previousInSelect), LOW);    // turn off previous input LED
      previousInSelect = inSelect;                        // update previous
    }
    if (spkSelect != previousSpkSelect)                   // if the speaker select is different:
    {
      digitalWrite((spk1_LED + spkSelect), HIGH);         // turn on new speaker LED
      digitalWrite((spk1_LED + previousSpkSelect), LOW);  // turn off previous speaker LED
      previousSpkSelect = spkSelect;                      // update previous
    }   
    previousInOut = InOut;                  // update previous value
    EEPROM.update(InOutAddress, InOut);     // update value to EEPROM
  }

  // mutes and other functions:
  if (digitalRead(muteL_button) == LOW && muteL_button_status == 0)   // mute L button is pressed
  {
    muteL = !muteL;                      // toggle mute L
    muteL_button_status = 1;             // set button status
  }
  if (digitalRead(muteL_button) == HIGH && muteL_button_status == 1)
  {
    muteL_button_status = 0;             // reset status
  }

  if (digitalRead(muteR_button) == LOW && muteR_button_status == 0)   // mute R button is pressed
  {
    muteR = !muteR;                       // toggle mute R
    muteR_button_status = 1;              // set button status
  }
  if (digitalRead(muteR_button) == HIGH && muteR_button_status == 1)
  {
    muteR_button_status = 0;              // reset status
  }

  if (digitalRead(mono_button) == LOW && mono_button_status == 0)    // mono button is pressed
  {
    mono = !mono;                         // toggle mono
    mono_button_status = 1;               // set button status
  }
  if (digitalRead(mono_button) == HIGH && mono_button_status == 1)
  {
    mono_button_status = 0;               // reset status
  }

  if (digitalRead(invR_button) == LOW && invR_button_status == 0)   // invert R button is pressed
  {
    invR = !invR;                         // toggle invert R
    invR_button_status = 1;               // set button status
  }
  if (digitalRead(invR_button) == HIGH && invR_button_status == 1)
  {
    invR_button_status = 0;               // reset status
  }

  if (digitalRead(dim_button) == LOW && dim_button_status == 0)    // dim button is pressed
  {
    dim = !dim;                           // toggle dim
    dim_button_status = 1;                // set button status
  }
  if (digitalRead(dim_button) == HIGH && dim_button_status == 1)
  {
    dim_button_status = 0;                // reset status
  }
  
  // calculate functions value:
  functions = B10000000 + (muteL << 5) + (muteR << 4) + (mono << 3) + (invR << 2) + dim;

  // if functions have changed, send to RS485 and toggle LEDs:
  if (functions != previousFunctions)
  { 
    RS485Serial.write(functions);
//    Serial.print("functions: ");           // print to serial monitor
//    Serial.println(functions, BIN);        // print to serial monitor 
    digitalWrite(muteL_LED, muteL);        // toggle LEDs
    digitalWrite(muteR_LED, muteR);
    digitalWrite(mono_LED, mono);
    digitalWrite(invR_LED, invR);
    digitalWrite(dim_LED, dim);
    previousFunctions = functions;         // update
  }

  // set NumberArray for input (source) and output (speaker)
  NumberArray[0] = (inSelect + 1);         // selected input
  if (muteL == 1 && muteR == 1)            // if MUTE L and R are on...
  {
    NumberArray[1] = 0;                    // ...speaker 0
  }
  else
  {
    NumberArray[1] = (spkSelect + 1);      // ...otherwise selected speaker
  } 

  // volume read - update the ResponsiveAnalogRead object every loop
  analog.update();
   
  // if the volume level has changed, write to RS485 serial:
  if(analog.hasChanged()) 
  {
    volume = analog.getValue() / 16;                     // get responsive value and divide by 16 to scale to 0-63
    RS485Serial.write(volume);                           // write to RS485
  }
  
  // format volume and update NumberArray
  displayVolume = map(volume, 0, 63, 63, 0);             // reverse scale according to MC624 volume display
  if (dim == 1) {displayVolume = displayVolume + 15;}    // when dimmed, adjust according to MC624 dim setting
  int volumeTens = ((displayVolume/10)%10);              // separate into digits - tens
  int volumeOnes = (displayVolume%10);                   // separate into digits - ones
  NumberArray[2] = volumeTens;                           // set NumberArray for volume - tens digit
  NumberArray[3] = volumeOnes;                           // set NumberArray for volume - ones digit

  DisplayFadeNumberString();                             // display full NumberArray on Nixies
}
