/*
  
  DEFCON China 1.0 Official Badge (2019)
                                                         
  Author: Joe Grand, Grand Idea Studio [@joegrand]             
  
  Program Description:
  
  This program contains the firmware for the DEFCON China 1.0 official badge. 
  
  Along with being a standard Arduino platform based on an Arduino Pro Mini 
  running on a CR2032 3V Lithium coin cell battery, the main purpose at the 
  conference is to be used as part of a game where attendees have to complete 
  certain tasks in order to unlock LEDs on their badge. When a task is 
  completed, the badge's FPC (flexible printed circuit) connector will be 
  plugged into a "programming station" (Arduino plus the DEFCON FPC-to-
  Arduino Shield hardware) to set the specific LED. Communication between 
  badge and programming station is via UART. 

  The badge also contains some hardware- and firmware-related "mystery" tasks.
  If you dig through this source code, you should be able to work out what
  they are :)
  
  Complete design documentation can be found at:
  http://www.grandideastudio.com/portfolio/defcon-china-2019-badge
  
*/

/***************************************************************************
 *********************** ATmega328P Fuse Settings **************************
 ***************************************************************************/

// Device: Arduino Pro Mini (3.3V, 8 MHz) w/ ATmega328P
// Bootloader: atmega/ATmegaBOOT_168_atmega328_pro_8MHz.hex

// Extended Fuse: 0xFE = 0b11111110
// BODLEVEL: Brown Out Detector Trigger Level: 110 (Vbot minimum = 1.7V, typical = 1.8V, maximum = 2.0V)

// High Fuse: 0xD2 = 0b11010010
// RSTDISBL: External Reset Disabled: 1 (Not Disabled)
// DWEN: debugWIRE Enable: 1 (Not Enabled)
// SPIEN: Enable Serial Program and Data Downloading: 0 (SPI Programming Enabled)
// WDTON: Watchdog Timer Always On: 1 (Not Always On)
// EESAVE: Preserve EEPROM memory through Chip Erase: 0 (EEPROM Preserved)
// BOOTSZ: Boot Size: 01 (1024 Words, 16 Pages, Application Flash Section 0x0-0x3BFF, Boot Reset Address 0x3C00)
// BOOTRST: Select Reset Vector: 0 (Jump to Boot Reset Address)

// Low Fuse: 0xFF = 0b11111111
// CKDIV8: Divide Clock by 8: 1 (Disabled)
// CKOUT: Output System Clock on Port B0: 1 (Disabled)
// SUT: Start Up Time: 11 (Crystal Oscillator, Slowly Rising Power, Start-up Time from Power Down = 16K clock cycles, Reset Delay = ~65mS)
// CKSEL: Clock Source Select: 1111 (Low Power Crystal Oscillator, 8-16MHz, 12-22pF Capacitors)

// Lock Bit Byte: 0xCF = 0b11001111
// BLB12-11: Boot Lock Bit: 00 (Boot Loader Section Protected)
// BLB02-01: Boot Lock Bit: 11 (No Restrictions for Application Section)
// LB: Lock Bit: 11 (No Memory Lock Features Enabled)

#include <avr/boot.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <LowPower.h>         // https://github.com/rocketscream/Low-Power
#include <Adafruit_Sensor.h>  // https://github.com/adafruit/Adafruit_Sensor
#include <Adafruit_LIS3DH.h>  // https://github.com/adafruit/Adafruit_LIS3DH
#include "LED_Matrix.h"       // https://github.com/marcmerlin/LED-Matrix

/***************************************************************************
 ************************** Pin Assignments ********************************
 ***************************************************************************/

#define flexTxPin 8  // Serial output (via FPC connector)
#define flexRxPin 9  // Serial input (via FPC connector)

#define nUSBPowerDetect A1        // LOW if FT231X has properly enumerated to host PC or connected to Dedicated Charging Port
#define nBatteryChargerDetect A0  // LOW if connected via USB to a Dedicated Charging Port

#define accelInt 2     // External wake-up pin from accelerometer, INT0
#define nFlexSense 3   // External wake-up pin from FPC, INT1
#define nSolderBlob 6  // GPIO pin to detect existence of solder blob between ATmega328P physical pin 9 & 10
#define flexGPIO 7     // GPIO pin (via FPC connector), unused

// Connections to 74HC595 shift register for LED matrix control
// The LED-Matrix library uses Fast I/O functions (https://www.codeproject.com/Articles/732646/Fast-digital-I-O-for-Arduino)
// Pin numbers must use special data type DP instead of just an integer
#define ledData DP11   // Data
#define ledLatch DP12  // Latch
#define ledClk DP13    // Clock
#define ledRow0 DP16   // A2
#define ledRow1 DP17   // A3
#define ledRow2 DP5
#define ledRow3 DP4
#define nLedOE 10  // Output Enable

/***************************************************************************
 **************************** Definitions **********************************
 ***************************************************************************/

#define __DEBUG
#define ATTENDED_BADGE_HACKING_WORKSHOP true

#define EEBaseAddress 0     // starting address within EEPROM for our data storage
#define LIS3DHAddress 0x19  // alternate I2C address

#define DCN_UART_START '<'  // start byte (sent from FPC to Arduino Shield)
#define DCN_UART_END '>'    // end byte

#define ACCEL_TIMEOUT 10000  // time (in mS) of no motion before badge goes to sleep

#define DirectMatrix_ISR_FREQ 175  // refresh frequency for LED matrix driver

// desired brightness for the particular LED color (16 levels)
// different LED colors have different Vf, which affects brightness
// we want all LEDs to visually appear at the same brightness regardless of color
#define LED_BRIGHTNESS_RED 2
#define LED_BRIGHTNESS_ORANGE 6
#define LED_BRIGHTNESS_YELLOW 6
#define LED_BRIGHTNESS_GREEN 12

// map logical LED to actual point on the matrix and set brightness
#define LED_ROOT_1A \
  point { \
    .x = 0, .y = 3, .b = LED_BRIGHTNESS_RED \
  }
#define LED_ROOT_1B \
  point { \
    .x = 0, .y = 2, .b = LED_BRIGHTNESS_RED \
  }
#define LED_ROOT_1C \
  point { \
    .x = 0, .y = 1, .b = LED_BRIGHTNESS_ORANGE \
  }
#define LED_ROOT_1D \
  point { \
    .x = 0, .y = 0, .b = LED_BRIGHTNESS_YELLOW \
  }

#define LED_ROOT_2A \
  point { \
    .x = 1, .y = 3, .b = LED_BRIGHTNESS_RED \
  }
#define LED_ROOT_2B \
  point { \
    .x = 1, .y = 2, .b = LED_BRIGHTNESS_RED \
  }
#define LED_ROOT_2C \
  point { \
    .x = 1, .y = 1, .b = LED_BRIGHTNESS_ORANGE \
  }
#define LED_ROOT_2D \
  point { \
    .x = 1, .y = 0, .b = LED_BRIGHTNESS_YELLOW \
  }

#define LED_ROOT_3A \
  point { \
    .x = 2, .y = 3, .b = LED_BRIGHTNESS_RED \
  }
#define LED_ROOT_3B \
  point { \
    .x = 2, .y = 2, .b = LED_BRIGHTNESS_RED \
  }
#define LED_ROOT_3C \
  point { \
    .x = 2, .y = 1, .b = LED_BRIGHTNESS_ORANGE \
  }
#define LED_ROOT_3D \
  point { \
    .x = 2, .y = 0, .b = LED_BRIGHTNESS_YELLOW \
  }

#define LED_ROOT_4A \
  point { \
    .x = 3, .y = 3, .b = LED_BRIGHTNESS_RED \
  }
#define LED_ROOT_4B \
  point { \
    .x = 3, .y = 2, .b = LED_BRIGHTNESS_RED \
  }
#define LED_ROOT_4C \
  point { \
    .x = 3, .y = 1, .b = LED_BRIGHTNESS_ORANGE \
  }
#define LED_ROOT_4D \
  point { \
    .x = 3, .y = 0, .b = LED_BRIGHTNESS_YELLOW \
  }

#define LED_BRANCH_1A \
  point { \
    .x = 7, .y = 3, .b = LED_BRIGHTNESS_GREEN \
  }
#define LED_BRANCH_1B \
  point { \
    .x = 7, .y = 2, .b = LED_BRIGHTNESS_GREEN \
  }
#define LED_BRANCH_1C \
  point { \
    .x = 7, .y = 1, .b = LED_BRIGHTNESS_GREEN \
  }
#define LED_BRANCH_1D \
  point { \
    .x = 7, .y = 0, .b = LED_BRIGHTNESS_GREEN \
  }

#define LED_BRANCH_2A \
  point { \
    .x = 6, .y = 3, .b = LED_BRIGHTNESS_GREEN \
  }
#define LED_BRANCH_2B \
  point { \
    .x = 6, .y = 2, .b = LED_BRIGHTNESS_GREEN \
  }
#define LED_BRANCH_2C \
  point { \
    .x = 6, .y = 1, .b = LED_BRIGHTNESS_GREEN \
  }
