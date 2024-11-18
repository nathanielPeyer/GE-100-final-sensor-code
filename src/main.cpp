#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define MICRO 1000000
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3c

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int YELLOWLED = 13;
const int BLUELED = 12;
const int TAPSENSOR = 1;

int timeInner1 = 0;
int timeInner2 = 0;
int timeOuter = 0;

long int tempSensoruVolts;
long int tempSensoruDC;
long int tempSensoruDF;
long int photoSensorVolt;
long int photoSensoruVolt;
long int touchSensorVolts;
long int tapState;

long int tempDFStorage[18];
long int tempDFTempStorage[10];

bool isDark;
bool isLoad;
bool isVibration;
bool isLoad1;
bool isLoad2;

bool isDarkStorage[180];
bool isLoadStorage[180];
bool isVibrationStorage[180];



void setup() 
{
  Serial.begin(9600);

  //setting up display for later
  display.begin(SSD1306_SWITCHCAPVCC,SCREEN_ADDRESS);
  display.clearDisplay();

  //setting up output pins
  pinMode(YELLOWLED, OUTPUT);
  pinMode(BLUELED, OUTPUT);
  
  //setting up input pins
  pinMode(TAPSENSOR, INPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
}

void loop() 
{
  //resetting lights
  digitalWrite(YELLOWLED, LOW);
  digitalWrite(BLUELED, LOW);
  
  //shift data in storage arrays so there is an open slot.
  if(timeOuter == 180)
  {
    for(int i = 1; i < 180; i++)
    {
      isDarkStorage[i-1] = isDarkStorage[i];
      isLoadStorage[i-1] = isLoadStorage[i];
      isVibrationStorage[i-1]= isVibrationStorage[i];
    }
    for(int i = 1; i < 18; i++)
    {
      tempDFStorage[i-1] = tempDFStorage[i];
    }
    for(int i = 1; i < 10; i++)
    {
      tempDFTempStorage[i-1] = tempDFStorage[i];
    }
    timeOuter--;
  }

  //scanning in all data and converting to proper values
  tempSensoruVolts = (long)analogRead(A0);
  photoSensorVolt = (long)analogRead(A1);
  touchSensorVolts = (long)analogRead(A2);
  tapState = (long)digitalRead(TAPSENSOR);

  //conversions
  tempSensoruVolts = tempSensoruVolts*(5000000/1024);
  photoSensoruVolt = photoSensorVolt*(5000000/1024);
  touchSensorVolts = touchSensorVolts*(5000000/1024);
  tempSensoruDC = (tempSensoruVolts-500000)*100;
  tempSensoruDF = tempSensoruDC*9/5+32000000;

  //checking if there is a load
  if(touchSensorVolts < 2.5*MICRO)
  {
    timeInner1++;
    if(timeInner1 >= 6)
    {
      timeInner1 = 1;
      isLoad1 = false;
      isLoad2 = false;
    }
    // defining load as true.
    //storing values for 3 minutes before being written over.
    isLoad = true;
    isLoadStorage[timeOuter]= isLoad;
    //checks for second load
    if(isLoad1 == false)
    {
      isLoad1 = true;
    }
    else
    {
      //if found triggers yellow led
      isLoad2 = true;
      timeInner1 = 10;
      digitalWrite(YELLOWLED, HIGH);
    }
  }
  else
  {
    //storing values for 3 minutes before being written over.
    isLoad = false;
    isLoadStorage[timeOuter] = isLoad;
  }
  if(timeOuter%10 == 0)
  {

  }
  
  //waiting a second before going through loop again.
  timeOuter++;
  delay(1000);
}