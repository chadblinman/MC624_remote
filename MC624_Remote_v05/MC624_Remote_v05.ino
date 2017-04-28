#include <SoftwareSerial.h>
#include <EEPROM.h>

// The Eye Socket desktop remote for the SoundSkulptor MC624 monitor controller
// by Chad Blinman
// www.chadblinman.com
// 2017
//
// MC624 serial command reference:
// CMD_VOL = B00000000;    // VOL:b5...b0
// CMD_INOUT = B01000000;  // SUB:b5  OUT:b4-b3(0..3)  IN:b2-b1-b0(0..5)
// CMD_FN = B10000000;     // MUTEL:b5  MUTER:b4  INVR:b3  MONO:b2  DIM:b0

// Code for Arduinix 4 tube board by Jeremy Howa mods by M. Keith Moore 
// for IN-2/IN-12 4 digits and blinking colon
// www.robotpirate.com
// www.arduinix.com
// 2009/2016(MKM)
//
// Note: Anod pin 3 and 2 are used for colons (not used in this design).
// 
// Anod to number diagram for IN-2/IN-12 boards:
//          num array position
//            0   1   2   3
// Anod 0     #           #
// Anod 1         #   #
//
// Anod 2   Array #2 Colon=0
// Anod 3   Array #1 Colon=0

// SN74141 Truth Table:
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

// SN74141 pins (1):
int ledPin_0_a = 2;                
int ledPin_0_b = 3;
int ledPin_0_c = 4;
int ledPin_0_d = 5;
// SN74141 pins (2):
int ledPin_1_a = 6;                
int ledPin_1_b = 7;
int ledPin_1_c = 8;
int ledPin_1_d = 9;
// anode pins:
int ledPin_a_1 = 10;
int ledPin_a_2 = 11;
int ledPin_a_3 = 12;
int ledPin_a_4 = 13;

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
int invR_button = 33; int invR_LED = 48;
int mono_button = 34; int mono_LED = 49;
int muteR_button = 35; int muteR_LED = 50;
int muteL_button = 36; int muteL_LED = 51;

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

// Input and Speaker selection variables:
int InOutAddress = 0;                       // EEPROM storage address
int InOut = EEPROM.read(InOutAddress);      // CMD_INOUT value for RS485, read from EEPROM
int previousInOut = InOut;                  // previous value for comparison within loop
int inSelect = InOut & B00000111;           // stored input selection (0-5)
int spkSelect = (InOut & B00011000) >> 3;   // stored speaker selection (0-3)
int previousInSelect = inSelect;            // previous value
int previousSpkSelect = spkSelect;          // previous value

// Mute and other functions:
boolean muteL = 1;
boolean muteR = 1;
boolean mono = 0;
boolean invR = 0;
boolean dim = 0;
int functions = B10000000 + (muteL << 5) + (muteR << 4) + (mono << 3) + (invR << 2) + dim;
int previousFunctions = functions;
boolean muteL_button_status = 0;
boolean muteR_button_status = 0;
boolean mono_button_status = 0;
boolean invR_button_status = 0;
boolean dim_button_status = 0;

// Volume read and smoothing:
// Define the number of samples to keep track of.  The higher the number,
// the more the readings will be smoothed, but the slower the output will
// respond to the input.  Using a constant rather than a normal variable lets
// use this value to determine the size of the readings array:
const int numReadings = 10;
int volumeInputPin = A0;            // the volume pot input pin
int volumeReadings[numReadings];    // the readings from the analog input
int readIndex = 0;                  // the index of the current reading
int volumeTotal = 0;                // the running total
int volume = 0;                     // the averaged volume level
int previousVolume = 0;             // the stored previous level
int displayVolume = 0;              // the volume formatted for Nixie display

// RS485 serial pins:
int SSerialRX = 19;          // Serial Receive pin (Receive Out) - Not connected
int SSerialTX = 20;          // Serial Transmit pin (Data Input)
int SSerialTxControl = 21;   // RS485 Direction Control pin (Data Enable)
// Object declaration:
SoftwareSerial RS485Serial(SSerialRX, SSerialTX); // RX, TX