#define LED_BRANCH_2D \
  point { \
    .x = 6, .y = 0, .b = LED_BRIGHTNESS_GREEN \
  }

#define LED_BRANCH_3A \
  point { \
    .x = 5, .y = 3, .b = LED_BRIGHTNESS_GREEN \
  }
#define LED_BRANCH_3B \
  point { \
    .x = 5, .y = 2, .b = LED_BRIGHTNESS_GREEN \
  }
#define LED_BRANCH_3C \
  point { \
    .x = 5, .y = 1, .b = LED_BRIGHTNESS_GREEN \
  }
#define LED_BRANCH_3D \
  point { \
    .x = 5, .y = 0, .b = LED_BRIGHTNESS_GREEN \
  }

#define LED_BRANCH_4A \
  point { \
    .x = 4, .y = 3, .b = LED_BRIGHTNESS_GREEN \
  }
#define LED_BRANCH_4B \
  point { \
    .x = 4, .y = 0, .b = LED_BRIGHTNESS_GREEN \
  }
#define LED_BRANCH_4C \
  point { \
    .x = 4, .y = 1, .b = LED_BRIGHTNESS_GREEN \
  }
#define LED_BRANCH_4D \
  point { \
    .x = 4, .y = 2, .b = LED_BRIGHTNESS_GREEN \
  }

/****************************************************************************
 ************************** Global variables ********************************
 ***************************************************************************/

void (*resetFunc)(void) = 0;  // declare reset function @ address 0

// Set up a new serial port for communication via FPC
SoftwareSerial flexSerial = SoftwareSerial(flexRxPin, flexTxPin);

// Set up LIS3DH accelerometer via I2C
Adafruit_LIS3DH lis = Adafruit_LIS3DH();
volatile unsigned long accel_timeout;  // counter for automatic sleep functionality
boolean accel_found;                   // set to 0 if accelerometer is not present

typedef enum {
  accel_motion_low = 1,
  accel_motion_medium = 2,
  accel_motion_high = 3,
  accel_motion_off = 0
} accel_state_enum;

// FPC
const byte numChars = 8;               // maximum length of incoming data
char receivedChars[numChars];          // buffer for incoming data
char tempChars[numChars];              // temporary array for use when parsing
char commandFromPC[numChars] = { 0 };  // variables to hold the parsed data
int valueFromPC = 0;
boolean newData = false;

// Arduino Serial Terminal
char receivedByte;
boolean newByte = false;

// EEPROM
// Contents are preserved during chip reprogramming
int timer = 0;  // counter used as seed for the PRNG
byte b4 = 0;    // unique ID
byte b5 = 0;
byte b6 = 0;
byte b7 = 0;
byte b8 = 0;  // game flags (bit set to 1 when task is completed)
byte b9 = 0;
volatile accel_state_enum b10;  // accelerometer/motion sensing state

// LEDs
PWMDirectMatrix *matrix;

GPIO_pin_t line_pins[] = { ledRow0, ledRow1, ledRow2, ledRow3 };
GPIO_pin_t column_pins[] = { DINV };                               // All column pins are controller by shift register
GPIO_pin_t sr_pins[] = { ledLatch, DINV, DINV, ledData, ledClk };  // DINV = no latch for that location

struct point {
  uint8_t x;  // x axis in matrix (0..7)
  uint8_t y;  // y axis in matrix (0..3)
  uint8_t b;  // brightness (0..15)
};

typedef enum {
  led_normal = 0,
  led_all_on = 1,
  led_all_off = 2,
  led_grow_tree = 4
} led_state_enum;
volatile led_state_enum ledState;

typedef enum {
  normal_game = 0,
  tree_game = 1,
  ring_game = 2,
  all_blink_game = 4
} game_mode_enum;
volatile game_mode_enum gameState;
volatile byte change_game_mode = 0;
volatile byte loop_count = 0;

// Power
/*
    nUSBPowerDetect     nBatteryChargerDetect
    -----------------------------------------
          L                      L              = Charger via USB
          L                      H              = Host PC via USB
          H                      L              = Invalid
          H                      H              = No USB Connected
*/
typedef enum {
  pwr_invalid = -1,
  pwr_battery = 0,
  pwr_pc = 1,
  pwr_charger = 2
} pwr_state_enum;
volatile pwr_state_enum pwrState;

/****************************************************************************
 *************************** Constants **************************************
 ***************************************************************************/

const char command_prompt[] PROGMEM = "\n\r> ";

const char menu_banner[] PROGMEM = "\n\r\
L: Toggle LED state [Normal, All On, All Off, Tree Grow]\n\r\
M: Toggle motion sensing [Low, Medium, High, Off]\n\r\
A: Read accelerometer\n\r\
U: Display unique ID\n\r\
F: Display game flags\n\r\
H: Display available commands\n\r\
Ctrl-C: System reset\
";

const char msg_welcome[] PROGMEM = "Welcome to the DEFCON China 1.0 Official Badge\n\r\n\r";
const char msg_init_start[] PROGMEM = "[*] Starting initialization\n\r";
const char msg_init_complete[] PROGMEM = "[*] Initialization complete\n\r";
const char msg_interactive_mode[] PROGMEM = "[*] Entering interactive mode [Press 'H' for available commands]\n\r";
const char msg_sleep_mode[] PROGMEM = "\n\r[*] Entering sleep mode\n\r";
const char msg_awake[] PROGMEM = "[*] Wake up\n\r";
const char msg_abort_key[] PROGMEM = "Press any key to abort\n\r";
const char msg_fpc_command[] PROGMEM = "[*] FPC Command Received\n\r";
const char msg_eeprom_config[] PROGMEM = "[*] Configuring EEPROM for first use\n\r";
const char msg_eeprom_uuid[] PROGMEM = "[*] Generating unique ID: 0x";
//const char msg_accel_config[] PROGMEM       = "[*] Configuring accelerometer\n\r";
const char msg_accel_nofound[] PROGMEM = "[*] Cannot find accelerometer\n\r";
const char msg_accel_motion_low[] PROGMEM = "[*] Motion sensing: Low [1g]\n\r";
const char msg_accel_motion_med[] PROGMEM = "[*] Motion sensing: Medium [375mg]\n\r";
const char msg_accel_motion_high[] PROGMEM = "[*] Motion sensing: High [100mg]\n\r";
const char msg_accel_motion_off[] PROGMEM = "[*] Motion sensing: Off\n\r";
const char msg_game_flags[] PROGMEM = "[*] Game flags: ";
const char msg_normal_game[] PROGMEM = "[*] Normal Blink Branches mode\n\r";
const char msg_tree_game[] PROGMEM = "[*] Growing Tree Blink mode\n\r";
const char msg_ring_game[] PROGMEM = "[*] Light Rings Blink mode\n\r";
const char msg_all_blink[] PROGMEM = "[*] All Blink mode\n\r";
const char msg_flag_usb[] PROGMEM = "[*] Flag: Badge connected to computer\n\r";
const char msg_flag_badge_hack[] PROGMEM = "[*] Flag: Badge hacking workshop\n\r";
const char msg_flag_solder_blob[] PROGMEM = "[*] Flag: Solder blob\n\r";
const char msg_uuid[] PROGMEM = "[*] Unique ID: 0x";
const char msg_set_led[] PROGMEM = "[*] Setting LED #";
const char msg_clear_led[] PROGMEM = "[*] Clearing LED #";
const char msg_led_state[] PROGMEM = "[*] Sending LED state\n\r";
const char msg_led_normal[] PROGMEM = "[*] LEDs: Normal\n\r";
const char msg_led_on[] PROGMEM = "[*] LEDs: All On\n\r";
const char msg_led_off[] PROGMEM = "[*] LEDs: All Off\n\r";
const char msg_led_tree[] PROGMEM = "[*] LEDs: Tree Grow Mode\n\r";

