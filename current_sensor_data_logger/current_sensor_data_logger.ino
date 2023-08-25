//---- DATA LOGGER ----/////////////////////////////////////////////////////////////////////////////////////////
/*
* AUTHOR: Daniel Reithofer
* REVISION: 1.0
* DESCRIPTION: 
*   A data logger built on using the adafruit Datalogging Arduino Shield on an Arduino Mega 2560 R3.
*   3 types of different units can be measured right now: Current, Voltage, Temperature. The ammount of 
*   used sensors/inputs can be changed inthe define section of the program. Also the right pins have to
*   be declared in this part. For the temperature sensor the DS18B20 is used which communicates by OneWire.
*   Right now only one temperature sensor can be used due to how the value is called. As for the current,
*   ACS712 sensors with a current limit of 20A are used. If the limit of the sensors changes the scaling
*   factor has to be changed. This can also be done in the define section of this program. For measureing 
*   the voltage a simple voltage devider is used whcih can be modfied for ervery needs. The resistor values
*   have to be defined in the program in order to get the right output voltage out of the mathematic formula.
*   Now to start and stop the data logging process the adafruit i2c 16x2 RGB LCD shield is used because
*   it already comes with preinstalled buttons which are already defined in their own provided libary. 
*   The design of the "LCD-menu" is fairely simply as it only contains start and stop as selectable options.
*   To use as less pins as needed the LCD is only connected with 4 wires: VCC, GND, SDA, SCL (=i2c). The
*   contrast can be changed with a potentiometer installed on the shield. 
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---- USED LIBARIES ----///////////////////
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
////////////////////////////////////////////

//---- SYNCHRONISATION ----///////////////////////////////////////////////////////////////
/*    --SYNC_INTERVAL--
* how many milliseconds before writing the logged data permanently to disk
* set it to the LOG_INTERVAL to write each time (safest)
* set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to 
* the last 10 reads if power is lost but it uses less power and is much faster!
*/
// how many milliseconds between grabbing data and logging it. 1000 ms is once a second
#define LOG_INTERVAL  1000 // mills between entries (reduce to take more/faster data)
#define SYNC_INTERVAL 1000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()
///////////////////////////////////////////////////////////////////////////////////////////


#define ECHO_TO_SERIAL   1 // echo data to serial port

// the digital pins that connect to the LEDs
#define redLEDpin 2
#define greenLEDpin 24

//the adafruit shield uses the PCF8523 RTC
RTC_PCF8523 RTC; // define the Real Time Clock object

// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;

// strats and stopps the data logging process
int startLogging = 0; // the sd card always gets initialialized independed from this value

//---- Current Sensors ----////////////////////////////////////////////////////////////////////////////////////////////////
const int numCurrentSensors = 1; //change depending on how much sensors you use
const int currentSensorPins[numCurrentSensors] = {A8}; //defines wich analog pins are used and in which order
const float scaleFactor = 0.100; //sets the scaling-factor of the current Sensor (20A Version: 0.100, 30A Version: 0.185)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---- Voltage Sensors ----////////////////////////////////////////////////////////////////////////////////////////////////
const int numVoltageSensors = 1; //change depending on how much sensors you use
const int voltageSensorPins[numVoltageSensors] = {A15}; //defines wich analog pins are used and in which order
const int R1 = 1600;
const int R2 = 200;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---- Temperature Sensors ----/////////////////////////////////////////////////////////////
const int oneWireBus = 24; // data pin of the temperature sensor
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
const int TEMP_RESOLUTION = 9; // sets the resolution of the temperature sensor (9-12bit)
DeviceAddress insideThermometer; // array for the device address of the temperature sensor
////////////////////////////////////////////////////////////////////////////////////////////

//---- LCD-Display ----//////////////////////////////////
enum { ELEMENT_START, ELEMENT_STOP };
int currentElement = ELEMENT_START;
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
#define ECHO_TO_LCD      1 // echo error to lcd display
////////////////////////////////////////////////////////


// the logging file
File logfile; // uses the SD libary

//if an error occours the massage gets sent as a string to this function 
//when ECHO_TO_LCD is set to 1 the error-massage also gets displayed on the LCD
void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
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

//used to set the startup screen for the LCD
void setupLcd()
{
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  lcd.print("DATA LOGGER 1.0");
  lcd.setCursor(0, 1);
  lcd.print(" >START   STOP ");
}

void welcomeLcd()
{
  lcd.begin(16, 2);
  lcd.setCursor(4, 0);
  lcd.print("BOOTING");
  lcd.setCursor(1, 1);
  lcd.print("**DATALOGGER**");
}

