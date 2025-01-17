#include "TM1637.h"
#include "mp3tf16p.h"
#include "Adafruit_NeoPixel.h"

enum clockStates
{
  standby,
  clock,
  clockAdjust,
  alarmAjust,
  timeTravel,
};

//Pins
int clk = 2;
int dio = 3;
int x0 = 11;
int x1 = 10;
int ledPin = 9;
int setupButtonPin = A0;
int hourButtonPin = A1;
int minuteButtonPin = A2;

//Global Variables
boolean passedFirstStart = false;

int analogHighLevel = 1000;

int hours = 0;
int minutes = 0;
long miliseconds = 0;
int milisecondDelay = 368;
int milisecondAdjust = 500 - milisecondDelay;

int clockHour = 0;
int clockMinute = 0;

boolean alarm = false;
int alarmHours = 0;
int alarmMinutes = 0;

int setupButton = 0;
int hourButton = 0;
int minuteButton = 0;

int ledCount = 24;

clockStates clockState = clockStates::standby;

TM1637 tm(clk, dio);
MP3Player mp3(x1, x0);
Adafruit_NeoPixel strip(ledCount, ledPin, NEO_GRB + NEO_KHZ800);

void setup() 
{
  Serial.begin(9600);

  startClock();
  mp3.initialize();
  strip.begin();
  strip.setBrightness(30);
}

void loop() 
{
  mp3.serialPrintStatus(MP3_ALL_MESSAGE);
  if (clockState != clockStates::standby)
  {
    flux();
    updateTime();
  }
  else
    delay(milisecondDelay);
  
  switch(clockState)
  {
    case clockStates::standby:
      standBy();
      break;
    case clockStates::clockAdjust:
      adjustClock();
      break;
    case clockStates::alarmAjust:
      adjustAlarm();
      break;
    case clockStates::clock:
      runClock();
      break;
  }
}

void standBy()
{
  setupButton = analogRead(setupButtonPin);

  if(setupButton >= analogHighLevel)
  {
    if(passedFirstStart)
    {
      setupButton = 0;
      clockState = clockStates::alarmAjust;
      mp3.playTrackNumber(1, 25, true);
      tm.start();
      initClockDisplay();
      Serial.println("Alarm Adjust");
      miliseconds = 1000;
    }
    else
      passedFirstStart = true;
  }
}

void adjustClock()
{
  setupButton = analogRead(setupButtonPin);
  hourButton = analogRead(hourButtonPin);
  minuteButton = analogRead(minuteButtonPin);
  /*Serial.print("SetupButton: " + String(setupButton));
  Serial.print(" HourButton: " + String(hourButton));
  Serial.print(" HourButton: " + String(minuteButton));
  Serial.println("");*/

  if (hourButton >= analogHighLevel && 
      minuteButton < analogHighLevel && 
      setupButton < analogHighLevel)
  {
    if (clockHour < 23)
      clockHour++;
    else
      clockHour = 0;
      
    setClock(getTime(clockHour, clockMinute));
    hourButton = LOW;
  }
  else if (hourButton < analogHighLevel && 
           minuteButton >= analogHighLevel && 
           setupButton < analogHighLevel)
  {
    if (clockMinute < 59)
      clockMinute++;
    else
      clockMinute = 0;
    setClock(getTime(clockHour, clockMinute));
    minuteButton = 0;
  }
  else if (hourButton < analogHighLevel && 
           minuteButton < analogHighLevel && 
           setupButton >= analogHighLevel)
  {
    clockState = clockStates::clock;
    mp3.playTrackNumber(2, 25, true);
    Serial.println("Clock");
    setupButton = 0;
    printSave();
    hours = clockHour;
    minutes = clockMinute;
    miliseconds = 500;
    setClock(getTime(hours, minutes));
  }
}