// Courtesy of https://manytools.org/hacker-tools/convert-images-to-ascii-art/
const char logo_banner[] PROGMEM = "\n\r\n\r\
                         :::::::::::::::::\n\r\
                   :::::::::::::::::::::::::::::\n\r\
                :::::::::::::::::::::::::::::::::::\n\r\
             :::::::::::::::::::::::::::::::::::::::::\n\r\
          :::::::::::::::::::          ::::::::::::::::::\n\r\
        :::::::::::::::::                  ::::::::::::::::\n\r\
       ::::::::::::::::                      :::::::::::::::\n\r\
     ::::::::::::::::                          :::::::::::::::\n\r\
    ::::::::::::::::       :::::      :::::     :::::::::::::::\n\r\
   ::::::   :::::::       :::::::    :::::::     :::::::::::::::\n\r\
  ::::::      :::::        :::::      :::::       :::::::   :::::\n\r\
 ::::::       :::::                               :::::       ::::\n\r\
 :::::::     ::::::   ::::                 ::::   :::::       ::::\n\r\
:::::           :::    ::                   ::    ::::::       ::::\n\r\
::::                    ::                 ::    :::             ::\n\r\
:::::     :::            ::               ::                     ::\n\r\
::::::::::::::::           :::         :::             ::::     :::\n\r\
::::::::::::::::::::          :::::::::            ::::::::::::::::\n\r\
::::::::::::::::::::::::                       ::::::::::::::::::::\n\r\
:::::::::::::::::::::::::::           ::   ::::::::::::::::::::::::\n\r\
:::::::::::::::::::::::::::: ::          ::::::::::::::::::::::::::\n\r\
 :::::::::::::::::::::::        ::           :::::::::::::::::::::\n\r\
 :::::::::::::::::::           :::::::          ::::::::::::::::::\n\r\
  ::::::     :::           ::::::::::::::           ::     ::::::\n\r\
   :::::               ::::::::::::::::::::::               ::::\n\r\
    ::::           :::::::::::::::::::::::::::::           ::::\n\r\
     :::::::    :::::::::::::::::::::::::::::::::::     ::::::\n\r\
       ::::      ::::::::::::::::::::::::::::::::::     ::::\n\r\
        :::      ::::::::::::::::::::::::::::::::::     :::\n\r\
          :::  ::::::::::::::::::::::::::::::::::::::::::\n\r\
             :::::::::::::::::::::::::::::::::::::::::\n\r\
                :::::::::::::::::::::::::::::::::::\n\r\
                   :::::::::::::::::::::::::::::\n\r\
                         :::::::::::::::::\n\r\
\n\r\
020d\n\r";

/****************************************************************************
 ********************* Function Prototypes **********************************
 ***************************************************************************/

void printProgStr(PGM_P str);

// LED_Matrix.cpp
void DirectMatrix_RefreshPWMLine(void);

/****************************************************************************
 ************************** Functions ***************************************
 ***************************************************************************/

void setup()  // Setup code (called once on start-up)
{
  uint8_t i;

  hardware_init();  // Low-level hardware initialization

  setupLEDs(true);
  LED_AllOn();  // turn on all LEDs

#ifndef __DEBUG
  delay(500);  // delay to allow for visual inspection
  printProgStr(logo_banner);

#else
  delay(2500);
#endif

  printProgStr(msg_welcome);
  printProgStr(msg_init_start);

#ifdef __DEBUG
  long dev_sign;
  Serial.print("[*] Device: ");
  cli();
  dev_sign = (long)boot_signature_byte_get(0x00) << 16;
  dev_sign |= (long)boot_signature_byte_get(0x02) << 8;
  dev_sign |= (long)boot_signature_byte_get(0x04);
  sei();
  switch (dev_sign) {
    case 0x1E9514:
      Serial.print("ATmega328");
      break;
    case 0x1E950F:
      Serial.print("ATmega328P");
      break;
    case 0x1E9516:
      Serial.print("ATmega328PB");
      // configure Port E[3:0] (new on the '328PB)
      // PE1 & PE0 are connected directly to GND & VCC, respectively, on our board in order to support the ATmega328P
      _SFR_IO8(0x0D) = 0b00000000;  // Port E Data Direction Register (DDR), set all to inputs
      _SFR_IO8(0x0E) = 0b00001100;  // Port E Data Register, ADC6/PE2 & ADC7/PE3 internal pull-up, PE1 & PE0 Hi-Z
    default:
      Serial.print("Unknown");
      break;
  }
  Serial.print(" [0x");
  i = (dev_sign >> 24) & 0xff;
  printHex(&i, 1);
  i = (dev_sign >> 16) & 0xff;
  printHex(&i, 1);
  i = (dev_sign >> 8) & 0xff;
  printHex(&i, 1);
  i = dev_sign & 0xff;
  printHex(&i, 1);
  Serial.println("]");

  Serial.print("[*] Fuses: ");
  cli();
  uint8_t lowBits = boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS);
  uint8_t highBits = boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS);
  uint8_t extendedBits = boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS);
  uint8_t lockBits = boot_lock_fuse_bits_get(GET_LOCK_BITS);
  sei();
  Serial.print("Extended: 0x");
  printHex(&extendedBits, 1);
  Serial.print(", High: 0x");
  printHex(&highBits, 1);
  Serial.print(", Low: 0x");
  printHex(&lowBits, 1);
  Serial.print(", Lock: 0x");
  printHex(&lockBits, 1);
  Serial.println();
#endif

  // Set initial state for debugging purposes
#ifdef __DEBUG
  //setGameFlags(0, 0);
  //setGameFlags(0b00000101, 0b00001111);
  //setGameFlags(0x7F, 0xFF);
  //wipeEEPROM();
#endif

  setupEEPROM();
  if (setupAccelerometer()) {
    // Blink the LEDs to indicate a failure
    for (i = 0; i < 4; i++) {
      LED_AllOff();
      delay(250);
      LED_AllOn();
      delay(250);
    }
  }

  LED_AllOff();  // turn off all LEDs
  printProgStr(msg_init_complete);

#ifdef __DEBUG
  b8 = 0xFF;
  b9 = 0xFF;
  EEPROM.write(EEBaseAddress + 9, b9);
  EEPROM.write(EEBaseAddress + 8, b8);
#endif
  // If you took the badge hacking workshop (or otherwise found the #define)
  if ((ATTENDED_BADGE_HACKING_WORKSHOP == true) && !bitRead(b9, 2)) {
    printProgStr(msg_flag_badge_hack);
    bitSet(b9, 2);  // update flag
    EEPROM.write(EEBaseAddress + 9, b9);
  }

  printProgStr(msg_interactive_mode);
  printProgStr(command_prompt);
  Serial.flush();  // Wait for all bytes to be transmitted to the console
}

/**************************************************************/

void hardware_init() {
  // Define pin modes
  pinMode(flexTxPin, OUTPUT);
  pinMode(nLedOE, OUTPUT);

  pinMode(flexRxPin, INPUT);
  pinMode(accelInt, INPUT);    // External pull-down resistor for low power
  pinMode(nFlexSense, INPUT);  // External pull-up resistor for low power

  pinMode(nUSBPowerDetect, INPUT_PULLUP);
  pinMode(nBatteryChargerDetect, INPUT_PULLUP);
  pinMode(nSolderBlob, INPUT_PULLUP);

  // Unused pin, set as input w/ pull-up for low power
  pinMode(flexGPIO, INPUT_PULLUP);

  // Setup Arduino Serial Monitor (via USB/FTDI FT231X to host PC)
  Serial.begin(9600);  // D0 = RXD, D1 = TXD
  while (!Serial)
    ;  // Wait until ready
}

/**************************************************************/

void setupLEDs(boolean init) {
  ledState = led_normal;      // current display state of LEDs
  gameState = normal_game;    // Default blink mode
  digitalWrite(nLedOE, LOW);  // enable shift register outputs

  if (init) {
    matrix = new PWMDirectMatrix(4, 8, 1, 1);  // Rows: 4, Columns: 8, Colors: 1, Common: 1 (Common pins of rows are anodes)

    // The ISR frequency is doubled 3 times to create 4 PWM values
    // and will run at x, x*2, x*4, x*16 to simulate 16 levels of
    // intensity without causing 16 interrupts at x, leaving more
    // time for the main loop and causing less intensity loss.
    matrix->begin(line_pins, column_pins, sr_pins, DirectMatrix_ISR_FREQ);
  } else {
    Timer1.initialize(DirectMatrix_ISR_FREQ);
    Timer1.attachInterrupt(DirectMatrix_RefreshPWMLine);
  }
}

/**************************************************************/

// Initialize EEPROM with default flags if it hasn't been done before
// From https://forum.arduino.cc/index.php?topic=65577.0
void setupEEPROM() {
  // Read memory locations and store in local/global variables
  byte b0 = EEPROM.read(EEBaseAddress);
  byte b1 = EEPROM.read(EEBaseAddress + 1);
  byte b2 = EEPROM.read(EEBaseAddress + 2);
  byte b3 = EEPROM.read(EEBaseAddress + 3);
  b4 = EEPROM.read(EEBaseAddress + 4);
  b5 = EEPROM.read(EEBaseAddress + 5);
  b6 = EEPROM.read(EEBaseAddress + 6);
  b7 = EEPROM.read(EEBaseAddress + 7);
  b8 = EEPROM.read(EEBaseAddress + 8);
  b9 = EEPROM.read(EEBaseAddress + 9);
  b10 = (accel_state_enum)EEPROM.read(EEBaseAddress + 10);

  if ((b0 != 'D') || (b1 != 'C') || (b2 != 'N') || (b3 != '1')) {
    printProgStr(msg_eeprom_config);

    b0 = 'D';  // magic bytes for our header
    b1 = 'C';
    b2 = 'N';
    b3 = '1';
    EEPROM.write(EEBaseAddress, b0);
    EEPROM.write(EEBaseAddress + 1, b1);
    EEPROM.write(EEBaseAddress + 2, b2);
    EEPROM.write(EEBaseAddress + 3, b3);

    b8 = 0;  // clear game flags
    b9 = 0;
    EEPROM.write(EEBaseAddress + 8, b8);
    EEPROM.write(EEBaseAddress + 9, b9);

    b10 = accel_motion_medium;  // enable motion sensing
    EEPROM.write(EEBaseAddress + 10, (byte)b10);
  }
}