void setup() 
{

  // Nixie driver pin configuration:
  pinMode(ledPin_0_a, OUTPUT);
  pinMode(ledPin_0_b, OUTPUT);
  pinMode(ledPin_0_c, OUTPUT);
  pinMode(ledPin_0_d, OUTPUT);
  
  pinMode(ledPin_1_a, OUTPUT);
  pinMode(ledPin_1_b, OUTPUT);
  pinMode(ledPin_1_c, OUTPUT);
  pinMode(ledPin_1_d, OUTPUT);
  
  pinMode(ledPin_a_1, OUTPUT);
  pinMode(ledPin_a_2, OUTPUT);
  pinMode(ledPin_a_3, OUTPUT);
  pinMode(ledPin_a_4, OUTPUT);

  // Button pin configuration:
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
  pinMode(invR_button, INPUT_PULLUP);
  pinMode(mono_button, INPUT_PULLUP);
  pinMode(muteR_button, INPUT_PULLUP);
  pinMode(muteL_button, INPUT_PULLUP);
  
  // LED pin configuration:
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
  pinMode(invR_LED, OUTPUT);
  pinMode(mono_LED, OUTPUT);
  pinMode(muteR_LED, OUTPUT);
  pinMode(muteL_LED, OUTPUT);

  // RS485 pin configuration:
  pinMode(SSerialTxControl, OUTPUT);
  digitalWrite(SSerialTxControl, HIGH);

  // Start the RS485 software serial port:
  RS485Serial.begin(9600);

  // Start the USB serial monitor port:
  Serial.begin(9600);
  
  // Initialize all volume readings to 0:
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    volumeReadings[thisReading] = 0;
  }

  // Run startup marquee effect:
  for (int marquee = in1_LED; marquee <=muteL_LED; marquee++)   // Left to right
  {
    digitalWrite(marquee, HIGH);
    delay(60);
    digitalWrite(marquee, LOW);
  }
  for (int marquee = muteR_LED; marquee >=in1_LED; marquee--)   // Right to left
  {
    digitalWrite(marquee, HIGH);
    delay(60);
    digitalWrite(marquee, LOW);
  }

//  Serial.print("stored input: ");      // Print to serial monitor
//  Serial.println(inSelect + 1);        // Print to serial monitor
//  Serial.print("stored speaker: ");    // Print to serial monitor
//  Serial.println(spkSelect + 1);       // Print to serial monitor

  // Set Input and Speaker LEDs according to stored InOut value:
  digitalWrite((in1_LED + inSelect), HIGH);
  digitalWrite((spk1_LED + spkSelect), HIGH);

  // Set initial monitor functions (mute L and mute R):
  RS485Serial.write(functions);
  digitalWrite(muteL_LED, muteL);  
  digitalWrite(muteR_LED, muteR);  
//  Serial.print("functions: ");           // Print to serial monitor
//  Serial.println(functions, BIN);        // Print to serial monitor 

}

