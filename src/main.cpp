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
const int REDLED = 11;
const int TAPSENSOR = 1;

int timeInner = 0;
int timeOuter = 0;
int darkness = 0;
int tapState;

int darknessStorage[180];

long int intialTempuDF = 0;
long int tempSensoruVolts;
long int tempSensoruDC;
long int tempSensoruDF;
long int tempAvg;
long int photoSensorVolt;
long int photoSensoruVolt;
long int touchSensorVolts;


long int tempDFStorage[18];
long int tempDFTempStorage[10];

bool isLoad;
bool isVibration;
bool isLoad1;
bool isLoad2;

bool isLoadStorage[180];
bool isVibrationStorage[180];

char loadDetected[50];
char vibrationDetected[50];
char tempOut[100];
char isDark[30];

void setup() 
{
  Serial.begin(9600);

  //setting up display for later
  display.begin(SSD1306_SWITCHCAPVCC,SCREEN_ADDRESS);
  display.clearDisplay();

  //setting up output pins
  pinMode(YELLOWLED, OUTPUT);
  pinMode(BLUELED, OUTPUT);
  pinMode(REDLED, OUTPUT);
  
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

  //resetting some variables
  darkness = 4;
  isLoad = false;
  isVibration = false;
  
  //shift data in storage arrays so there is an open slot.
  if(timeOuter == 180)
  {
    for(int i = 1; i < 180; i++)
    {
      darknessStorage[i-1] = darknessStorage[i];
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
  tapState = digitalRead(TAPSENSOR);

  //conversions
  tempSensoruVolts = tempSensoruVolts*(5000000/1024);
  photoSensoruVolt = photoSensorVolt*(5000000/1024);
  touchSensorVolts = touchSensorVolts*(5000000/1024);
  tempSensoruDC = (tempSensoruVolts-500000)*100;
  tempSensoruDF = tempSensoruDC*9/5+32000000;

  //checking if there is a load
  if(touchSensorVolts < 2.5*MICRO)
  {
    timeInner++;
    
    

    if(timeInner >= 6)
    {
      timeInner = 1;
      isLoad1 = false;
      isLoad2 = false;
    }
    
    // defining load as true.
    //storing values for 3 minutes before being written over.
    isLoad = true;
    sprintf(loadDetected, "A load has been detected near or on the bridge!");
    Serial.println(loadDetected);
    display.println(loadDetected);
    isLoadStorage[timeOuter] = isLoad;
    //checks for second load
    
    if(isLoad1 == false)
    {
      isLoad1 = true;

    }
    else
    {
      //if found triggers yellow led
      isLoad2 = true;
      timeInner = 10;
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
    tempAvg = 0;
    tempDFStorage[timeOuter] = tempSensoruDF;
    if(intialTempuDF == 0)
    {
      intialTempuDF = tempSensoruDF;
    }
    if((tempSensoruDF >= (intialTempuDF+5*MICRO)) || (tempSensoruDF) <= (intialTempuDF+5*MICRO))
    {
      digitalWrite(BLUELED, HIGH);
    }
    for(int i = 0; i < 10; i++)
    {
      tempAvg = tempAvg + tempDFStorage[i];
    }
    tempAvg = tempAvg/10;
    sprintf(tempOut, "The current tempature is %ld.%06ld F, The average temperature over the last ten seconds is %ld.%06ld F.",tempSensoruDF/MICRO, tempSensoruDF%MICRO, tempAvg/MICRO, tempAvg%MICRO);
    Serial.println(tempOut);
    display.println(tempOut);
  }
  else
  {
    tempDFStorage[timeOuter] = tempSensoruDF;
  }
  
  /*
    Darkness scale
    0 - day
    1 - semi-dark/dusk
    2 - dark
    4 - default state
  */
  
  //assigning values to darkness levels
  if(photoSensoruVolt > 2.2*MICRO)
  {
    darkness = 0;
  }
  else if( photoSensoruVolt < 1.0*MICRO)
  {
    darkness = 1;
  }
  else
  {
    darkness = 2;
  }
  darknessStorage[timeOuter] = darkness;
  
  //checking if it went from day to dusk.
  if(timeOuter > 0)
  {
    if(((darknessStorage[timeOuter]+darknessStorage[timeOuter-1]) == 1) || ((darknessStorage[timeOuter]-darknessStorage[timeOuter-1]) == 0))
    {      
      digitalWrite(REDLED, HIGH);
    }
  }
  
  //checking if completely dark and if printing message
  if(darknessStorage[timeOuter] == 2)
  {
    sprintf(isDark, "It is dark by the bridge!");
    Serial.println(isDark);
    display.println(isDark);
  }

  //checking for load using sensor number 2
  if(tapState == HIGH)
  {
    sprintf(vibrationDetected, "A load has been detected by sensor number 2.");
    Serial.println(vibrationDetected);
    display.println(vibrationDetected);
    isVibration = true;
  }
  else
  {
    isVibration = false;
  }
  isVibrationStorage[timeOuter] = isVibration;

  //waiting a second before going through loop again.
  timeOuter++;
  delay(1000);
}