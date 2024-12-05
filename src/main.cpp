#include <Arduino.h>            //allows for arduino control
#include <stdio.h>              //standard c/c++ input output header file
#include <Wire.h>               //used for I2C communication
#include <SSD1306Ascii.h>       //new libary used
#include <SSD1306AsciiAvrI2c.h> //new libary used

// quantity's that dont change and are stored in the flash memory
#define MICRO 1000000   //used for converting values so they line up with micro values from calcs
#define SCREEN_ADDRESS 0x3C
#define YELLOW_LED  12
#define BLUE_LED  11
#define RED_LED  13
#define TAP_SENSOR 4
#define PRESENCE_SENSOR 3
#define TEMP_SENSOR A0
#define PHOTO_SENSOR A1
#define DARKNESS_LEVEL_ONE 22*MICRO/10
#define DARKNESS_LEVEL_TWO 2*MICRO/10
#define PRESENT 0
#define NOT_PRESENT 1
#define LOADED 1
#define NOT_LOADED 0
#define TEMP_SENSOR_LOAD 0 //Takes two lines
#define LIGHT_SENSOR_ROW 2 //Takes one line
#define TOTAL_DARK_ROW 3   //Takes one line
#define LOAD_PRESENT_ROW 4 //Takes one line
#define LOAD_ON_ROW 5      //Takes one line
#define SECOND_LOAD_ROW 6  //Takes one line
#define TIME_ROW 7         //Takes one line

//switch used to toggle OLED display to turn off display replace 1 with 0.
#if 1
# define DISPLAY_PRINTLN(MSG) simpleDisplay.println(MSG)
# define DISPLAY_PRINT(MSG) simpleDisplay.print(MSG)
#else
# define DISPLAY_PRINTLN(MSG)
# define DISPLAY_PRINT(MSG)
#endif

//defining booleans for latter use and initlazying to false.
bool isLoad = false;
bool isNear = false;
//defining ints for latter use and initlazying
int timeInner = 0;
int timeOuter = 0;
int darkness = 4;
int previousDarkness = 4;
int loadState = NOT_LOADED;
int previousLoadState = NOT_LOADED;
int presenceSensor = NOT_PRESENT;
int previousPresenceSensor = NOT_PRESENT;
int timeOfLoad = -1;

//defining long ints for int calculations latter so no floating point needed.
long int intialTempuDF = 0;
long int tempSensoruVolts;
long int tempSensoruDC;
long int tempSensoruDF;
long int tempAvg;
long int photoSensorVolt;
long int photoSensoruVolt;

//used for finding average temp over 10 sec
long int tempDFStorage[10];

//This is really a string and is used for some of the print statements
char tempOut[120];

//building an object of the display
SSD1306AsciiAvrI2c simpleDisplay;

void setup() 
{
  Serial.begin(9600);
  
  //setting up output pins
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  //setting up input pins
  pinMode(TAP_SENSOR, INPUT);
  pinMode (PRESENCE_SENSOR, INPUT);
  pinMode(TEMP_SENSOR, INPUT);
  pinMode(PHOTO_SENSOR, INPUT);

  //setting up display for later
  simpleDisplay.begin(&Adafruit128x64, SCREEN_ADDRESS);
  simpleDisplay.setFont(System5x7);
  Serial.println("display is starting up");
  simpleDisplay.clear();
  //used to ensure display was working
  delay(1000);
}

