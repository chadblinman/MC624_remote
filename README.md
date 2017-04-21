# MC624_remote
An Arduino Mega-based desktop hardware remote for the SoundSkulptor MC624 monitor controller.

The remote is built on an Arduino Mega 2560 -- overkill in memory and processing but chosen for its many I/O pins, so I could develop the code simply without complex multiplexing.

The remote uses 15 normally open, momentary button switches with independently controlled LED lamps.  The ones I am using may be found here: http://www.ebay.com/sch/m.html?_odkw=illuminated+momentary+12MM+6V&_ssn=paer-intl&_osacat=0&_from=R40&_trksid=p2046732.m570.l1313.TR0.TRC0.H0.Xilluminated+momentary+12MM+6V+square.TRS1&_nkw=illuminated+momentary+12MM+6V+square&_sacat=0 
I chose six yellow buttons for the input selectors, four blue for speaker output selectors, and five red for the function buttons.  

Volume is controlled by a 10K linear taper analog potentiometer which is sampled, averaged for smoothing and scaled to the MC624's 64 steps of attenuation.

Input, output and attenuation levels are indicated on IN-2 Nixie tubes, driven by the Arduinix shield: http://www.arduinix.com/

Interface to the main unit is via RS485 using the interface board found here:  https://arduino-info.wikispaces.com/RS485-Modules

I am new to Arduino programming, so this code may be wildly inefficient...but it works.
