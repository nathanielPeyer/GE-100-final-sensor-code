#include <Arduino.h>
#include <stdio.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define MICRO 1000000
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define YELLOW_LED  13
#define BLUE_LED  12
#define RED_LED  11
#define TAP_SENSOR 2
#define PRESENCE_SENSOR 3
#define TEMP_SENSOR A0
#define PHOTO_SENSOR A1
#define DARKNESS_LEVEL_ONE 22*MICRO/10
#define DARKNESS_LEVEL_TWO 2*MICRO/10
#define PRESENT 1
#define LOADED 1
#define NOT_LOADED 0


int timeInner = 0;
int timeOuter = 0;
int darkness = 4;
int previousDarkness = 4;
int loadState = NOT_LOADED;
int previousLoadState = NOT_LOADED;
int presenceSensor = -1;
int timeOfLoad = -1;


long int intialTempuDF = 0;
long int tempSensoruVolts;
long int tempSensoruDC;
long int tempSensoruDF;
long int tempAvg;
long int photoSensorVolt;
long int photoSensoruVolt;


char tempOut[100];

long int tempDFStorage[10];
//long int tempDFTempStorage[10];

bool isLoad = false;
bool isNear = false;


//char isDark[30];

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


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
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  digitalWrite(BLUE_LED, HIGH);
  display.display();
  digitalWrite(BLUE_LED, LOW);
  delay(1000);
}

void loop() 
{
  //setting up display
  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  
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

  //checking if there is a load
  if(presenceSensor == PRESENT)
  {
    // defining load as true.
    //storing values for 3 minutes before being written over.
    if(!isNear)
    {
      isNear = true;
      snprintf(tempOut, sizeof(tempOut) - 1, "Load near bridge!");
      Serial.println(tempOut);
      display.println(tempOut);
    }
  }
  else
  {
    isNear = false;
  }
  
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
    display.println(tempOut);
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
    Serial.println("Really Dark");
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
    Serial.println("Changed from day to dusk.");
    //display.println("Changed from day to dusk");
  }
  else if((previousDarkness != 2) && (darkness == 2))
  {
    Serial.println("It is totally dark.");
    //display.println("It is totally dark.");
  }
  else if((previousDarkness > 0) && (darkness == 0))
  {
    digitalWrite(RED_LED, LOW);
    Serial.println("It is day.");
    //display.print("It is day.");
  }
  previousDarkness = darkness;

  //checking for load using tap sensor and debounce it
  if(loadState != previousLoadState)
  {
    if((loadState == LOADED) && (presenceSensor == PRESENT))
    {
      Serial.println("something");
      if(isLoad == false)
      {
        Serial.println("Load on bridge!");
        //display.println("Load on bridge!");
        isLoad = true;
        timeOfLoad = timeOuter;
      }
      else if(timeOfLoad +5 >= timeOuter)
      {
        digitalWrite(YELLOW_LED, HIGH);
        Serial.println("A second load has been detected within 5 seconds of the first");
        //display.println("Second Load");
      }
      else
      {
        digitalWrite(YELLOW_LED, LOW);
        isLoad = false;
      }
      
    }
  }
  previousLoadState = loadState;

  //waiting a second before going through loop again.
  timeOuter++;
  display.display();
  delay(1000);
}