void loop() 
{
  //setting up display
  simpleDisplay.setCol(0);

  //resetting lights
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(BLUE_LED, LOW);

  //resetting darkness value
  darkness = 4;

  //scanning in all data and converting to proper values
  tempSensoruVolts = (long)analogRead(A0);
  photoSensorVolt = (long)analogRead(A1);
  loadState = digitalRead(TAP_SENSOR);
  presenceSensor = digitalRead(PRESENCE_SENSOR);

  //conversions
  tempSensoruVolts = tempSensoruVolts*(5000000/1024);
  photoSensoruVolt = photoSensorVolt*(5000000/1024);
  tempSensoruDC = (tempSensoruVolts-500000)*100;
  tempSensoruDF = tempSensoruDC*9/5+32000000;

  //writting over old temp data in average array
  tempDFStorage[timeOuter%10] = tempSensoruDF;

  //setting initial temp first time through the loop.
  if(timeOuter == 0)
  {
    intialTempuDF = tempSensoruDF;
  }
  //checking for temp change of +- 5 degress
  if((tempSensoruDF >= (intialTempuDF+5*MICRO)) || (tempSensoruDF) <= (intialTempuDF-5*MICRO))
  {
    digitalWrite(BLUE_LED, HIGH);
  }

  //printing temp and avg temp every 10 sec
  if((timeOuter%10 == 0) && (timeOuter != 0))
  {
    tempAvg = 0;
    
    for(int i = 0; i < 10; i++)
    {
      tempAvg = tempAvg + tempDFStorage[i];
    }
    tempAvg = tempAvg/10;
    snprintf(tempOut, sizeof(tempOut) - 1, "The current temperature is %ld.%06ld F, The average temperature over the last ten seconds is %ld.%06ld F.",
             tempSensoruDF/MICRO, tempSensoruDF%MICRO, tempAvg/MICRO, tempAvg%MICRO);
    Serial.println(tempOut);
    snprintf(tempOut, sizeof(tempOut) - 1, "Cur temp: %ld.%06ldF\nAvg temp: %ld.%06ldF",tempSensoruDF/MICRO, tempSensoruDF%MICRO, tempAvg/MICRO, tempAvg%MICRO);
    simpleDisplay.setRow(TEMP_SENSOR_LOAD);
    DISPLAY_PRINTLN(tempOut);
  }

  /*
    Darkness scale
    0 - day
    1 - semi-dark/dusk
    2 - dark
    4 - default state
  */
  
  //setting darkness value to correct value as seen in above table
  if(photoSensoruVolt > DARKNESS_LEVEL_ONE)
  {
    darkness = 0;
  }
  else if( photoSensoruVolt < DARKNESS_LEVEL_TWO)
  {
    darkness = 2;
  }
  else
  {
    darkness = 1;
  }
  
   //checking if it went from day to dusk.
  if((previousDarkness == 0) && (darkness == 1))
  {
    digitalWrite(RED_LED, HIGH);
    Serial.println(("Changed from day to dusk."));
    simpleDisplay.setRow(LIGHT_SENSOR_ROW);
    DISPLAY_PRINTLN("Day to Dusk. ");
  }
  else if((previousDarkness != 2) && (darkness == 2))
  {
    Serial.println(("It is totally dark."));
    simpleDisplay.setRow(TOTAL_DARK_ROW);
    DISPLAY_PRINTLN("It is totally dark.");
  }
  else if((previousDarkness > 0) && (darkness == 0))
  {
    digitalWrite(RED_LED, LOW);
    simpleDisplay.setRow(LIGHT_SENSOR_ROW);
    Serial.println(("It is day.   "));
    DISPLAY_PRINTLN("It is day.   ");
  }
  previousDarkness = darkness;

  //checking for load using tap sensor and presence using tracking sensor, there must be an object present for there to be a load
  if((loadState != previousLoadState) || (presenceSensor !=previousPresenceSensor ))
  {
    snprintf(tempOut, sizeof(tempOut)-1, "Load state changed to: %d prescense is: %d", loadState, presenceSensor);
    Serial.println(tempOut);
    if(presenceSensor == PRESENT)
    {
      if(previousPresenceSensor != PRESENT)
      {
        Serial.println("Load near bridge!");
        simpleDisplay.setRow(LOAD_PRESENT_ROW);
        DISPLAY_PRINTLN("Load near bridge!");
      }
      
      if(loadState == LOADED)
      {
        if(isLoad == false)
        {
          Serial.println(("Load on bridge!"));
          simpleDisplay.setRow(LOAD_ON_ROW);
          DISPLAY_PRINTLN("Load on bridge!");
          isLoad = true;
          timeOfLoad = timeOuter;
        }
        else if(timeOfLoad + 5 >= timeOuter)
        {
          digitalWrite(YELLOW_LED, HIGH);
          Serial.println(("A second load has been detected within 5 seconds of the first."));
          simpleDisplay.setRow(SECOND_LOAD_ROW);
          DISPLAY_PRINTLN("Second Load!");
        }
        else
        {
          simpleDisplay.setRow(SECOND_LOAD_ROW);
          //needs to be the same number of characters as the line it overrides
          DISPLAY_PRINTLN("            ");
        }
      }
    }
    else if(loadState != LOADED)
    {
      digitalWrite(YELLOW_LED, LOW);
      Serial.println("Load not Present!");
      simpleDisplay.setRow(LOAD_PRESENT_ROW);
      DISPLAY_PRINTLN("Load not present!");
      Serial.println("Load removed!  ");
      simpleDisplay.setRow(LOAD_ON_ROW);
      DISPLAY_PRINTLN("Load removed!  ");
      isLoad = false;
    } 
  }

  //updating previous values for next time through the loop
  previousLoadState = loadState;
  previousPresenceSensor = presenceSensor;

  //waiting a second before going through loop again and printing the time to console and display
  timeOuter++;
  Serial.println(timeOuter);
  simpleDisplay.setRow(TIME_ROW);
  DISPLAY_PRINT("Time: ");
  DISPLAY_PRINTLN(timeOuter);
  delay(1000);
}