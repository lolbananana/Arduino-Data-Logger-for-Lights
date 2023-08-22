# Arduino-Data-Logger-for-Lights
A data logger for measuring current, voltage and temperature.

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
* <SPI.h>
* <SD.h>
* <Wire.h>
* <RTClib.h>
* <DallasTemperature.h>
* <OneWire.h>
* <Adafruit_RGBLCDShield.h>
* <utility/Adafruit_MCP23017.h>

## Used Shields
1. [Adafruit Data Logger Shield](https://learn.adafruit.com/adafruit-data-logger-shield/overview)
2. [Adafruit i2c 16x2 RGB LCD Shield](https://www.adafruit.com/product/714)