////////////////////////////////////////////////////////////////////////
//
// DisplayNumberSet
// Use: Passing anod number, and number for Tube 1 and Tube 2, this function
//      looks up the truth table and opens the correct outputs from the arduino
//      to light the numbers given to this funciton (num1,num2).
//      On a 4 nixie bulb setup.
//
////////////////////////////////////////////////////////////////////////
void DisplayNumberSet( int anod, int num1, int num2 )
{
  int anodPin;
  int a,b,c,d;
  
  // set defaults.
  a=0;b=0;c=0;d=0;           // will display a zero.
  anodPin =  ledPin_a_1;     // default on first anod.
  //
  // Note: Anodes are marked one-relative on the PCB board instead of zero-relative here.
  // 
  // Select what anod to fire.
  switch( anod )  
  {
    case 0:    anodPin =  ledPin_a_1;    break; // Anode marked "1" on the PCB 
    case 1:    anodPin =  ledPin_a_2;    break; // Anode marked "2" on the PCB
    case 2:    anodPin =  ledPin_a_3;    break; // Anode marked "3" on the PCB
    case 3:    anodPin =  ledPin_a_4;    break; // Anode marked "4" on the PCB
  }  
  
  // Load the a,b,c,d.. to send to the SN74141 IC (1)
 switch( num1 )
  {
    case 0: a=0;b=0;c=0;d=0;break; // pin 10
    case 1: a=1;b=0;c=0;d=0;break; // pin 1
    case 2: a=0;b=1;c=0;d=0;break; // pin 2
    case 3: a=1;b=1;c=0;d=0;break; // pin 3
    case 4: a=0;b=0;c=1;d=0;break; // pin 4
    case 5: a=1;b=0;c=1;d=0;break; // pin 5
    case 6: a=0;b=1;c=1;d=0;break; // pin 6
    case 7: a=1;b=1;c=1;d=0;break; // pin 7
    case 8: a=0;b=0;c=0;d=1;break; // pin 8
    case 9: a=1;b=0;c=0;d=1;break; // pin 9
    default: break;  // used to no-op the number in the array
  }  

  // Write to output pins.
  digitalWrite(ledPin_0_d, d);
  digitalWrite(ledPin_0_c, c);
  digitalWrite(ledPin_0_b, b);
  digitalWrite(ledPin_0_a, a);

  // Load the a,b,c,d.. to send to the SN74141 IC (2)
  switch( num2 )
  {
    case 0: a=0;b=0;c=0;d=0;break;
    case 1: a=1;b=0;c=0;d=0;break;
    case 2: a=0;b=1;c=0;d=0;break;
    case 3: a=1;b=1;c=0;d=0;break;
    case 4: a=0;b=0;c=1;d=0;break;
    case 5: a=1;b=0;c=1;d=0;break;
    case 6: a=0;b=1;c=1;d=0;break;
    case 7: a=1;b=1;c=1;d=0;break;
    case 8: a=0;b=0;c=0;d=1;break;
    case 9: a=1;b=0;c=0;d=1;break;
    default: break;
  }

  // Write to output pins
  digitalWrite(ledPin_1_d, d);
  digitalWrite(ledPin_1_c, c);
  digitalWrite(ledPin_1_b, b);
  digitalWrite(ledPin_1_a, a);

  // Turn on this anod.
  digitalWrite(anodPin, HIGH);   

  // Delay
  // NOTE: With the differnce in Nixie bulbs you may have to change
  //       this delay to set the update speed of the bulbs. If you 
  //       dont wait long enough the bulb will be dim or not light at all
  //       you want to set this delay just right so that you have 
  //       nice bright output yet quick enough so that you can multiplex with
  //       more bulbs.
  delay (10);
  
  // Shut off this anod.
  digitalWrite(anodPin, LOW);
}

////////////////////////////////////////////////////////////////////////
void DisplayNumberString( int* array )
{
  // bank 1 (bulb 0,3)
  DisplayNumberSet(0,array[0],array[3]);   
  // bank 2 (bulb 1,2)
  DisplayNumberSet(1,array[1],array[2]);
}
////////////////////////////////////////////////////////////////////////