/**************************************************************/

#ifdef __DEBUG
void setGameFlags(byte b8_new, byte b9_new)  // for testing/development purposes
{
  b8 = b8_new;
  b9 = b9_new;
  EEPROM.write(EEBaseAddress + 8, b8);
  EEPROM.write(EEBaseAddress + 9, b9);
}
#endif

/**************************************************************/

#ifdef __DEBUG
void wipeEEPROM()  // for testing/development purposes
{
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0xFF);
  }
}
#endif

/**************************************************************/

// Generate and load unique ID into EEPROM if it doesn't already exist
// From https://forum.arduino.cc/index.php?topic=65577.0
void setupEEPROM_UUID() {
  // b4, b5, b6, b7 already read on power-up via setupEEPROM() and stored in globals
  // if unique ID locations within EEPROM are unprogrammed, create a unique ID and store it
  if ((b4 == 0xFF) && (b5 == 0xFF) && (b6 == 0xFF) && (b7 == 0xFF)) {
    printProgStr(msg_eeprom_uuid);

    randomSeed(timer);  // seed PRNG using timer variable, which is incremented every loop()
    b4 = random(256);
    b5 = random(256);
    b6 = random(256);
    b7 = random(256);

    printHex(&b4, 1);
    printHex(&b5, 1);
    printHex(&b6, 1);
    printHex(&b7, 1);
    Serial.println();

    EEPROM.write(EEBaseAddress + 4, b4);
    EEPROM.write(EEBaseAddress + 5, b5);
    EEPROM.write(EEBaseAddress + 6, b6);
    EEPROM.write(EEBaseAddress + 7, b7);
  }
}

/**************************************************************/

boolean setupAccelerometer() {
  // Configure LIS3DH accelerometer
  //printProgStr(msg_accel_config);

  if (!lis.begin(LIS3DHAddress)) {
    printProgStr(msg_accel_nofound);
    accel_found = 0;
    b10 = accel_motion_off;                       // disable motion sensing
    EEPROM.write(EEBaseAddress + 10, (byte)b10);  // store setting in EEPROM to persist power cycle/new firmware

    return 1;
  } else  // If accelerometer was found, configure it
  {
    accel_found = 1;

    // Set control registers
    // Values based on https://forums.adafruit.com/viewtopic.php?f=19&t=87936 and ST LIS3DH Application note AN3308
    writeRegister(0x1F, 0x00);  // TEMP_CFG_REG: Disable temperature sensor, disable ADC
    writeRegister(0x20, 0x3F);  // CTRL_REG1: Enable X, Y, Z axes, output data rate (ODR) = 25Hz low power mode
    writeRegister(0x21, 0x09);  // CTRL_REG2: High-pass filter (HPF) enabled, cutoff frequency 0.5Hz (@ 25Hz ODR)
    writeRegister(0x22, 0x40);  // CTRL_REG3: ACC AOI1 interrupt signal is routed to INT1 pin
    writeRegister(0x23, 0x00);  // CTRL_REG4: Full Scale = +/-2 g
    writeRegister(0x24, 0x08);  // CTRL_REG5: Interrupt latched

    // Wake-up/motion detection
    Accel_SetMotion(b10);       // Configure and display detection threshold depending on current setting
    writeRegister(0x33, 0x01);  // INT1_DURATION: Duration = 1LSBs * (1/25Hz) = 0.04s (40ms)
    writeRegister(0x30, 0x2A);  // INT1_CFG: Enable interrupt generation on XLIE, YLIE, ZLIE

    accel_timeout = millis() + ACCEL_TIMEOUT;  // set starting point for automatic sleep timer

    // Allow accelerometer pin to trigger interrupt on logic high level
    attachInterrupt(digitalPinToInterrupt(accelInt), ISR_accelInt, HIGH);
  }

  return 0;
}

/**************************************************************/

// Configure and display detection threshold depending on current setting
void Accel_SetMotion(accel_state_enum accelState) {
  switch (accelState) {
    case accel_motion_low:  // low sensitivity (requires more motion to trigger interrupt)
      printProgStr(msg_accel_motion_low);
      b10 = accel_motion_low;
      writeRegister(0x32, 0x40);  // INT1_THS: Threshold (THS) = 64LSBs * 15.625mg/LSB = 1g
      break;
    case accel_motion_medium:  // medium sensitivity (requires moderate motion to trigger interrupt)
      printProgStr(msg_accel_motion_med);
      b10 = accel_motion_medium;
      writeRegister(0x32, 0x18);  // INT1_THS: Threshold (THS) = 24LSBs * 15.625mg/LSB = 375mg
      break;
    case accel_motion_high:  // high sensitivity (requires smaller motion to trigger interrupt)
      printProgStr(msg_accel_motion_high);
      b10 = accel_motion_high;
      writeRegister(0x32, 0x06);  // INT1_THS: Threshold (THS) = 7LSBs * 15.625mg/LSB = ~100mg
      break;
    case accel_motion_off:  // disable motion sensing code (set accelerometer threshold to maximum to avoid spurious interrupts)
      printProgStr(msg_accel_motion_off);
      b10 = accel_motion_off;
      writeRegister(0x32, 0x7F);  // INT1_THS: Threshold (THS) = 127LSBs * 15.625mg/LSB = ~2g
      break;
  }
  EEPROM.write(EEBaseAddress + 10, (byte)b10);  // store setting in EEPROM to persist power cycle/new firmware
}

/**************************************************************/

