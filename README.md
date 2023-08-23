# Arduino-Data-Logger-for-Lights
A data logger for measuring current, voltage and temperature.

> [!IMPORTANT]
> This project is still in it's startup phase which means that a lot of things aren't well thought through.
> The code of this project is partially based on the example project for the data logger shield from adafruit.

## Description
A data logger built on using the adafruit Datalogging Arduino Shield on an Arduino Mega 2560 R3.
3 types of different units can be measured right now: Current, Voltage, Temperature. The ammount of 
used sensors/inputs can be changed in the define section of the program. Also the right pins have to
be declared in this part. For the temperature sensor the DS18B20 is used which communicates by OneWire.
Right now only one temperature sensor can be used due to how the value is called. As for the current,
ACS712 sensors with a current limit of 20A are used. If the limit of the sensors changes the scaling
factor has to be changed. This can also be done in the define section of this program. For measureing 
the voltage a simple voltage devider is used whcih can be modfied for ervery needs. The resistor values
have to be defined in the program in order to get the right output voltage out of the mathematic formula.
Now to start and stop the data logging process the adafruit i2c 16x2 RGB LCD shield is used because
it already comes with preinstalled buttons which are already defined in their own provided libary. 
The design of the "LCD-menu" is fairely simply as it only contains start and stop as selectable options.
To use as less pins as needed the LCD is only connected with 4 wires: VCC, GND, SDA, SCL (=i2c). The
contrast can be changed with a potentiometer installed on the shield. 

## Used Libaries
* [<SPI.h>](https://www.arduino.cc/reference/en/language/functions/communication/spi/)
* [<SD.h>](https://www.arduino.cc/reference/en/libraries/sd/)
* [<Wire.h>](https://www.arduino.cc/reference/en/language/functions/communication/wire/)
* [<RTClib.h>](https://www.arduino.cc/reference/en/libraries/rtclib/)
* [<DallasTemperature.h>](https://www.arduino.cc/reference/en/libraries/dallastemperature/)
* [<OneWire.h>](https://www.arduino.cc/reference/en/libraries/onewire/)
* [<Adafruit_RGBLCDShield.h>](https://www.arduino.cc/reference/en/libraries/adafruit-rgb-lcd-shield-library/)
* [<utility/Adafruit_MCP23017.h>](https://www.arduino.cc/reference/en/libraries/adafruit-rgb-lcd-shield-library/)

## Used Shields
1. [Adafruit Data Logger Shield](https://learn.adafruit.com/adafruit-data-logger-shield/overview)
2. [Adafruit i2c 16x2 RGB LCD Shield](https://www.adafruit.com/product/714)

## Used Sensors
* [ACS712 20A current sensor](https://cdn-reichelt.de/documents/datenblatt/A300/ME067_DB-EN.pdf)
* [DS18B20 temperature sensor](https://asset.conrad.com/media10/add/160267/c1/-/en/002361335DS00/datenblatt-2361335-tru-components-tc-9445340-temperatursensor-1-st-passend-fuer-entwicklungskits-arduino.pdf)

## Voltage Devider
For measuring the voltage with the arduino we need a voltage devider as the analog inputs only can handle 0-5V. 
### Formula
$U_1=U_{ges}*\frac{R_1}{(R_1+R_2)}$
### Code
The following code includes the formula for converting the input from the voltage devider into the real value:
```
float rawValue = analogRead(voltageSensorPins[i]);
float voltage = rawValue * (5.0/1023.0) * ((R1+R2)/R2);
```

## Code Walkthrough
### Defines
* `ORIG_LOG_INTERVAL` defines the milliseconds for the sampling rate after starting the arduino
* `SYNC_INTERVAL` is how many milliseconds between reading the data and writing it to the .csv file
* `THRESHOLD_CURRENT` when set to **'1'** then the sampling rate changes to the defined value after reaching
  the defined threshold for the current sensors (same for the other 3 threshold defines)
* `INTERVAL_OPT` is the changed sampling rate after reaching one of the thresholds
* `ECHO_TO_SERIAL` when set to **'1'** every output (what gets written into the file) is written to the serial output terminal (use for testing/debugging purposes)
* `redLEDpin` defines the pin where the "errorLed" is connected to (same for the grren LED)
* `TEMP_SENSORS` when set to **'1'** the temperature sensor values are beeing read and written to the file (set to **'0'** if no temp. sensor is connected)
* `ECHO_TO_LCD` when set to **'1'** all error massages are beeing displayed on the LCD screen (set to **'1'** by default)

### Functions
#### error()
```
void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  digitalWrite(redLEDpin, 1);
  #if ECHO_TO_LCD
    lcd.clear();
    lcd.print("error: ");
    lcd.println(str);
    while(1)
    {
      for(uint8_t i= 0; i < 25; i++)
      {
        lcd.scrollDisplayLeft();
        delay(500);
      }
      lcd.home();
    }
  #endif
  
  // red LED indicates error
  digitalWrite(redLEDpin, HIGH);

  while(1);
}
```
  