void adjustAlarm()
{
  setupButton = analogRead(setupButtonPin);
  hourButton = analogRead(hourButtonPin);
  minuteButton = analogRead(minuteButtonPin);
  /*Serial.print("SetupButton: " + String(setupButton));
  Serial.print(" HourButton: " + String(hourButton));
  Serial.print(" HourButton: " + String(minuteButton));
  Serial.println("");*/

  if (hourButton >= analogHighLevel && 
      minuteButton < analogHighLevel && 
      setupButton < analogHighLevel)
  {
    if (alarmHours < 23)
      alarmHours++;
    else
      alarmHours = 0;
      
    setClock(getTime(alarmHours, alarmMinutes));
    hourButton = LOW;
  }
  else if (hourButton < analogHighLevel && 
           minuteButton >= analogHighLevel && 
           setupButton < analogHighLevel)
  {
    if (alarmMinutes < 59)
      alarmMinutes++;
    else
      alarmMinutes = 0;
    setClock(getTime(alarmHours, alarmMinutes));
    minuteButton = LOW;
  }
  else if (hourButton >= analogHighLevel && 
           minuteButton >= analogHighLevel && 
           setupButton < analogHighLevel)
  {
    alarm = !alarm;
    Serial.println("Set alarm to " + String(alarmHours) + ":" + String(alarmMinutes) + " | status: " + String(alarm));
    hourButton = 0;
    minuteButton = 0;
    mp3.playTrackNumber(2, 25, true);
    miliseconds += 500;
    printSave();
  }
  else if (hourButton < analogHighLevel && 
           minuteButton < analogHighLevel && 
           setupButton >= analogHighLevel)
  {
    clockState = clockStates::clockAdjust;
    mp3.playTrackNumber(1, 25, true);
    miliseconds += 1000;
    Serial.println("Adjust clock");
    setupButton = 0;  
    clockHour = hours;
    clockMinute = minutes;
    setClock(getTime(clockHour, clockMinute));

  }
}

void runClock()
{
  setupButton = analogRead(setupButtonPin);
  //Serial.println("SetupButton: " + String(setupButton));

  if(setupButton >= analogHighLevel)
  {
    clockState = clockStates::alarmAjust;
    mp3.playTrackNumber(1, 25, true);
    miliseconds += 1000;
    Serial.println("Alarm adjust");
    setClock(getTime(alarmHours, alarmMinutes));
  }
}

void startClock()
{
  tm.init();
  tm.set(7);
  tm.display(0, 5);
  tm.display(1, 7);
  tm.display(2, 10);
  tm.display(3, 7);
  delay(milisecondDelay*2);
  tm.clearDisplay();
  tm.stop();
  setupButton = 0;
}

void initClockDisplay()
{
  tm.set(7);
  setClock(getTime(hours, minutes));
}

void setClock(String time)
{
  for(int i = 0; i < 4; i++)
  {
    tm.display(i, time[i]);
  }
}

String getTime(int hours, int minutes)
{
  String strTime = "";
  if (hours == 0)
    strTime += "00";
  else
  {
    if (hours < 10)
      strTime += "0" + String(hours);
    else
      strTime += String(hours);
  }

  if (minutes == 0)
    strTime += "00";
   else
  {
    if (minutes < 10)
      strTime += "0" + String(minutes);
    else
      strTime += String(minutes);
  }
  return strTime;
}

void updateTime()
{
  //Serial.println("Miliseconds: " + String(miliseconds));
  delay(milisecondDelay);

  miliseconds += milisecondDelay + milisecondAdjust;

  if (hours == alarmHours && minutes == alarmMinutes && alarm)
    runAlarm();

  if (miliseconds >= 60000)
  {
    miliseconds = 0;
    minutes++;
    setClock(getTime(hours, minutes));
  }

  if (minutes == 60)
  {
    minutes = 0;
    hours++;
  }

  if (hours == 24)
    hours = 0;
}

void printSave()
{
  tm.clearDisplay();
  tm.display(1,5);
  tm.display(2,14);
  tm.display(3,7);
  tm.stop();
}

void flux()
{
  strip.clear();

  for(int i = 0; i < (ledCount/3); i++)
  {
    strip.setPixelColor(i, 255, 255, 255);
    strip.setPixelColor((8+i), 255, 255, 255);
    strip.setPixelColor((16+i), 255, 255, 255);
    strip.show();
    delay(8);
    strip.setPixelColor(i, 0, 0, 0);
    strip.setPixelColor((8+i), 0, 0, 0);
    strip.setPixelColor((16+i), 0, 0, 0);
    strip.show();
    delay(7);
  }
}

void runAlarm()
{
  strip.clear();

  for(int i = 0; i < (ledCount/3); i++)
  {
    strip.setPixelColor(i, 255, 255, 255);
    strip.setPixelColor((8+i), 255, 255, 255);
    strip.setPixelColor((16+i), 255, 255, 255);
  }

  strip.show();

  mp3.playTrackNumber(3, 20, true);

  strip.clear();

  /*for(int i = 0; i < (ledCount/3); i++)
  {
    strip.setPixelColor(i, 255, 255, 255);
    strip.setPixelColor((8+i), 255, 255, 255);
    strip.setPixelColor((16+i), 255, 255, 255);
  }

  strip.show();*/

  miliseconds += 22500;
  minutes += 1;

  setClock(getTime(hours, minutes));
}