void loop()  // Main code (runs repeatedly)
{
  // Set current power state
  if (!digitalRead(nUSBPowerDetect) && !digitalRead(nBatteryChargerDetect))  // Charger via USB
    pwrState = pwr_charger;
  else if (!digitalRead(nUSBPowerDetect) && digitalRead(nBatteryChargerDetect))  // Host PC via USB
    pwrState = pwr_pc;
  else if (digitalRead(nUSBPowerDetect) && digitalRead(nBatteryChargerDetect))  // Battery power (no USB connected)
    pwrState = pwr_battery;
  else
    pwrState = pwr_invalid;  // Invalid (should never be this)

  // If we are connected to a host PC via USB, allow interactive mode (via Arduino Serial Monitor)
  if (pwrState == pwr_pc) {
    recvOneChar();
    processOneChar();
  }
  // If we are running on battery power, not connected to FPC, accelerometer is enabled, and there has been no motion for defined period
  // Go into low-power sleep mode until external interrupt from accelerometer is asserted or FPC is plugged in
#ifndef __DEBUG
  else if ((pwrState == pwr_battery) && digitalRead(nFlexSense) && ((int32_t)(millis() - accel_timeout) > 0) && (b10 != accel_motion_off))
#else
  if (digitalRead(nFlexSense) && ((int32_t)(millis() - accel_timeout) > 0) && (b10 != accel_motion_off))
#endif
  {
    printProgStr(msg_sleep_mode);
    Serial.flush();            // Wait for all bytes to be transmitted to the console
    Serial.end();              // End Arduino Serial Monitor
    pinMode(1, INPUT_PULLUP);  // Change TXD pin to input to prevent backpowering the FT231X during sleep

    // Clear LEDs
    digitalWrite(nLedOE, HIGH);  // disable shift register outputs
    LED_AllOff();                // turn off all LEDs
    delay(100);                  // delay to let matrix turn off
    Timer1.stop();               // disable Timer 1 interrupts from LED matrix library
    Timer1.detachInterrupt();

    // Accelerometer re-configuration
    readRegister(0x26);                // Dummy read of HP_FILTER_RESET to set reference acceleration (deletes the DC component of the current acceleration)
    readRegister(LIS3DH_REG_INT1SRC);  // Read LIS3DH's INT1_SRC to return interrupt flags and clear interrupt
    delay(100);                        // Settling time to allow accelInt to go back to LOW

    // Allow nFlexSense pin to trigger interrupt on logic low level (when it is connected to FPC Shield)
    attachInterrupt(digitalPinToInterrupt(nFlexSense), ISR_nFlexSense, LOW);

    // Enter power down state with ADC and BOD module disabled
    // This will reduce the power consumption to a few uA
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

    // Wake up here when we've received an interrupt

    detachInterrupt(digitalPinToInterrupt(nFlexSense));  // Disable nFlexSense pin interrupt
    accel_timeout = millis() + ACCEL_TIMEOUT;            // re-set starting point for automatic sleep timer

    // Refresh LEDs
    delay(100);
    setupLEDs(false);  // re-enable LED matrix

    // Setup Arduino Serial Monitor again (via USB/FTDI FT231X to host PC)
    Serial.begin(9600);  // D0 = RXD, D1 = TXD
    while (!Serial)
      ;  // Wait until ready
    printProgStr(msg_awake);
  }

  // If ATmega328P pin 9 (D5, ROW2) is connected/soldered to pin 10 (D6), nSolderBlob will go LOW during LED multiplexing
  if (!digitalRead(nSolderBlob) && !bitRead(b9, 1)) {
    printProgStr(msg_flag_solder_blob);
    bitSet(b9, 1);  // update flag
    EEPROM.write(EEBaseAddress + 9, b9);
    pinMode(nSolderBlob, INPUT);  // re-configure pin as an input (without pull-up)
  }

  // receive and process any data via FPC (non-blocking)
  // from https://forum.arduino.cc/index.php?topic=396450.msg2727728#msg2727728
  // data expected in the following format: <command,value (optional)>
  // command = character or string
  // value = integer
  if (!digitalRead(nFlexSense))  // if FPC is connected
  {
    // LED matrix library uses Timer 1 interrupts, so it conflicts with SoftwareSerial
    LED_AllOff();   // clear matrix
    delay(100);     // delay to let matrix turn off
    Timer1.stop();  // disable Timer 1 interrupts from LED matrix library
    Timer1.detachInterrupt();

    // Setup FPC Serial Port
    flexSerial.begin(9600);
    while (!flexSerial)
      ;  // Wait until ready

    delay(1000);                      // Delay to ensure badge is fully inserted into connector (poor man's debounce)
    while (!digitalRead(nFlexSense))  // while FPC remains connected
    {
      recvWithStartEndMarkers();
      if (newData == true) {
        strcpy(tempChars, receivedChars);
        // this temporary copy is necessary to protect the original data
        // because strtok() used in parseData() replaces the commas with \0
        parseData();
        showParsedData();

        // act on newly received command/value accordingly (first byte only)
        switch (commandFromPC[0]) {
          case 'S':  // set LED on badge
          case 's':
            printProgStr(msg_set_led);
            Serial.println(valueFromPC, DEC);
            if (valueFromPC > 7)  // 15..8
            {
              bitSet(b8, valueFromPC - 8);  // update flag
            } else                          // 7..0
            {
              bitSet(b9, valueFromPC);  // update flag
            }
            EEPROM.write(EEBaseAddress + 8, b8);
            EEPROM.write(EEBaseAddress + 9, b9);
            break;
          case 'C':  // clear LED on badge
          case 'c':
            printProgStr(msg_clear_led);
            Serial.println(valueFromPC, DEC);
            if (valueFromPC > 7)  // 15..8
            {
              bitClear(b8, valueFromPC - 8);  // update flag
            } else                            // 7..0
            {
              bitClear(b9, valueFromPC);  // update flag
            }
            EEPROM.write(EEBaseAddress + 8, b8);
            EEPROM.write(EEBaseAddress + 9, b9);
            break;
          case 'R':  // read state of LEDs from badge
          case 'r':
            printProgStr(msg_led_state);
            setupEEPROM_UUID();  // generate unique ID and store in EEPROM if it doesn't already exist

            flexSerial.print(DCN_UART_START);
            flexSerial.write(b4);  // unique ID
            flexSerial.write(b5);
            flexSerial.write(b6);
            flexSerial.write(b7);
            flexSerial.write(b8);  // game flags (bit set to 1 when task is completed)
            flexSerial.write(b9);
            flexSerial.print(DCN_UART_END);
            break;
        }

        newData = false;
      }
    }

    flexSerial.flush();  // wait for all bytes to be transmitted
    flexSerial.end();    // disable FPC serial port
    delay(100);
    setupLEDs(false);                          // re-enable LED matrix
    accel_timeout = millis() + ACCEL_TIMEOUT;  // re-set starting point for automatic sleep timer
  }

  // This is why my tree mode would be stuck after sleep. It's no longer in led_normal mode so not getting refreshes.
  if (ledState == led_normal) {
    LED_GameMode();             // update display to reflect current flags
    digitalWrite(nLedOE, LOW);  // enable shift register outputs
  }

  readRegister(LIS3DH_REG_INT1SRC);  // Read LIS3DH's INT1_SRC to return interrupt flags and clear interrupt (if any)
  delay(100);                        // Settling time to allow accelInt to go back to LOW
  timer++;

  // wait in an idle state until Timer 1 (used by LED matrix driver) wakes us up to proceed
  LowPower.idle(SLEEP_FOREVER, ADC_OFF, TIMER2_OFF, TIMER1_ON, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);
}

/**************************************************************/

void recvOneChar() {
  if (Serial.available() > 0) {
    receivedByte = Serial.read();
    newByte = true;
  }
}

/**************************************************************/

void processOneChar() {
  if (newByte == true) {
    accel_timeout = millis() + ACCEL_TIMEOUT;  // re-set starting point for automatic sleep timer

    // if badge receives any data for the first time...
    if (!bitRead(b9, 0)) {
      printProgStr(msg_flag_usb);
      bitSet(b9, 0);  // update flag
      EEPROM.write(EEBaseAddress + 9, b9);
    }

    // local echo if character is printable and not whitespace
    if (isGraph(receivedByte)) {
      Serial.println(receivedByte);
    }

    switch (receivedByte) {
      case '?':
      case 'H':  // Display menu
      case 'h':
        printProgStr(menu_banner);
        break;
      case 'F':  // Display current game flags (read from EEPROM)
      case 'f':
        printProgStr(msg_game_flags);
        printBits(b8);
        Serial.print(" ");
        printBits(b9);
        Serial.print(" [0x");
        printHex(&b8, 1);
        printHex(&b9, 1);
        Serial.println("]");
        break;
      case 'U':  // Display unique ID
      case 'u':
        printProgStr(msg_uuid);
        printHex(&b4, 1);
        printHex(&b5, 1);
        printHex(&b6, 1);
        printHex(&b7, 1);
        Serial.println();
        break;
      case 'A':  // Read accelerometer
      case 'a':
        if (accel_found) {
          writeRegister(0x21, 0x00);  // CTRL_REG2: High-pass filter (HPF) disabled (to provide raw values during awake operation)
          delay(200);
          lis.read();  // dummy read to ignore the first results
          printProgStr(msg_abort_key);
          while (Serial.read() >= 0)
            ;  // flush the receive buffer
          while (Serial.available() == 0) {
            // Code from https://learn.adafruit.com/adafruit-lis3dh-triple-axis-accelerometer-breakout/arduino
            lis.read();  // get X, Y, and Z data all at once
            Serial.print("X:  ");
            Serial.print(lis.x);  // display raw values
            Serial.print("   \tY:  ");
            Serial.print(lis.y);
            Serial.print("   \tZ:  ");
            Serial.print(lis.z);

            sensors_event_t event;  // get a normalized sensor event
            lis.getEvent(&event);

            Serial.print("  \tX: ");
            Serial.print(event.acceleration.x);  // display normalized results (measured in m/s^2)
            Serial.print("  \tY: ");
            Serial.print(event.acceleration.y);
            Serial.print("  \tZ: ");
            Serial.print(event.acceleration.z);
            Serial.println(" m/s^2");
            delay(200);
          }
          while (Serial.read() >= 0)
            ;                         // flush the receive buffer
          writeRegister(0x21, 0x09);  // CTRL_REG2: High-pass filter (HPF) enabled, cutoff frequency 0.5Hz (@ 25Hz ODR)
        } else
          printProgStr(msg_accel_nofound);
        break;
      case 'L':  // Toggle all LEDs
      case 'l':
        switch (ledState) {
          case led_normal:
            printProgStr(msg_led_on);
            LED_AllOn();            // turn on all LEDs
            ledState = led_all_on;  // switch state for next iteration
            break;
          case led_all_on:
            printProgStr(msg_led_off);
            LED_AllOff();            // turn off all LEDs
            ledState = led_all_off;  // switch state for next iteration
            break;
          case led_all_off:
            printProgStr(msg_led_tree);
            LED_AllBlink();
            ledState = led_grow_tree;  // switch state for next iteration
            break;
          case led_grow_tree:
            printProgStr(msg_led_normal);
            LED_GameMode();  // normal display (game mode)
            ledState = led_normal;
            break;
        }
        break;
      case 'G':
      case 'g':
        nextGameMode();
        break;
      case 'M':  // Toggle motion sensing/sensitivity
      case 'm':
        if (accel_found) {
          switch (b10) {
            case accel_motion_off:
              Accel_SetMotion(accel_motion_low);  // switch state for next iteration
              break;
            case accel_motion_low:
              Accel_SetMotion(accel_motion_medium);  // switch state for next iteration
              break;
            case accel_motion_medium:
              Accel_SetMotion(accel_motion_high);  // switch state for next iteration
              break;
            case accel_motion_high:
              Accel_SetMotion(accel_motion_off);  // switch state for next iteration
              break;
          }
        } else
          printProgStr(msg_accel_nofound);
        break;
      case 3:         // ETX (Ctrl-C)
        resetFunc();  // System reset (does not return)
        break;
    }

    printProgStr(command_prompt);
    newByte = false;
  }
}

/**************************************************************/