void loop()     
{
 
  // Input select:
  if (digitalRead(in1_button) == LOW && in1_button_status == 0)  // input 1 button is pressed
  {
    delay(20);                         // delay to debounce
    inSelect = 0;                      // set input 1
    in1_button_status = 1;             // set button status
  }
  if (digitalRead(in1_button) == HIGH && in1_button_status == 1)
  {
    in1_button_status = 0;             // reset status
  }

  if (digitalRead(in2_button) == LOW && in2_button_status == 0)  // input 2 button is pressed
  {
    delay(20);                         // delay to debounce
    inSelect = 1;                      // set input 2
    in2_button_status = 1;             // set button status
  }
  if (digitalRead(in2_button) == HIGH && in2_button_status == 1)
  {
    in2_button_status = 0;             // reset status
  }

  if (digitalRead(in3_button) == LOW && in3_button_status == 0)  // input 3 button is pressed
  {
    delay(20);                         // delay to debounce
    inSelect = 2;                      // set input 3
    in3_button_status = 1;             // set button status
  }
  if (digitalRead(in3_button) == HIGH && in3_button_status == 1)
  {
    in3_button_status = 0;             // reset status
  }

  if (digitalRead(in4_button) == LOW && in4_button_status == 0)  // input 4 button is pressed
  {
    delay(20);                         // delay to debounce
    inSelect = 3;                      // set input 4
    in4_button_status = 1;             // set button status
  }
  if (digitalRead(in4_button) == HIGH && in4_button_status == 1)
  {
    in4_button_status = 0;             // reset status
  }

  if (digitalRead(in5_button) == LOW && in5_button_status == 0)  // input 5 button is pressed
  {
    delay(20);                         // delay to debounce
    inSelect = 4;                      // set input 5
    in5_button_status = 1;             // set button status
  }
  if (digitalRead(in5_button) == HIGH && in5_button_status == 1)
  {
    in5_button_status = 0;             // reset status
  }

  if (digitalRead(in6_button) == LOW && in6_button_status == 0)  // input 6 button is pressed
  {
    delay(20);                         // delay to debounce
    inSelect = 5;                      // set input 6
    in6_button_status = 1;             // set button status
  }
  if (digitalRead(in6_button) == HIGH && in6_button_status == 1)
  {
    in6_button_status = 0;             // reset status
  }

  // Speaker select:
  if (digitalRead(spk1_button) == LOW && spk1_button_status == 0)  // speaker 1 button is pressed
  {
    delay(20);                          // delay to debounce
    spkSelect = 0;                      // set speaker 1
    spk1_button_status = 1;             // set button status
  }
  if (digitalRead(spk1_button) == HIGH && spk1_button_status == 1)
  {
    spk1_button_status = 0;             // reset status
  }

  if (digitalRead(spk2_button) == LOW && spk2_button_status == 0)  // speaker 2 button is pressed
  {
    delay(20);                          // delay to debounce
    spkSelect = 1;                      // set speaker 2
    spk2_button_status = 1;             // set button status
  }
  if (digitalRead(spk2_button) == HIGH && spk2_button_status == 1)
  {
    spk2_button_status = 0;             // reset status
  }

  if (digitalRead(spk3_button) == LOW && spk3_button_status == 0)  // speaker 3 button is pressed
  {
    delay(20);                          // delay to debounce
    spkSelect = 2;                      // set speaker 3
    spk3_button_status = 1;             // set button status
  }
  if (digitalRead(spk3_button) == HIGH && spk3_button_status == 1)
  {
    spk3_button_status = 0;             // reset status
  }

  if (digitalRead(spk4_button) == LOW && spk4_button_status == 0)  // speaker 4 button is pressed
  {
    delay(20);                          // delay to debounce
    spkSelect = 3;                      // set speaker 4
    spk4_button_status = 1;             // set button status
  }
  if (digitalRead(spk4_button) == HIGH && spk4_button_status == 1)
  {
    spk4_button_status = 0;             // reset status
  }

  // Calculate InOut value:
  InOut = B01000000 + inSelect + (spkSelect << 3);

  // If input or output are changed, send to RS485 and update:
  if (InOut != previousInOut)
  {
    RS485Serial.write(InOut);                    // Send to RS485
//    Serial.print("input select: ");              // Print to serial monitor
//    Serial.println(inSelect + 1);                // Print to serial monitor
//    Serial.print("speaker select: ");            // Print to serial monitor
//    Serial.println(spkSelect + 1);               // Print to serial monitor
    if (inSelect != previousInSelect)                     // If the input select is different:
    {
      digitalWrite((in1_LED + inSelect), HIGH);           // Turn on new input LED
      digitalWrite((in1_LED + previousInSelect), LOW);    // Turn off previous input LED
      previousInSelect = inSelect;                        // Update
    }
    if (spkSelect != previousSpkSelect)                   // If the speaker select is different:
    {
      digitalWrite((spk1_LED + spkSelect), HIGH);         // Turn on new speaker LED
      digitalWrite((spk1_LED + previousSpkSelect), LOW);  // Turn off previous speaker LED
      previousSpkSelect = spkSelect;                      // Update      
    }   
    previousInOut = InOut;                  // Update previous value
    EEPROM.update(InOutAddress, InOut);     // Update value to EEPROM
  }

  // Mute and other functions:
  if (digitalRead(muteL_button) == LOW && muteL_button_status == 0)   // mute L button is pressed
  {
    delay(20);                           // delay to debounce
    muteL = !muteL;                      // toggle mute L
    muteL_button_status = 1;             // set button status
  }
  if (digitalRead(muteL_button) == HIGH && muteL_button_status == 1)
  {
    muteL_button_status = 0;             // reset status
  }

  if (digitalRead(muteR_button) == LOW && muteR_button_status == 0)   // mute R button is pressed
  {
    delay(20);                            // delay to debounce
    muteR = !muteR;                       // toggle mute R
    muteR_button_status = 1;              // set button status
  }
  if (digitalRead(muteR_button) == HIGH && muteR_button_status == 1)
  {
    muteR_button_status = 0;              // reset status
  }

  if (digitalRead(mono_button) == LOW && mono_button_status == 0)    // mono button is pressed
  {
    delay(20);                            // delay to debounce
    mono = !mono;                         // toggle mono
    mono_button_status = 1;               // set button status
  }
  if (digitalRead(mono_button) == HIGH && mono_button_status == 1)
  {
    mono_button_status = 0;               // reset status
  }

  if (digitalRead(invR_button) == LOW && invR_button_status == 0)   // invert R button is pressed
  {
    delay(20);                            // delay to debounce
    invR = !invR;                         // toggle invert R
    invR_button_status = 1;               // set button status
  }
  if (digitalRead(invR_button) == HIGH && invR_button_status == 1)
  {
    invR_button_status = 0;               // reset status
  }

  if (digitalRead(dim_button) == LOW && dim_button_status == 0)    // dim button is pressed
  {
    delay(20);                            // delay to debounce
    dim = !dim;                           // toggle dim
    dim_button_status = 1;                // set button status
  }
  if (digitalRead(dim_button) == HIGH && dim_button_status == 1)
  {
    dim_button_status = 0;                // reset status
  }
  
  // Calculate functions value:
  functions = B10000000 + (muteL << 5) + (muteR << 4) + (mono << 3) + (invR << 2) + dim;

  // If functions have changed, send command to RS485 and toggle LEDs:
  if (functions != previousFunctions)
  { 
    RS485Serial.write(functions);
//    Serial.print("functions: ");           // Print to serial monitor
//    Serial.println(functions, BIN);        // Print to serial monitor 
    digitalWrite(muteL_LED, muteL);        // Toggle LEDs
    digitalWrite(muteR_LED, muteR);
    digitalWrite(mono_LED, mono);
    digitalWrite(invR_LED, invR);
    digitalWrite(dim_LED, dim);
    previousFunctions = functions;         // Update
  }

  // Read and smooth volume pot value:
  volumeTotal = volumeTotal - volumeReadings[readIndex];   // subtract the last reading
  volumeReadings[readIndex] = analogRead(volumeInputPin);  // read from the sensor
  volumeTotal = volumeTotal + volumeReadings[readIndex];   // add the reading to the total
  readIndex = readIndex + 1;                               // advance to the next position in the array
  if (readIndex >= numReadings)                            // at the end of the array...
  { 
    readIndex = 0;                                         // wrap around to the beginning
  }
  volume = (volumeTotal / numReadings) / 16;               // calculate the average and scale:
//  delay(4);                                                // delay in between reads for stability

  // format for Nixie display:
  displayVolume = map(volume, 0, 63, 63, 0);               // MC624 volume display setting
  if (dim == 1)
  {
    displayVolume = displayVolume + 20;                    // MC624 dim setting
  }
  // separate into digits for Nixie tubes:
  int volumeOnes = (displayVolume%10);
  int volumeTens = ((displayVolume/10)%10);
  // Fill in the Number array for the Nixie tubes:
  int NumberArray[4];
  NumberArray[0] = (inSelect + 1);         // Selected input
  NumberArray[1] = volumeTens;             // Current volume - tens digit
  NumberArray[2] = volumeOnes;             // Current volume - ones digit
  if (muteL == 1 && muteR == 1)            // If MUTE L and R are on...
  {
    NumberArray[3] = 0;                    // ...speaker 0...
  }
  else
  {
    NumberArray[3] = (spkSelect + 1);      // ...otherwise selected speaker
  }
  // Display on the Nixie tubes:
  DisplayNumberString( NumberArray );

  // If volume level is changed, send to RS485 and update:
  if (volume != previousVolume)
  {
    RS485Serial.write(volume);
    previousVolume = volume;
//    Serial.print("volume: ");      // Print to serial monitor
//    Serial.println(volume);        // Print to serial monitor
  }

}