void setup(void)
{
  Serial.begin(9600);
  Serial.println();

  welcomeLcd();
  delay(2000); //wait 2 sec
  setupLcd();

  // use debugging LEDs
  pinMode(redLEDpin, OUTPUT); //the red LED indicates an error
  pinMode(greenLEDpin, OUTPUT); //the green LED indicates that the logging has started and that the sd card was recognised

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
  
  // create a new file
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

  // connect to RTC
  Wire.begin();  
  if (!RTC.begin()) {
    logfile.println("RTC failed"); // checks if the real time clock is running
#if ECHO_TO_SERIAL
    Serial.println("RTC failed");
#endif  //ECHO_TO_SERIAL
  }
  
  //this prints the first row of the .csv file to indicate which value stands for which sensor
  //this has to updated manually if the amount of sensors changes
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
  logfile.print(",temp1");

#if ECHO_TO_SERIAL
  Serial.print("millis,datetime");
  for(int i=0; i<numCurrentSensors; i++)
  {
    Serial.print(",current");
    Serial.print((i+1));
  }

  for(int i=0; i<numVoltageSensors; i++)
  {
    Serial.print(",voltage");
    Serial.print((i+1));
  }
  Serial.print(",temp1");

#endif //ECHO_TO_SERIAL
 
  logfile.println();
#if ECHO_TO_SERIAL
    Serial.println();
#endif  //ECHO_TO_SERIAL

  // this line sets the RTC to the date & time this sketch was compiled 
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__))); // always be shure the time on the pc is set correctly and synchronised automatically


  sensors.setResolution(insideThermometer, TEMP_RESOLUTION);
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
}

//reads the button values of the adafruit LCD shield and executes the updateDisplay() function
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

void loop(void)
{
  readButtons();

  if(startLogging){
    sensors.requestTemperaturesByAddress(insideThermometer);
    dataLogger();
  }
}

void dataLogger(){

  //---- RTC -----------------
  DateTime now;
  //--------------------------

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

  // fetch the time
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
#if ECHO_TO_SERIAL
  Serial.print('"');
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.print('"');
#endif //ECHO_TO_SERIAL
  
  //***** reading sensor values *****
  //always be shure it's the right order because it has to match with the first row of the .csv file
    currentSensors();
    voltageSensors();
    tempSensors(insideThermometer);
  //**********************************
  

  logfile.println();
#if ECHO_TO_SERIAL
  Serial.println();
#endif // ECHO_TO_SERIAL

  digitalWrite(greenLEDpin, LOW);

  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
  
  // blink LED to show we are syncing data to the card & updating FAT!
  digitalWrite(redLEDpin, HIGH);
  logfile.flush();
  digitalWrite(redLEDpin, LOW);
}

//reads the analog inputs of the voltageSensors, converts them to the right voltage value
//and writes them into the .csv file
void voltageSensors()
{
  //reading each sensor value and writing them in the .csv file
  for(int i=0; i < numVoltageSensors; i++)
  {
    float rawValue = analogRead(voltageSensorPins[i]);
    float voltage = rawValue * (5.0/1023.0) * ((R1+R2)/R2);

    logfile.print(", ");    
    logfile.print(voltage, 3);
    #if ECHO_TO_SERIAL
    Serial.print(", ");   
    Serial.print(voltage, 3);
    #endif //ECHO_TO_SERIAL
   
  }
}

//reads the analog inputs of the temperatureSensors, converts them to the right temperature value
//and writes them into the .csv file
void tempSensors(DeviceAddress deviceAddress)
{
  float temperatureC = sensors.getTempC(deviceAddress); // if only one DS18B20 is used
  if(temperatureC == DEVICE_DISCONNECTED_C) 
  {
    error("couldn't read temperature data");
    return;
  }
  logfile.print(", ");    
  logfile.print(temperatureC);
  #if ECHO_TO_SERIAL
  Serial.print(", ");   
  Serial.print(temperatureC);
  #endif //ECHO_TO_SERIAL
 
} 

//reads the analog inputs of the currentSensors, converts them to the right current value
//and writes them into the .csv file
void currentSensors()
{
  for(int i=0; i < numCurrentSensors; i++)
  {
    float rawValue = analogRead(currentSensorPins[i]);
    float current = (2.5 - (rawValue * (5.0 / 1024.0)) )/scaleFactor;;

    logfile.print(", ");    
    logfile.print(current, 3);
    #if ECHO_TO_SERIAL
    Serial.print(", ");   
    Serial.print(current, 3);
    #endif //ECHO_TO_SERIAL
   
  }
}

//updates the LCD-display when a button is pressed 
void updateDisplay() {
  lcd.setCursor(1, 1);
  if (currentElement == ELEMENT_START) {
    lcd.print(">");
    lcd.setCursor(9, 1);
    lcd.print(" ");
  } else {
    lcd.setCursor(1, 1);
    lcd.print(" ");
    lcd.setCursor(9, 1);
    lcd.print(">");
  }
}

//executes a instant reset by setting address 0
void(* resetFunc) (void) = 0;//declare reset function at address 0

//when a element on the LCD-display is selcted and the SELECT-Button is 
//pressed the data logger is beeing started or stopped
void executeSelectedFunction() {
  lcd.clear();
  
  if (currentElement == ELEMENT_START) {
    lcd.setCursor(2, 0);
    lcd.print("Logging has ");
    lcd.setCursor(4, 1);
    lcd.print("started");
    startLogging = 1;
    delay(2000);
  } else {
    lcd.setCursor(4, 0);
    lcd.print("Stopped");
    startLogging = 0;
    delay(2000);
    resetFunc();
  }
  
  //shows the massage for 2 seconds
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DATA LOGGER 1.0");
  lcd.setCursor(0, 1);
  lcd.print(" >START   STOP ");
}
