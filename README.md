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
* [Adafruit Data Logger Shield](https://learn.adafruit.com/adafruit-data-logger-shield/overview)
* [Adafruit i2c 16x2 RGB LCD Shield](https://www.adafruit.com/product/714)

## Used Sensors
* [ACS712 20A current sensor](https://cdn-reichelt.de/documents/datenblatt/A300/ME067_DB-EN.pdf)
* [DS18B20 temperature sensor](https://asset.conrad.com/media10/add/160267/c1/-/en/002361335DS00/datenblatt-2361335-tru-components-tc-9445340-temperatursensor-1-st-passend-fuer-entwicklungskits-arduino.pdf)

## Voltage Devider
For measuring the voltage with the arduino we need a voltage devider as the analog inputs only can handle 0-5V. 
### Formula
$U_1=U_{ges}*\frac{R_1}{(R_1+R_2)}$
### Code
The following code includes the formula for converting the input from the voltage devider into the real value:
```c++
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
* `redLEDpin` defines the pin where the "errorLed" is connected to (same for the green LED)
* `TEMP_SENSORS` when set to **'1'** the temperature sensor values are beeing read and written to the file (set to **'0'** if no temp. sensor is connected)
* `ECHO_TO_LCD` when set to **'1'** all error massages are beeing displayed on the LCD screen (set to **'1'** by default)

### Functions
#### error()
```c++
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
The `error()` function recieves a string with the **error massage** in it which then gets printed into the serial output and to the LCD if `ECHO_TO_LCD`
is set to anything but **'0'**.It turns on the red LED and sits in an infinite loop (`while(1);`).

#### setupLcd()
```c++
void setupLcd()
{
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  lcd.print("DATA LOGGER 1.0");
  lcd.setCursor(0, 1);
  lcd.print(" >START   STOP ");
}
```
`setupLcd()` is used to set the default menu screen on the LCD (`welcomeLcd()` is built the same way and shows a booting screen for 1.5 sec)

#### setup()
```c++
void setup(void)
{
  Serial.begin(9600);
  Serial.println();

  // initialize the SD card
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");
```
This code initializes the SD card and sends an error if no card is inserted or the pin is corrupted.

#### creating a new file
```c++
char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  if (! logfile) {
    error("couldnt create file");
  }
  
  Serial.print("Logging to: ");
  Serial.println(filename); //writes the current Filename into the serial output
```
This creates a new file every time the data logger is started/stopped. The index of `filename[]` gets increased every time the filename already exists. 

#### initializing the RTC
```c++
  Wire.begin();  
  if (!RTC.begin()) {
    logfile.println("RTC failed"); // checks if the real time clock is running
#if ECHO_TO_SERIAL
    Serial.println("RTC failed");
#endif  //ECHO_TO_SERIAL
  }
```
Sends an **error** if the Real Time Clock isn't running properly.

#### printing the .csv headline
```c++
logfile.print("millis,datetime");
  for(int i=0; i<numCurrentSensors; i++)
  {
    logfile.print(",current");
    logfile.print(i);
  }

  for(int i=0; i<numVoltageSensors; i++)
  {
    logfile.print(",voltage");
    logfile.print((i+1));
  }
  #if TEMP_SENSORS
  logfile.print(",temp1");
  #endif
```
The ammount of collumns changes depending on how much sensor are beeing used. `#if TEMP_SENSORS` checks if a temp. sensor is connected.

#### initialize temperature sensor
```c++
  #if TEMP_SENSORS
  sensors.setResolution(insideThermometer, TEMP_RESOLUTION);
  if (!sensors.getAddress(insideThermometer, 0)) error("Unable to find address for Device 0"); 
  #endif
```
If **no** temp sensor is connected but you forgot to change `TEMP_SENSORS` to **'0'** then this error message appears.  

#### readButtons()
```c++
void readButtons()
{
  uint8_t buttons = lcd.readButtons();
  
  if (buttons) {
    if (buttons & BUTTON_LEFT) {
      currentElement = ELEMENT_START;
      updateDisplay();
    }
    if (buttons & BUTTON_RIGHT) {
      currentElement = ELEMENT_STOP;
      updateDisplay();
    }
    if (buttons & BUTTON_SELECT) {
      executeSelectedFunction();
    }
  }
}
```
This code uses the [<Adafruit_RGBLCDShield.h>](https://www.arduino.cc/reference/en/libraries/adafruit-rgb-lcd-shield-library/) libary to easily
read the buttons of the LCD shield. 

#### loop()
```c++
void loop(void)
{
  readButtons();

  if(startLogging){
    #if TEMP_SENSORS
    sensors.requestTemperaturesByAddress(insideThermometer);
    #endif
    dataLogger();
  }
}
```
The `loop()` function only loops the button requests and starts the data logger when it was selected from the LCD menu. To save some time, 
the temperature is **only** read if the sensor is connected. 

#### dataLogger()

```c++
void dataLogger(){
  DateTime now;

  // delay for the amount of time we want between readings
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
  
  digitalWrite(greenLEDpin, HIGH); //indicates that the data logging has started
  
  // log milliseconds since the uC has started
  uint32_t m = millis();
  logfile.print(m);           // milliseconds since start
  logfile.print(", ");    
#if ECHO_TO_SERIAL
  Serial.print(m);         // milliseconds since start
  Serial.print(", ");  
#endif
```
`DateTime now;` sets the RTC to the date/time the computer is using. Also the millisenconds since 
the arduino has **started** are being printed in the first collumn of the file. After approximately **50** days the `millis()` function 
reaches an overflow and is set to **'0'** again. 

#### printing the date/time
```c++
now = RTC.now();
  // log time
  logfile.print('"');
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print('"');
```

#### writing to the SD
```c++
 if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
```
Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card which uses a bunch of **power** and takes **time**.

#### reading sensor values
```c++
void voltageSensors()
{
  for(int i=0; i < numVoltageSensors; i++)
  {
    float rawValue = analogRead(voltageSensorPins[i]);
    float voltage = rawValue * (5.0/1023.0) * ((R1+R2)/R2);

    logfile.print(", ");    
    logfile.print(voltage, 3);

    #if THRESHOLD_VOLTAGE
      if(voltage >= voltageThreshold)
        LOG_INTERVAL = INTERVAL_OPT;
      else
        LOG_INTERVAL = ORIG_LOG_INTERVAL;
    #endif
  }
}
```
This function reads the analog inputs and converts the readings into usable values. `#if THRESHOLD_VOLTAGE` **changes** when selected the **sampling intervall**.
The functions for the other sensors are built the same way. 

## Using the data logger
Now after uploading the whole sketch the LCD will show the booting screen and then the menu. After selecting **"Start"** the arduino will start logging the data. 
If you set `ECHO_TO_SERIAL` to **'1'** this is what the serial output should look like:

![serial output](/images/serial_output.png)

Currently nothing's plugged in on the sensors but the current sensor still shows some values that aren't **0**. 
Thats because these ACS712 sensors aren't that accurate but once you plugged in a current the values will be pretty "stable".

Once you have checked that the data logger is running properly it's time to log the data and evaluate it. 

### Start logging
First it's important to check that the defined RTC matches with the one on our shield:
```c++
//the adafruit shield uses the PCF8523 RTC
RTC_PCF8523 RTC; // define the Real Time Clock object
```
Then insert a SD card and press **Start** as shown on the LCD:

![LCD menu screen](/images/grafik.png)
> Source: [WOKWI Simualtion](https://wokwi.com/projects/307244658548802112)

