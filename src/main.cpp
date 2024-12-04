#include <Arduino.h>
#include <stdio.h>
#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiAvrI2c.h>

#define MICRO 1000000
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
#define LIGHT_SENSOR_ROW 2
#define TOTAL_DARK_ROW 3
#define LOAD_PRESENT_ROW 4
#define LOAD_ON_ROW 5
#define SECOND_LOAD_ROW 6
#define TIME_ROW 7


#if 1
# define DISPLAY_PRINTLN(MSG) simpleDisplay.println(MSG)
# define DISPLAY_PRINT(MSG) simpleDisplay.print(MSG)
#else
# define DISPLAY_PRINTLN(MSG)
# define DISPLAY_PRINT(MSG)
#endif

bool isLoad = false;
bool isNear = false;

int timeInner = 0;
int timeOuter = 0;
int darkness = 4;
int previousDarkness = 4;
int loadState = NOT_LOADED;
int previousLoadState = NOT_LOADED;
int presenceSensor = NOT_PRESENT;
int previousPresenceSensor = NOT_PRESENT;
int timeOfLoad = -1;

long int intialTempuDF = 0;
long int tempSensoruVolts;
long int tempSensoruDC;
long int tempSensoruDF;
long int tempAvg;
long int photoSensorVolt;
long int photoSensoruVolt;

long int tempDFStorage[10];

char tempOut[120];

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

  digitalWrite(BLUE_LED, HIGH);
  delay(1000);
}

void loop() 
{
  //setting up display
  simpleDisplay.setCol(0);
  //resetting lights
  digitalWrite(YELLOW_LED, LOW);
  //todo check and make sure LED needs top be reset.
  digitalWrite(BLUE_LED, LOW);

  //resetting some variables
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

  tempDFStorage[timeOuter%10] = tempSensoruDF;
  //checking for +- 5 degrees from starting temp
  if(timeOuter == 0)
  {
    intialTempuDF = tempSensoruDF;
  }
  if((tempSensoruDF >= (intialTempuDF+5*MICRO)) || (tempSensoruDF) <= (intialTempuDF-5*MICRO))
  {
    digitalWrite(BLUE_LED, HIGH);
  }

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
  
  //assigning values to darkness levels
  /*Serial.print(photoSensoruVolt/MICRO);
  Serial.print(".");
  Serial.println(photoSensoruVolt%MICRO);*/
  if(photoSensoruVolt > DARKNESS_LEVEL_ONE)
  {
    darkness = 0;
  }
  else if( photoSensoruVolt < DARKNESS_LEVEL_TWO)
  {
    darkness = 2;
    //Serial.println("Really Dark");
    //display.println("Really Dark");
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


  //checking for load using tap sensor and debounce it

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


  /*
  Serial.print("Load state: ");
  Serial.println(loadState);
  Serial.print("Previous load state: ");
  Serial.println(previousLoadState);
  Serial.print("presence State: ");
  Serial.println(presenceSensor);
  Serial.print("Previous presence state: ");
  Serial.println(previousPresenceSensor);
  */

  previousLoadState = loadState;
  previousPresenceSensor = presenceSensor;

  //waiting a second before going through loop again.
  timeOuter++;
  Serial.println(timeOuter);
  simpleDisplay.setRow(TIME_ROW);
  DISPLAY_PRINT("Time: ");
  DISPLAY_PRINTLN(timeOuter);
  
  delay(1000);
}