void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = DCN_UART_START;
  char endMarker = DCN_UART_END;
  char rc;

  while (flexSerial.available() > 0 && newData == false) {
    rc = flexSerial.read();

    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;  // truncate buffer to maximum allowable
        }
      } else {
        receivedChars[ndx] = '\0';  // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }

    else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

/**************************************************************/

void parseData()  // split the received data into its parts
{
  char *strtokIndx;  // this is used by strtok() as an index

  strtokIndx = strtok(tempChars, ",");  // get the first part - the command
  strcpy(commandFromPC, strtokIndx);    // copy it to commandFromPC

  strtokIndx = strtok(NULL, ",");  // this continues where the previous call left off
  valueFromPC = atoi(strtokIndx);  // convert this part to an integer
}

/**************************************************************/

void showParsedData() {
  printProgStr(msg_fpc_command);
  Serial.print("-> Command: ");
  Serial.println(commandFromPC);

  Serial.print("-> Value: ");
  Serial.println(valueFromPC, DEC);
}

/**************************************************************/

// print 8-bit data in hex with leading zero if needed
// from http://forum.arduino.cc/index.php?topic=38107#msg282343
void printHex(uint8_t *data, uint8_t length) {
  char tmp[length * 2 + 1];
  byte first;
  int j = 0;

  for (uint8_t i = 0; i < length; i++) {
    first = (data[i] >> 4) | 48;
    if (first > 57)
      tmp[j] = first + (byte)39;
    else
      tmp[j] = first;
    j++;

    first = (data[i] & 0x0F) | 48;
    if (first > 57)
      tmp[j] = first + (byte)39;
    else
      tmp[j] = first;
    j++;
  }

  tmp[length * 2] = 0;
  Serial.print(tmp);
}

/**************************************************************/

// print all 8 bits of a byte including leading zeros
// from https://forum.arduino.cc/index.php?topic=46320.0
void printBits(byte myByte) {
  for (byte mask = 0x80; mask; mask >>= 1) {
    if (mask & myByte)
      Serial.print('1');
    else
      Serial.print('0');
  }
}

/**************************************************************/

// given a PROGMEM string, use Serial.print() to send it out
void printProgStr(PGM_P str) {
  int i;
  char myChar;

  for (i = 0; i < strlen_P(str); i++) {
    myChar = pgm_read_byte_near(str + i);
    Serial.print(myChar);
  }
}

/**************************************************************/

// Utility function to read data from specified register of the LIS3DH accelerometer
unsigned int readRegister(byte reg) {
  Wire.beginTransmission(LIS3DHAddress);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(LIS3DHAddress, 1);
  return Wire.read();
}

/**************************************************************/

// Utility function to write data to specified register of the LIS3DH accelerometer
void writeRegister(byte reg, byte data) {
  Wire.beginTransmission(LIS3DHAddress);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

/**************************************************************/

void nextGameMode() {
  switch (gameState) {
    case normal_game:
      printProgStr(msg_tree_game);
      gameState = tree_game;
      break;
    case tree_game:
      printProgStr(msg_ring_game);
      gameState = ring_game;
      break;
    case ring_game:
      printProgStr(msg_all_blink);
      gameState = all_blink_game;
      break;
    case all_blink_game:
      printProgStr(msg_normal_game);
      gameState = normal_game;
      break;
  }
}

void changeGameMode() {

  sensors_event_t event;  // get a normalized sensor event
                          //Serial.write("Getting event");
  lis.getEvent(&event);
  //Serial.write("Got event");

  change_game_mode = 0;
  if (event.acceleration.y < -8.0) {
    Serial.print("  \tX: ");
    Serial.print(event.acceleration.x);  // display normalized results (measured in m/s^2)
    Serial.print("  \tY: ");
    Serial.print(event.acceleration.y);
    Serial.print("  \tZ: ");
    Serial.print(event.acceleration.z);
    Serial.println(" m/s^2");
    Serial.println("Triggering game mode switch!");
    nextGameMode();
  }
}
void LED_AllBlink() {
  //LED_AllOff();
  for (int xj = 0; xj < 4; xj++) {

    for (int yi = 0; yi < 8; yi++) {
      matrix->drawPixel(yi, xj, millis() % 16);

      // matrix->writeDisplay();
      //delay(100);
    }
  }

  //LED_GameMode();
}

void LED_Root2Branch() {
  matrix->clear();
  if (loop_count < 10) {
    loop_count = 0;
  }
  if (loop_count > 50) {  // Inner branch lights
    matrix->drawPixel(LED_ROOT_1A.x, LED_ROOT_1A.y, LED_ROOT_1A.b);
    matrix->drawPixel(LED_ROOT_2A.x, LED_ROOT_2A.y, LED_ROOT_2A.b);
    matrix->drawPixel(LED_ROOT_3A.x, LED_ROOT_3A.y, LED_ROOT_3A.b);
    matrix->drawPixel(LED_ROOT_4A.x, LED_ROOT_4A.y, LED_ROOT_4A.b);
  }
  if (loop_count > 60) {
    matrix->drawPixel(LED_ROOT_1B.x, LED_ROOT_1B.y, LED_ROOT_1B.b);
    matrix->drawPixel(LED_ROOT_2B.x, LED_ROOT_2B.y, LED_ROOT_2B.b);
    matrix->drawPixel(LED_ROOT_3B.x, LED_ROOT_3B.y, LED_ROOT_3B.b);
    matrix->drawPixel(LED_ROOT_4B.x, LED_ROOT_4B.y, LED_ROOT_4B.b);
  }
  if (loop_count > 80) {  // Inner root lights
    matrix->drawPixel(LED_ROOT_1C.x, LED_ROOT_1C.y, LED_ROOT_1C.b);
    matrix->drawPixel(LED_ROOT_2C.x, LED_ROOT_2C.y, LED_ROOT_2C.b);
    matrix->drawPixel(LED_ROOT_3C.x, LED_ROOT_3C.y, LED_ROOT_3C.b);
    matrix->drawPixel(LED_ROOT_4C.x, LED_ROOT_4C.y, LED_ROOT_4C.b);
  }
  if (loop_count > 120) {  // Middle root lights

    matrix->drawPixel(LED_ROOT_1D.x, LED_ROOT_1D.y, LED_ROOT_1D.b);
    matrix->drawPixel(LED_ROOT_2D.x, LED_ROOT_2D.y, LED_ROOT_2D.b);
    matrix->drawPixel(LED_ROOT_3D.x, LED_ROOT_3D.y, LED_ROOT_3D.b);
    matrix->drawPixel(LED_ROOT_4D.x, LED_ROOT_4D.y, LED_ROOT_4D.b);
  }
  if (loop_count > 160) {  // Outer ring


    matrix->drawPixel(LED_BRANCH_1A.x, LED_BRANCH_1A.y, LED_BRANCH_1A.b);
    matrix->drawPixel(LED_BRANCH_2A.x, LED_BRANCH_2A.y, LED_BRANCH_1A.b);
    matrix->drawPixel(LED_BRANCH_3A.x, LED_BRANCH_3A.y, LED_BRANCH_1A.b);
    matrix->drawPixel(LED_BRANCH_4A.x, LED_BRANCH_4A.y, LED_BRANCH_1A.b);
  }
  if (loop_count > 200) {
    matrix->drawPixel(LED_BRANCH_1B.x, LED_BRANCH_1B.y, LED_BRANCH_1B.b);
    matrix->drawPixel(LED_BRANCH_1D.x, LED_BRANCH_1D.y, LED_BRANCH_1D.b);
    matrix->drawPixel(LED_BRANCH_2C.x, LED_BRANCH_2C.y, LED_BRANCH_2C.b);
    matrix->drawPixel(LED_BRANCH_3B.x, LED_BRANCH_3B.y, LED_BRANCH_3B.b);
    matrix->drawPixel(LED_BRANCH_3D.x, LED_BRANCH_3D.y, LED_BRANCH_3D.b);
    matrix->drawPixel(LED_BRANCH_4C.x, LED_BRANCH_4C.y, LED_BRANCH_4C.b);

    matrix->drawPixel(LED_BRANCH_1C.x, LED_BRANCH_1C.y, LED_BRANCH_1C.b);
    matrix->drawPixel(LED_BRANCH_2B.x, LED_BRANCH_2B.y, LED_BRANCH_2B.b);
    matrix->drawPixel(LED_BRANCH_2D.x, LED_BRANCH_2D.y, LED_BRANCH_2D.b);
    matrix->drawPixel(LED_BRANCH_3C.x, LED_BRANCH_3C.y, LED_BRANCH_3C.b);
    matrix->drawPixel(LED_BRANCH_4B.x, LED_BRANCH_4B.y, LED_BRANCH_4B.b);
    matrix->drawPixel(LED_BRANCH_4D.x, LED_BRANCH_4D.y, LED_BRANCH_4D.b);
  }
  if (loop_count > 220) {
    // It just looked bad to me splitting off the final green edge into 2 parts so I moved them all together above.
  }
  loop_count += 20;
  matrix->writeDisplay();
}

void LED_RingMode() {
  matrix->clear();
  if (loop_count < 10) {
    loop_count = 0;
  }
  if (loop_count > 50) {  // Inner branch lights
    matrix->drawPixel(LED_BRANCH_1A.x, LED_BRANCH_1A.y, LED_BRANCH_1A.b);
    matrix->drawPixel(LED_BRANCH_2A.x, LED_BRANCH_2A.y, LED_BRANCH_1A.b);
    matrix->drawPixel(LED_BRANCH_3A.x, LED_BRANCH_3A.y, LED_BRANCH_1A.b);
    matrix->drawPixel(LED_BRANCH_4A.x, LED_BRANCH_4A.y, LED_BRANCH_1A.b);
  }
  if (loop_count > 100) {  // Inner root lights
    matrix->drawPixel(LED_ROOT_1D.x, LED_ROOT_1D.y, LED_ROOT_1D.b);
    matrix->drawPixel(LED_ROOT_2D.x, LED_ROOT_2D.y, LED_ROOT_2D.b);
    matrix->drawPixel(LED_ROOT_3D.x, LED_ROOT_3D.y, LED_ROOT_3D.b);
    matrix->drawPixel(LED_ROOT_4D.x, LED_ROOT_4D.y, LED_ROOT_4D.b);
  }
  if (loop_count > 150) {  // Middle root lights
    matrix->drawPixel(LED_ROOT_1C.x, LED_ROOT_1C.y, LED_ROOT_1C.b);
    matrix->drawPixel(LED_ROOT_2C.x, LED_ROOT_2C.y, LED_ROOT_2C.b);
    matrix->drawPixel(LED_ROOT_3C.x, LED_ROOT_3C.y, LED_ROOT_3C.b);
    matrix->drawPixel(LED_ROOT_4C.x, LED_ROOT_4C.y, LED_ROOT_4C.b);
  }
  if (loop_count > 200) {  // Outer ring

    matrix->drawPixel(LED_BRANCH_1B.x, LED_BRANCH_1B.y, LED_BRANCH_1B.b);
    matrix->drawPixel(LED_BRANCH_1C.x, LED_BRANCH_1C.y, LED_BRANCH_1C.b);
    matrix->drawPixel(LED_BRANCH_1D.x, LED_BRANCH_1D.y, LED_BRANCH_1D.b);


    matrix->drawPixel(LED_BRANCH_2B.x, LED_BRANCH_2B.y, LED_BRANCH_2B.b);
    matrix->drawPixel(LED_BRANCH_2C.x, LED_BRANCH_2C.y, LED_BRANCH_2C.b);
    matrix->drawPixel(LED_BRANCH_2D.x, LED_BRANCH_2D.y, LED_BRANCH_2D.b);


    matrix->drawPixel(LED_BRANCH_3B.x, LED_BRANCH_3B.y, LED_BRANCH_3B.b);
    matrix->drawPixel(LED_BRANCH_3C.x, LED_BRANCH_3C.y, LED_BRANCH_3C.b);
    matrix->drawPixel(LED_BRANCH_3D.x, LED_BRANCH_3D.y, LED_BRANCH_3D.b);


    matrix->drawPixel(LED_BRANCH_4B.x, LED_BRANCH_4B.y, LED_BRANCH_4B.b);
    matrix->drawPixel(LED_BRANCH_4C.x, LED_BRANCH_4C.y, LED_BRANCH_4C.b);
    matrix->drawPixel(LED_BRANCH_4D.x, LED_BRANCH_4D.y, LED_BRANCH_4D.b);

    matrix->drawPixel(LED_ROOT_1A.x, LED_ROOT_1A.y, LED_ROOT_1A.b);
    matrix->drawPixel(LED_ROOT_2A.x, LED_ROOT_2A.y, LED_ROOT_2A.b);
    matrix->drawPixel(LED_ROOT_3A.x, LED_ROOT_3A.y, LED_ROOT_3A.b);
    matrix->drawPixel(LED_ROOT_4A.x, LED_ROOT_4A.y, LED_ROOT_4A.b);

    matrix->drawPixel(LED_ROOT_1B.x, LED_ROOT_1B.y, LED_ROOT_1B.b);
    matrix->drawPixel(LED_ROOT_2B.x, LED_ROOT_2B.y, LED_ROOT_2B.b);
    matrix->drawPixel(LED_ROOT_3B.x, LED_ROOT_3B.y, LED_ROOT_3B.b);
    matrix->drawPixel(LED_ROOT_4B.x, LED_ROOT_4B.y, LED_ROOT_4B.b);
  }
  loop_count += 20;
  matrix->writeDisplay();
}

void LED_AllBlinkMode() {
  int m = millis();

  for (int li = 0; li < 4; li++) {
    int x = (m + li) % 8;
    int bright;
    if (x < 2) bright = LED_BRIGHTNESS_RED + 1;
    else if (x < 3) bright = LED_BRIGHTNESS_ORANGE + 1;
    else bright = LED_BRIGHTNESS_GREEN + 1;
    for (int lj = 0; lj < 4; lj++)
      matrix->drawPixel(x, li, (m + lj) % bright);
  }
  matrix->writeDisplay();
}
/**************************************************************/

void LED_AllOn() {
  // set each individual pixel to account for differences in brightness
  matrix->drawPixel(LED_ROOT_1A.x, LED_ROOT_1A.y, LED_ROOT_1A.b);
  matrix->drawPixel(LED_ROOT_1B.x, LED_ROOT_1B.y, LED_ROOT_1B.b);
  matrix->drawPixel(LED_ROOT_1C.x, LED_ROOT_1C.y, LED_ROOT_1C.b);
  matrix->drawPixel(LED_ROOT_1D.x, LED_ROOT_1D.y, LED_ROOT_1D.b);

  matrix->drawPixel(LED_ROOT_2A.x, LED_ROOT_2A.y, LED_ROOT_2A.b);
  matrix->drawPixel(LED_ROOT_2B.x, LED_ROOT_2B.y, LED_ROOT_2B.b);
  matrix->drawPixel(LED_ROOT_2C.x, LED_ROOT_2C.y, LED_ROOT_2C.b);
  matrix->drawPixel(LED_ROOT_2D.x, LED_ROOT_2D.y, LED_ROOT_2D.b);

  matrix->drawPixel(LED_ROOT_3A.x, LED_ROOT_3A.y, LED_ROOT_3A.b);
  matrix->drawPixel(LED_ROOT_3B.x, LED_ROOT_3B.y, LED_ROOT_3B.b);
  matrix->drawPixel(LED_ROOT_3C.x, LED_ROOT_3C.y, LED_ROOT_3C.b);
  matrix->drawPixel(LED_ROOT_3D.x, LED_ROOT_3D.y, LED_ROOT_3D.b);

  matrix->drawPixel(LED_ROOT_4A.x, LED_ROOT_4A.y, LED_ROOT_4A.b);
  matrix->drawPixel(LED_ROOT_4B.x, LED_ROOT_4B.y, LED_ROOT_4B.b);
  matrix->drawPixel(LED_ROOT_4C.x, LED_ROOT_4C.y, LED_ROOT_4C.b);
  matrix->drawPixel(LED_ROOT_4D.x, LED_ROOT_4D.y, LED_ROOT_4D.b);

  matrix->drawPixel(LED_BRANCH_1A.x, LED_BRANCH_1A.y, LED_BRANCH_1A.b);
  matrix->drawPixel(LED_BRANCH_1B.x, LED_BRANCH_1B.y, LED_BRANCH_1B.b);
  matrix->drawPixel(LED_BRANCH_1C.x, LED_BRANCH_1C.y, LED_BRANCH_1C.b);
  matrix->drawPixel(LED_BRANCH_1D.x, LED_BRANCH_1D.y, LED_BRANCH_1D.b);

  matrix->drawPixel(LED_BRANCH_2A.x, LED_BRANCH_2A.y, LED_BRANCH_2A.b);
  matrix->drawPixel(LED_BRANCH_2B.x, LED_BRANCH_2B.y, LED_BRANCH_2B.b);
  matrix->drawPixel(LED_BRANCH_2C.x, LED_BRANCH_2C.y, LED_BRANCH_2C.b);
  matrix->drawPixel(LED_BRANCH_2D.x, LED_BRANCH_2D.y, LED_BRANCH_2D.b);

  matrix->drawPixel(LED_BRANCH_3A.x, LED_BRANCH_3A.y, LED_BRANCH_3A.b);
  matrix->drawPixel(LED_BRANCH_3B.x, LED_BRANCH_3B.y, LED_BRANCH_3B.b);
  matrix->drawPixel(LED_BRANCH_3C.x, LED_BRANCH_3C.y, LED_BRANCH_3C.b);
  matrix->drawPixel(LED_BRANCH_3D.x, LED_BRANCH_3D.y, LED_BRANCH_3D.b);

  matrix->drawPixel(LED_BRANCH_4A.x, LED_BRANCH_4A.y, LED_BRANCH_4A.b);
  matrix->drawPixel(LED_BRANCH_4B.x, LED_BRANCH_4B.y, LED_BRANCH_4B.b);
  matrix->drawPixel(LED_BRANCH_4C.x, LED_BRANCH_4C.y, LED_BRANCH_4C.b);
  matrix->drawPixel(LED_BRANCH_4D.x, LED_BRANCH_4D.y, LED_BRANCH_4D.b);

  matrix->writeDisplay();
}

/**************************************************************/

void LED_AllOff() {
  matrix->clear();
  matrix->writeDisplay();
}

void default_sparkle() {
  matrix->drawPixel(LED_ROOT_1A.x, LED_ROOT_1A.y, LED_ROOT_1A.b);
  matrix->drawPixel(LED_ROOT_1B.x, LED_ROOT_1B.y, LED_ROOT_1B.b);
  matrix->drawPixel(LED_ROOT_1C.x, LED_ROOT_1C.y, LED_ROOT_1C.b);
  matrix->drawPixel(LED_ROOT_1D.x, LED_ROOT_1D.y, LED_ROOT_1D.b);

  matrix->drawPixel(LED_ROOT_2A.x, LED_ROOT_2A.y, LED_ROOT_2A.b);
  matrix->drawPixel(LED_ROOT_2B.x, LED_ROOT_2B.y, LED_ROOT_2B.b);
  matrix->drawPixel(LED_ROOT_2C.x, LED_ROOT_2C.y, LED_ROOT_2C.b);
  matrix->drawPixel(LED_ROOT_2D.x, LED_ROOT_2D.y, LED_ROOT_2D.b);

  matrix->drawPixel(LED_ROOT_3A.x, LED_ROOT_3A.y, LED_ROOT_3A.b);
  matrix->drawPixel(LED_ROOT_3B.x, LED_ROOT_3B.y, LED_ROOT_3B.b);
  matrix->drawPixel(LED_ROOT_3C.x, LED_ROOT_3C.y, LED_ROOT_3C.b);
  matrix->drawPixel(LED_ROOT_3D.x, LED_ROOT_3D.y, LED_ROOT_3D.b);

  matrix->drawPixel(LED_ROOT_4A.x, LED_ROOT_4A.y, LED_ROOT_4A.b);
  matrix->drawPixel(LED_ROOT_4B.x, LED_ROOT_4B.y, LED_ROOT_4B.b);
  matrix->drawPixel(LED_ROOT_4C.x, LED_ROOT_4C.y, LED_ROOT_4C.b);
  matrix->drawPixel(LED_ROOT_4D.x, LED_ROOT_4D.y, LED_ROOT_4D.b);

  // sparkle the branches
  matrix->drawPixel(random(4, 8), random(4), random(16));  // set random pixel at random brightness
}

/**************************************************************/

void LED_GameMode()  // update display to reflect current flags
{
  if (change_game_mode == 1) {  // movement inturrupt with downward shake detected.
    changeGameMode();
  }
  // if all roots unlocked, light the whole tree in sparkle mode
  if ((b8 == 0xFF) && (b9 == 0xFF)) {
    // enable all roots
    /*

        */
    if (gameState == normal_game) {
      default_sparkle();
    } else if (gameState == tree_game) {
      LED_Root2Branch();
    } else if (gameState == ring_game) {
      LED_RingMode();
    } else if (gameState == all_blink_game) {
      LED_AllBlink();
    }
    //delay(10);
  } else {
    // set each individual pixel if its corresponding flag is set
    if (bitRead(b9, 0)) matrix->drawPixel(LED_ROOT_1A.x, LED_ROOT_1A.y, LED_ROOT_1A.b);
    if (bitRead(b9, 1)) matrix->drawPixel(LED_ROOT_1B.x, LED_ROOT_1B.y, LED_ROOT_1B.b);
    if (bitRead(b9, 2)) matrix->drawPixel(LED_ROOT_1C.x, LED_ROOT_1C.y, LED_ROOT_1C.b);
    if (bitRead(b9, 3)) matrix->drawPixel(LED_ROOT_1D.x, LED_ROOT_1D.y, LED_ROOT_1D.b);
    if (bitRead(b9, 4)) matrix->drawPixel(LED_ROOT_2A.x, LED_ROOT_2A.y, LED_ROOT_2A.b);
    if (bitRead(b9, 5)) matrix->drawPixel(LED_ROOT_2B.x, LED_ROOT_2B.y, LED_ROOT_2B.b);
    if (bitRead(b9, 6)) matrix->drawPixel(LED_ROOT_2C.x, LED_ROOT_2C.y, LED_ROOT_2C.b);
    if (bitRead(b9, 7)) matrix->drawPixel(LED_ROOT_2D.x, LED_ROOT_2D.y, LED_ROOT_2D.b);
    if (bitRead(b8, 0)) matrix->drawPixel(LED_ROOT_3A.x, LED_ROOT_3A.y, LED_ROOT_3A.b);
    if (bitRead(b8, 1)) matrix->drawPixel(LED_ROOT_3B.x, LED_ROOT_3B.y, LED_ROOT_3B.b);
    if (bitRead(b8, 2)) matrix->drawPixel(LED_ROOT_3C.x, LED_ROOT_3C.y, LED_ROOT_3C.b);
    if (bitRead(b8, 3)) matrix->drawPixel(LED_ROOT_3D.x, LED_ROOT_3D.y, LED_ROOT_3D.b);
    if (bitRead(b8, 4)) matrix->drawPixel(LED_ROOT_4A.x, LED_ROOT_4A.y, LED_ROOT_4A.b);
    if (bitRead(b8, 5)) matrix->drawPixel(LED_ROOT_4B.x, LED_ROOT_4B.y, LED_ROOT_4B.b);
    if (bitRead(b8, 6)) matrix->drawPixel(LED_ROOT_4C.x, LED_ROOT_4C.y, LED_ROOT_4C.b);
    if (bitRead(b8, 7)) matrix->drawPixel(LED_ROOT_4D.x, LED_ROOT_4D.y, LED_ROOT_4D.b);

    // if an entire root is completed, turn on the corresponding branch
    if (bitRead(b9, 3) && bitRead(b9, 2) && bitRead(b9, 1) && bitRead(b9, 0))  // root 1
    {
      matrix->drawPixel(LED_BRANCH_1A.x, LED_BRANCH_1A.y, LED_BRANCH_1A.b);  // branch 1
      matrix->drawPixel(LED_BRANCH_1B.x, LED_BRANCH_1B.y, LED_BRANCH_1B.b);
      matrix->drawPixel(LED_BRANCH_1C.x, LED_BRANCH_1C.y, LED_BRANCH_1C.b);
      matrix->drawPixel(LED_BRANCH_1D.x, LED_BRANCH_1D.y, LED_BRANCH_1D.b);
    }
    if (bitRead(b9, 7) && bitRead(b9, 6) && bitRead(b9, 5) && bitRead(b9, 4))  // root 2
    {
      matrix->drawPixel(LED_BRANCH_2A.x, LED_BRANCH_2A.y, LED_BRANCH_2A.b);  // branch 2
      matrix->drawPixel(LED_BRANCH_2B.x, LED_BRANCH_2B.y, LED_BRANCH_2B.b);
      matrix->drawPixel(LED_BRANCH_2C.x, LED_BRANCH_2C.y, LED_BRANCH_2C.b);
      matrix->drawPixel(LED_BRANCH_2D.x, LED_BRANCH_2D.y, LED_BRANCH_2D.b);
    }
    if (bitRead(b8, 3) && bitRead(b8, 2) && bitRead(b8, 1) && bitRead(b8, 0))  // root 3
    {
      matrix->drawPixel(LED_BRANCH_3A.x, LED_BRANCH_3A.y, LED_BRANCH_3A.b);  // branch 3
      matrix->drawPixel(LED_BRANCH_3B.x, LED_BRANCH_3B.y, LED_BRANCH_3B.b);
      matrix->drawPixel(LED_BRANCH_3C.x, LED_BRANCH_3C.y, LED_BRANCH_3C.b);
      matrix->drawPixel(LED_BRANCH_3D.x, LED_BRANCH_3D.y, LED_BRANCH_3D.b);
    }
    if (bitRead(b8, 7) && bitRead(b8, 6) && bitRead(b8, 5) && bitRead(b8, 4))  // root 4
    {
      matrix->drawPixel(LED_BRANCH_4A.x, LED_BRANCH_4A.y, LED_BRANCH_4A.b);  // branch 4
      matrix->drawPixel(LED_BRANCH_4B.x, LED_BRANCH_4B.y, LED_BRANCH_4B.b);
      matrix->drawPixel(LED_BRANCH_4C.x, LED_BRANCH_4C.y, LED_BRANCH_4C.b);
      matrix->drawPixel(LED_BRANCH_4D.x, LED_BRANCH_4D.y, LED_BRANCH_4D.b);
    }
  }

  matrix->writeDisplay();
}

/**************************************************************/

void ISR_accelInt() {
  // Just a handler for the pin interrupt
  accel_timeout = millis() + ACCEL_TIMEOUT;  // set new starting point for automatic sleep timer

#ifdef __DEBUG
  Serial.write('+');
  // Serial.write(millis()/100);
  change_game_mode = 1;


#endif
}

/**************************************************************/

void ISR_nFlexSense() {
  // Just a handler for the pin interrupt
}

/**************************************************************/
