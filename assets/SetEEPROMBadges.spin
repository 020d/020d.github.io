'' SetEEPROMBadge.spin v1.0

CON
  _CLKMODE      = XTAL1 + PLL16X
  _XINFREQ      = 5_000_000

  EEPROM_SCL      = 28
  EEPROM_DEVID    = $A0
  EEPROM_PAGESIZE = 128

  I2C_SCL                       = 28
  EEPROMAddr                    = %1010_0000
  NVSTATUS  = $7F00

  debugMode     = 1
  LED1 = 16
  LED_BRIGHTNESS = 48

OBJ
  Serial        : "FullDuplexSerialPlusCog"
  i2c           : "basic_i2c_driver"
  leds          : "jm_pwm8"                 ' led modulation    *

VAR
  byte whichBadge, eepOffset, gotByte

PUB main
  Serial.start(31, 30, 0, 57600, 2)
  leds.start(8, LED1)                                           ' start led driver
  i2c.Initialize(I2C_SCL)

  Serial.str(string("Reading EEPROM!"))  ' Send the debug message
  serial.tx(13)

  repeat
    showStats()
    Serial.str("0-7 to toggle, 9 to exit: ")

    whichBadge := Serial.rx

    Serial.tx(whichBadge) ' ASCII digit
    Serial.tx(13)

    case whichBadge
        "0".."7":
            flipBadge(whichBadge)
        "8":
            Serial.str("No.")
        "9":
            Serial.str("Exit... rebooting.... ")
            waitcnt(cnt + clkfreq * 3)
            reboot
        other:
            Serial.str("Don't be silly")

   Serial.tx(13)


PUB flipBadge(badgeIndex)
    eepOffset := badgeIndex - "0"  ' char range verified before calling
    gotByte := i2c.ReadByte(I2C_SCL,EEPROMAddr, NVSTATUS+eepOffset)
    gotByte := gotByte ^ $FF
    i2c.WriteByte(I2C_SCL,EEPROMAddr, NVSTATUS+eepOffset,gotByte)      ' Write flipped byte

PUB showStats
    repeat eepOffset from 0 to 7
        badgeName(eepOffset)
        gotByte := i2c.ReadByte(I2C_SCL,EEPROMAddr, NVSTATUS+eepOffset)
        serial.hex(gotByte,2)
        serial.tx(13)
        if gotByte == $FF
            leds.set(eepOffset,LED_BRIGHTNESS)
        else
            leds.set(eepOffset,0)

PUB badgeName(badgeNum)
    case badgeNum
        0: Serial.str("[0] Human ... ")
        1: Serial.str("[1] Artist .. ")
        2: Serial.str("[2] Press ... ")
        3: Serial.str("[3] Speaker . ")
        4: Serial.str("[4] Vendor .. ")
        5: Serial.str("[5] Contest . ")
        6: Serial.str("[6] Goon .... ")
        7: Serial.str("[7] Uber .... ")
