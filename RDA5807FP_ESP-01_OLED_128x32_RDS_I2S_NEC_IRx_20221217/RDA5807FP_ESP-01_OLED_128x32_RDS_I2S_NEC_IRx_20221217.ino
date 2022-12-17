//* I2S OUTPUT FM TUNER *//
// TUNER: RDA7088 
// CPU: ESP-01 or ESP-01S 
// OLED: 128x32 or 128x64
// IR: NEC 38kHz 21 Keys
// OUTPUT: I2S (16bits upto 48kHz) 
// I2S DAC: PCM5102A or MAX98357A or UDA1334A or PCM5100
// INPUT: 5V or 3.7V Li
// VERSION: 1.0 // 13th DEC 2022 //
// ESP-01 CONNECTIONs //
// IO0 - SDA
// IO2 - SCL
// RX  - IR Rx I/P
// RST - 10kΩ-/\/\/\- 3V3
// EN  - 12kΩ-/\/\/\- 3V3 
// 3V3 - 3V3
// GND - GND
// TX  - 4.7kΩ-/\/\/\- 3V3 [UNUSED]-AVAILABLE FOR USE
#include <Wire.h>
#include <radio.h> 
#include <RDA5807M.h> 
#include <RDSParser.h> 
#include <IRremote.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#define I2C_ADDRESS 0x3C // make sure your OLED'S I2C address is correct

#define RST_PIN -1  // Define proper RST_PIN if required.

RDA5807M radio;    
RDSParser rds; 
SSD1306AsciiWire oled;

/// State definition for this radio implementation.
enum RADIO_STATE {
  STATE_PARSECOMMAND, //<- waiting for a new command character.
  STATE_PARSEINT,     //<- waiting for digits for the parameter.
  STATE_EXEC          //<- executing the command.
                  };
RADIO_STATE state; //< The state variable is used for parsing input characters.

int RECV_PIN = 3; //Connect the IR data to RX pin(IO3) of ESP-01 // Rx pin // No UART possible
IRrecv irrecv(RECV_PIN);
decode_results results;

int volume = 2;  // start with the volume low // NOT APPLICABLE FOR I2S
int channellist[]= {911,919,927,935,943,950,983,1001,1013,1029,1030,1040,1064}; // Define some radio stations
int maxc = 12;   // Define the number of radio stations used [0..n]
int channel = 1; // Start with a favorite station [0,1,2,3,4,..]

// Update the ServiceName text on the LCD display.
void DisplayServiceName(char *name)
{
    oled.clearToEOL();
    oled.print(" ");
    oled.println(name); 
}   

void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
                 rds.processData(block1, block2, block3, block4);
}

void setup()
{
  Wire.begin(0,2); // Assign ESP-01 I2C // SDA-IO0, SCL-IO2 

#if RST_PIN >= 0
  oled.begin(&Adafruit128x32, I2C_ADDRESS, RST_PIN);
#else // RST_PIN >= 0
  oled.begin(&Adafruit128x32, I2C_ADDRESS);
#endif // RST_PIN >= 0

  oled.setFont(fixed_bold10x15); 
  oled.clear();

  irrecv.enableIRIn(); // Start the receiver
 
  radio_init();
  setfreq(channellist[channel]);
  setvolume(volume);
  radio.attachReceiveRDS(RDS_process);
  rds.attachServicenNameCallback(DisplayServiceName);
}
void loop() {
if (irrecv.decode(&results)) {
 
switch (results.value) {
case 0x1FE609F: //NEC VOLUME +

  if (volume < 15) {
      volume++;
      setvolume(volume);
    }
break;

case 0x1FEA05F: //NEC VOLUME -

  if (volume > 0) {
      volume--;
      setvolume(volume);
    }
break;

case 0x1FEC03F: //NEC CH +

  if (channel < maxc) {
      channel++;
      setfreq(channellist[channel]);
    }
break;

case 0x1FE40BF: //NEC CH -

  if (channel > 0) {
      channel--;
      setfreq(channellist[channel]);
    }
break;
default:
//oled.clear();
  delay(100); //check CASE DEFAULT syntax...
}
  irrecv.resume(); // Receive the next value
}
  delay(100);

//some internal static values for parsing the input
  static char command;
  static int16_t value;
  static RADIO_FREQ lastf = 0;
  RADIO_FREQ f = 0;

  radio.checkRDS();   // check for RDS data
}
void setvolume(int thevolume)
{
byte volbyte;
 
volbyte = thevolume + 0xD0;
  Wire.beginTransmission(0x11);
  Wire.write(0x05);
  Wire.write(0x84); Wire.write(volbyte);
  Wire.endTransmission();
  delay(500);
  
    oled.clear();
    oled.print(channellist[channel]/10); oled.print("."); oled.print((channellist[channel]) % 10); oled.print(" ");
    oled.print("VOL:"); oled.print(volume); oled.println("  ");
}

