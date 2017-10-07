# MC624_remote
An Arduino-based desktop hardware remote for the SoundSkulptor MC624 monitor controller (http://www.soundskulptor.com/uk/mc624.html).

The remote is built on an Arduino Mega2560 –– overqualified in terms of memory and processing but chosen for its abundance of I/O pins, so I could develop simply without multiplexing for the 15 buttons and indicators.

Input (source), output (speaker), and volume (attenuation) levels are indicated on IN-2 Nixie tubes, driven by an Arduinix shield:  https://www.tindie.com/products/Nixiekeith/arduinix-nixie-driver-shield/  (see also http://www.arduinix.com/)

The remote interfaces to the main unit via RS485, using the interface board found here:  https://arduino-info.wikispaces.com/RS485-Modules

The remote uses 15 normally open, momentary button switches with independently controlled LED lamps.  The ones I am using may be found here: http://www.ebay.com/sch/m.html?_odkw=illuminated+momentary+12MM+6V&_ssn=paer-intl&_osacat=0&_from=R40&_trksid=p2046732.m570.l1313.TR0.TRC0.H0.Xilluminated+momentary+12MM+6V+square.TRS1&_nkw=illuminated+momentary+12MM+6V+square&_sacat=0  

Volume is controlled by a 10K linear taper analog potentiometer (I used this Bourns pot:  http://www.mouser.com/Search/ProductDetail.aspx?R=PDA241-HRT02-103B0virtualkey65210000virtualkey652-PDA241HRT02103B0
).  Voltage is sampled on an analog input, averaged for smoothing and scaled to the MC624's 64 steps of attenuation.

A single cable connects to the remote control unit, splitting at the opposite end to connect to a power supply (+9VDC to the Arduino), to the MC624's LINK I/O using a standard USB connector, and to a computer via USB for Arduino programming.  The MC624 is switched to SLAVE mode to respond to commands received via the LINK input.

The remote code essentially does two things:  It continuously indicates its own local status, and it transmits serial commands to the main unit whenever the user changes a setting.  

The serial connection is unidirectional, so the remote's status is not updated by status of the main unit.  When the user operates the main unit's front panel, it works normally but the remote will not indicate any change.  The next command from the remote will then override.  As long as operation happens at the remote, its local status will always be in parity with the main unit.  Since the main unit's input and output selections persist on power-down, the remote also updates its input/output status to EEPROM whenever it is changed by the user –– so on power-up the remote and the main unit will be in parity.

The panels are designed in Front Panel Designer software.  The final design files are provided here for documentation and may be used for production by Front Panel Express (http://www.frontpanelexpress.com/) only.  Dimensions of my enclosure are also provided here in PDF format, which if printed at 100% scale may be used as a cutting template.

I documented the design and build process in more detail here:  http://www.chadblinman.com/making-the-mc624-remote/