void setfreq(int thefreq)
{
int freqB;
byte freqH, freqL;
 
  freqB = thefreq - 870;
  freqH = freqB >> 2;
  freqL = (freqB & 3) <<6;
 
  Wire.beginTransmission(0x11);
  Wire.write(0x03);
  Wire.write(freqH); // write frequency into bits 15:6, set tune bit
  Wire.write(freqL + 0x10);
  Wire.endTransmission();
  delay(500);

      oled.clear();
      oled.print(channellist[channel]/10); oled.print("."); oled.print((channellist[channel]) % 10); oled.print(" "); 
      oled.print("VOL:"); oled.print(volume); oled.println("  ");  
}

void radio_init()
{
// RESET 
   Wire.beginTransmission(0x11); // Device address 0x11 (random access)
   Wire.write(0x02); // Register address 0x02
   Wire.write(0xC0); Wire.write(0x03); // Initialize the settings
   Wire.endTransmission();
   delay(500); // wait 500ms to finalize setup
// INIT 
   Wire.beginTransmission(0x11);     // Device address 0x11 (random access)
   Wire.write(0x02);
   Wire.write(0xC0); Wire.write(0x0D); // Setup radio settings
   Wire.endTransmission();
// REG:0x04H // BIT 6 Enables I2S
   Wire.beginTransmission(0x11);
   Wire.write(0x04);           // REG(04H)
   Wire.write(0B00001000);     // [11] DE - [De-emphasis] 0 = 75 μs; 1 = 50 μs
   Wire.write(0B01000000);     // [06] [I2S_ENABLED] 1- Enabled
//   Wire.write(0B00000000);     // [06] [I2S_ENABLED] 0- Disabled
   Wire.endTransmission();
// REG:0x06H // SET DATA_SIGNED & I2S_SW_CNT
   Wire.beginTransmission(0x11);
   Wire.write(0x06);           // REG(06H)
   Wire.write(0B00000010);     // [09] [DATA_SIGNED] 1- I2S output signed 16-bit audio data
   Wire.write(0B10000000);     // [7:4] I2S_SW_CNT[4:0] 48kbps
   Wire.endTransmission();
// INIT DONE!
   delay(500); // wait 500ms to finalize settings
}
// Arduino 1.8.19
// Executable segment sizes:
// ICACHE : 32768           - flash instruction cache 
// IROM   : 247924          - code in flash         (default or ICACHE_FLASH_ATTR) 
// IRAM   : 28089   / 32768 - code in IRAM          (IRAM_ATTR, ISRs...) 
// DATA   : 1572  )         - initialized variables (global, static) in RAM/HEAP 
// RODATA : 3408  ) / 81920 - constants             (global, static) in RAM/HEAP 
// BSS    : 26992 )         - zeroed variables      (global, static) in RAM/HEAP 
// Sketch uses 280993 bytes (29%) of program storage space. Maximum is 958448 bytes.
// Global variables use 31972 bytes (39%) of dynamic memory, leaving 49948 bytes for local variables. Maximum is 81920 bytes.
