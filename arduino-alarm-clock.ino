/***************************************************************************************
    Name    : Alarm / Temp / Umid / Clock / Countdown
    Author  : kernelpanik
    Created : March 03, 2023
    Last Modified: March 26, 2023
    Version : 1.0
    Notes   : This code is for use with an Arduino Uno and LCD/button shield. The
              intent is for anyone to use this program to give them a starting
              program with a fully functional menu with minimal modifications
              required by the user.
 ***************************************************************************************/

#include <LiquidCrystal.h>
#include <RTClib.h>
#include <DHT.h>
#include <Tone.h>
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"


///////////////////////////////////////////////////////////////
// Menu
String menuItems[] = {"Alarm", "Countdown", "Ringtone", "Ringtone Vol", "Set Date/Time", ""};
String alarmMenuItems[] = {"Set Alarm", "Clear Alarm", ""};
String songMenuItems[] = {"Bru1", "Bru2", ""};
int songNumber = sizeof(songMenuItems) / sizeof(songMenuItems[0]);

///////////////////////////////////////////////////////////////
// LCD shields
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

///////////////////////////////////////////////////////////////
// RTC
RTC_DS3231 rtc;
DateTime now;

///////////////////////////////////////////////////////////////
// DHT22
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);


///////////////////////////////////////////////////////////////
// AM312 PIR
#define pirPin 11             // the pin that OUTPUT pin of sensor is connected to
#define backLightPin 10         // Pin for backLight
int pinStateCurrent   = LOW; // current state of pin
int pinStatePrevious  = LOW; // previous state of pin
const unsigned long DELAY_TIME_MS = 30000; // 30000 miliseconds ~ 30 seconds
bool delayEnabled = false;
unsigned long delayStartTime;         


///////////////////////////////////////////////////////////////
// DFPlayer
SoftwareSerial mySoftwareSerial(13, 12); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
void setAlarmSong();
void setAlarmVol();
int songmenuPage = 0;
int songmaxMenuPages = 0;
int songcursorPosition = 0;
int songmaxCursorPosition = 0;
int songN = 1;
int volume = 20;
uint8_t state = 0;


///////////////////////////////////////////////////////////////
// Navigation button variables
int readKey;

///////////////////////////////////////////////////////////////
// Menu control variables
void MenuDraw(String menuItems[], int menuPage, int maxMenuPages);
void drawCursor( int menuPage, int cursorPosition);
int menuPage = 0;
int maxMenuPages = 0;
int cursorPosition = 0;
int maxCursorPosition = 0;
int alarmmenuPage = 0;
int alarmmaxMenuPages = 0;
int alarmcursorPosition = 0;
int alarmmaxCursorPosition = 0;
int activeButton = 0;
int kgoing = 0; // exit to main loop
int button;

bool startone = false;  // start counting day and month from one
bool UseAlarmMenu = false;

///////////////////////////////////////////////////////////////
// countdown
void CountdownTimer();
int getCountN(char countNText[], int startNum, int maxNum);
void countTheTime(int secToCount, int minToCount, char countLabel[]);
void countCancelled(char countMsgNO[]);
void countCompleted(char countMsgOK[]);
int secToCount = 0;
int minToCount = 0;
char countLabel;
int minutes = 0;
int seconds = 0;


///////////////////////////////////////////////////////////////
// Alarm
void setAlarm();
void clearAlarm();
void stopAlarm();
bool alarmSet = false;
bool alarmStop = true;
int alarmHours = 2;
int alarmMinutes = 3;


///////////////////////////////////////////////////////////////
// set Date/Time
void setDateTime();
int setYear = 0;
int setMonth = 0;
int setDay = 0;
int setHour = 0;
int setMinute = 0;
int goback = 0;  // to exit
int lastDay = 0;
int lastMonth = 0;
int lastYear = 0;
int lastHour = 0;
int lastMinute = 0;

///////////////////////////////////////////////////////////////
// clock
int one = 0;
int year = 0;
int tens = 0;

///////////////////////////////////////////////////////////////
// temp / umid
float t;
float h;



///////////////////////////////////////////////////////////////
// show main screen without using delay
unsigned long previousMillis = 0;  // will store last time LED was updated
unsigned long currentMillis;
const long interval = 1000;  // interval at which to blink (milliseconds)

///////////////////////////////////////////////////////////////
// Creates custom characters 
byte downArrow[8] = { 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b10101, 0b01110, 0b00100 };
byte  upArrow[8] = { 0b00100, 0b01110, 0b10101, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 };
byte menuCursor[8] = { 0b01000, 0b00100, 0b00010, 0b00001, 0b00010, 0b00100, 0b01000, 0b00000 };
byte thermometer[8] = { 0b00100, 0b01010, 0b01010, 0b01110, 0b01110, 0b11111, 0b11111, 0b01110 };
byte drop[8] = { 0b00100, 0b00100, 0b01010, 0b01010, 0b10001, 0b10001, 0b10001, 0b01110 }; 
byte bell[8] = {0b00100, 0b01110, 0b01110, 0b01110, 0b11111, 0b00000, 0b00100};



///////////////////////////////////////////////////////////////
// buzzer
#define beeper A3
#define shortBeep 100
#define longBeep  500
void timedBeep(int beepTime, int beepCount);
int beepTime = 0;
int beepCount = 0;







//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {

  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  lcd.begin(16, 2); //LCD
  lcd.clear();
  dht.begin(); // DHT22
  rtc.begin(); // RTC
  pinMode(pirPin, INPUT); // Pin motion detection
  pinMode(backLightPin, OUTPUT); // Pin backlight
  digitalWrite(backLightPin, HIGH);
  pinMode(beeper, OUTPUT); //buzzer
  digitalWrite(beeper, LOW);


    if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }

  myDFPlayer.volume(20);  //Set volume value. From 0 to 30

  // Creates the byte for the 3 custom characters
  lcd.createChar(0, menuCursor);
  lcd.createChar(1, upArrow);
  lcd.createChar(2, downArrow);
  lcd.createChar(3, thermometer);
  lcd.createChar(4, drop);
  lcd.createChar(5, bell);

  // The first line finds the index of the "end of array" marker (see (1.) above.)
  // The second line sets maxCursorPosition to the last actual element of the menuItems array.
  //Lines 3 & 4 set the number of menu pages. This is generally 1 less than the number of cursor positions 
    //(because we see 2 menu items at the same time), unless the number of cursor positions is 1 or less. (Note that we start counting at 0.)
  while (menuItems[maxCursorPosition] != "") {
  maxCursorPosition++;
  }
  maxCursorPosition--;
  maxMenuPages = maxCursorPosition;
  if (maxMenuPages >= 1) maxMenuPages -= 1;

  while (alarmMenuItems[alarmmaxCursorPosition] != "") {
  alarmmaxCursorPosition++;
  }
  alarmmaxCursorPosition--;
  alarmmaxMenuPages = alarmmaxCursorPosition;
  if (alarmmaxMenuPages >= 1) alarmmaxMenuPages -= 1;

  while (songMenuItems[songmaxCursorPosition] != "") {
  songmaxCursorPosition++;
  }
  songmaxCursorPosition--;
  songmaxMenuPages = songmaxCursorPosition;
  if (songmaxMenuPages >= 1) songmaxMenuPages -= 1;


}


void loop() {
  now = rtc.now();
  pinStatePrevious = pinStateCurrent; // store state
  pinStateCurrent = digitalRead(pirPin);   // read new state
  if (pinStatePrevious == LOW && pinStateCurrent == HIGH) {   // pin state change: LOW -> HIGH
    digitalWrite(backLightPin, HIGH);    
  }   else
  if (pinStatePrevious == HIGH && pinStateCurrent == LOW) {   // pin state change: HIGH -> LOW
    delayEnabled = true; // enable delay
    delayStartTime = millis(); // set start time
  }

  if (delayEnabled == true && (millis() - delayStartTime) >= DELAY_TIME_MS) {
    delayEnabled = false; // disable delay
   digitalWrite(backLightPin, LOW); 
  }


  if (kgoing == 0) {

    button_loop();
    currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      ClockDisplay();
      TempDisplay();
      checkAlarm();
    }

  } else if (kgoing = 1) {
    digitalWrite(backLightPin, HIGH);
    MenuDraw(menuItems, menuPage, maxMenuPages);
    drawCursor(menuPage, cursorPosition);
    operateMainMenu();
    checkAlarm();
  }

}



//This function is called whenever a button press is evaluated. The LCD shield works by observing a voltage drop across the buttons all hooked up to A0.
int evaluateButton(int x) {
  int result = 0;
  if (x < 50) {
    result = 1; // right
  } else if (x < 195) {
    result = 2; // up
  } else if (x < 380) {
    result = 3; // down
  } else if (x < 555) {
    result = 4; // left
  } else if (x < 790) {
    result = 5; // select
  }
  return result;
}



void button_loop() {

  button;
  readKey = analogRead(0);
  if (readKey < 790) {
    delay(100);
    readKey = analogRead(0);
  }
  button = evaluateButton(readKey);
  switch (button) {
  case 0: // When button returns as 0 there is no action taken
    break;
  case 1: // right
    break;
  case 2: // up
    break;
  case 3: // down
    break;
  case 4: // left
    break;
  case 5: //select
    kgoing = 1;
  }
}



// This function will generate the 2 menu items that can fit on the screen. They will change as you scroll through your menu. Up and down arrows will indicate your current menu position.
void MenuDraw(String menuItems[], int menuPage, int maxMenuPages) {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(menuItems[menuPage]);
  lcd.setCursor(1, 1);
  lcd.print(menuItems[menuPage + 1]);
  if (menuPage == 0) {
    lcd.setCursor(15, 1);
    lcd.write(byte(2));
  } else if (menuPage > 0 && menuPage < maxMenuPages) {
    lcd.setCursor(15, 1);
    lcd.write(byte(2));
    lcd.setCursor(15, 0);
    lcd.write(byte(1));
  } else if (menuPage == maxMenuPages) {
    lcd.setCursor(15, 0);
    lcd.write(byte(1));
  }
}







// // When called, this function will erase the current cursor and redraw it based on the cursorPosition and menuPage variables.
void drawCursor( int menuPage, int cursorPosition) {

  for (int x = 0; x < 2; x++) { // Erases current cursor
    lcd.setCursor(0, x);
    lcd.print(" ");
  }

  // The menu is set up to be progressive (menuPage 0 = Item 1 & Item 2, menuPage 1 = Item 2 & Item 3, menuPage 2 = Item 3 & Item 4), so
  // in order to determine where the cursor should be you need to see if you are at an odd or even menu page and an odd or even cursor position.
  if (menuPage % 2 == 0) {
    if (cursorPosition % 2 == 0) { // If the menu page is even and the cursor position is even that means the cursor should be on line 1
      lcd.setCursor(0, 0);
      lcd.write(byte(0));
    }
    if (cursorPosition % 2 != 0) { // If the menu page is even and the cursor position is odd that means the cursor should be on line 2
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
    }
  }
  if (menuPage % 2 != 0) {
    if (cursorPosition % 2 == 0) { // If the menu page is odd and the cursor position is even that means the cursor should be on line 2
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
    }
    if (cursorPosition % 2 != 0) { // If the menu page is odd and the cursor position is odd that means the cursor should be on line 1
      lcd.setCursor(0, 0);
      lcd.write(byte(0));
    }
  }
}




void operateMainMenu() {
  activeButton = 0;
  while (activeButton == 0) {
    button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
    case 0: // When button returns as 0 there is no action taken
      break;
    case 1: // This case will execute if the "forward" button is pressed
      button = 0;
      switch (cursorPosition) { // The case that is selected here is dependent on which menu page you are on and where the cursor is.
      case 0:
        menuItem1();
        break;
      case 1:
        menuItem2();
        break;
      case 2:
        menuItem3();
        break;
      case 3:
        menuItem4();
        break;
      case 4:
        menuItem5();
        break;  
      }
      activeButton = 1;
      MenuDraw(menuItems, menuPage, maxMenuPages);
      drawCursor(menuPage, cursorPosition);
      break;
    case 2:
      button = 0;
      if (menuPage == 0) {
        cursorPosition = cursorPosition - 1;
        cursorPosition = constrain(cursorPosition, 0, maxCursorPosition);
      }
      if (menuPage % 2 == 0 and cursorPosition % 2 == 0) {
        menuPage = menuPage - 1;
        menuPage = constrain(menuPage, 0, maxMenuPages);
      }

      if (menuPage % 2 != 0 and cursorPosition % 2 != 0) {
        menuPage = menuPage - 1;
        menuPage = constrain(menuPage, 0, maxMenuPages);
      }

      cursorPosition = cursorPosition - 1;
      cursorPosition = constrain(cursorPosition, 0, maxCursorPosition);

      MenuDraw(menuItems, menuPage, maxMenuPages);
      drawCursor(menuPage, cursorPosition);
      activeButton = 1;
      break;
    case 3:
      button = 0;
      if (menuPage % 2 == 0 and cursorPosition % 2 != 0) {
        menuPage = menuPage + 1;
        menuPage = constrain(menuPage, 0, maxMenuPages);
      }

      if (menuPage % 2 != 0 and cursorPosition % 2 == 0) {
        menuPage = menuPage + 1;
        menuPage = constrain(menuPage, 0, maxMenuPages);
      }

      cursorPosition = cursorPosition + 1;
      cursorPosition = constrain(cursorPosition, 0, maxCursorPosition);
      MenuDraw(menuItems, menuPage, maxMenuPages);
      drawCursor(menuPage, cursorPosition);
      activeButton = 1;
      break;
    case 4:
      button = 0;
      activeButton = 1;
      kgoing = 0;
      myDFPlayer.pause();
      lcd.clear();
      break;
    }
  }
}



void operateAlarmMenu() {
  activeButton = 0;
  while (activeButton == 0) {
    button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 0: // When button returns as 0 there is no action taken
        break;
      case 1:  // This case will execute if the "forward" button is pressed
        button = 0;
        switch (alarmcursorPosition) { // The case that is selected here is dependent on which menu page you are on and where the cursor is.
          case 0:
            setAlarm();
            break;
          case 1:
            clearAlarm();
            break;
        }
        MenuDraw(alarmMenuItems, alarmmenuPage, alarmmaxMenuPages);
        drawCursor(alarmmenuPage, alarmcursorPosition);
        break;
      case 2:
        button = 0;
        if (alarmmenuPage == 0) {
          alarmcursorPosition = alarmcursorPosition - 1;
          alarmcursorPosition = constrain(alarmcursorPosition, 0, alarmmaxCursorPosition);
        }
        if (alarmmenuPage % 2 == 0 and alarmcursorPosition % 2 == 0) {
          alarmmenuPage = alarmmenuPage - 1;
          alarmmenuPage = constrain(alarmmenuPage, 0, alarmmaxMenuPages);
        }

        if (alarmmenuPage % 2 != 0 and alarmcursorPosition % 2 != 0) {
          alarmmenuPage = alarmmenuPage - 1;
          alarmmenuPage = constrain(alarmmenuPage, 0, alarmmaxMenuPages);
        }

        alarmcursorPosition = alarmcursorPosition - 1;
        alarmcursorPosition = constrain(alarmcursorPosition, 0, alarmmaxCursorPosition);

        MenuDraw(alarmMenuItems, alarmmenuPage, alarmmaxMenuPages);
        drawCursor(alarmmenuPage, alarmcursorPosition);
        break;
      case 3:
        button = 0;
        if (alarmmenuPage % 2 == 0 and alarmcursorPosition % 2 != 0) {
          alarmmenuPage = alarmmenuPage + 1;
          alarmmenuPage = constrain(alarmmenuPage, 0, alarmmaxMenuPages);
        }

        if (alarmmenuPage % 2 != 0 and alarmcursorPosition % 2 == 0) {
          alarmmenuPage = alarmmenuPage + 1;
          alarmmenuPage = constrain(alarmmenuPage, 0, alarmmaxMenuPages);
        }

        alarmcursorPosition = alarmcursorPosition + 1;
        alarmcursorPosition = constrain(alarmcursorPosition, 0, alarmmaxCursorPosition);
        MenuDraw(alarmMenuItems, alarmmenuPage, alarmmaxMenuPages);
        drawCursor(alarmmenuPage, alarmcursorPosition);
        break;
        case 4:  
        button = 0;
        activeButton = 1;
        countCancelled("Operation");
        delay(1000);
        break;
     }
   }
}




void operateSongMenu() {
  activeButton = 0;
  songN = 1;
  while (activeButton == 0) {
    button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 0: // When button returns as 0 there is no action taken
        break;
      case 1:  // This case will execute if the "forward" button is pressed
        button = 0;
        setAlarmSong();
        MenuDraw(songMenuItems, songmenuPage, songmaxMenuPages);
        drawCursor(songmenuPage, songcursorPosition);
        break;
      case 2:
        button = 0;
        if (songmenuPage == 0) {
          songcursorPosition = songcursorPosition - 1;
          songcursorPosition = constrain(songcursorPosition, 0, songmaxCursorPosition);
        }
        if (songmenuPage % 2 == 0 and songcursorPosition % 2 == 0) {
          songmenuPage = songmenuPage - 1;
          songmenuPage = constrain(songmenuPage, 0, songmaxMenuPages);
        }

        if (songmenuPage % 2 != 0 and songcursorPosition % 2 != 0) {
          songmenuPage = songmenuPage - 1;
          songmenuPage = constrain(songmenuPage, 0, songmaxMenuPages);
        }

        songcursorPosition = songcursorPosition - 1;
        songcursorPosition = constrain(songcursorPosition, 0, songmaxCursorPosition);
        songN--;
        if (songN == 0){
          songN = 1;
        }
        Serial.print(songN);
        MenuDraw(songMenuItems, songmenuPage, songmaxMenuPages);
        drawCursor(songmenuPage, songcursorPosition);
        break;
      case 3:
        button = 0;
        if (songmenuPage % 2 == 0 and songcursorPosition % 2 != 0) {
          songmenuPage = songmenuPage + 1;
          songmenuPage = constrain(songmenuPage, 0, songmaxMenuPages);
        }

        if (songmenuPage % 2 != 0 and songcursorPosition % 2 == 0) {
          songmenuPage = songmenuPage + 1;
          songmenuPage = constrain(songmenuPage, 0, songmaxMenuPages);
        }
        songN++;
        if (songN > (songNumber -1 )) {
          songN = (songNumber -1 );
        }
        Serial.print(songN);
        songcursorPosition = songcursorPosition + 1;
        songcursorPosition = constrain(songcursorPosition, 0, songmaxCursorPosition);
        MenuDraw(songMenuItems, songmenuPage, songmaxMenuPages);
        drawCursor(songmenuPage, songcursorPosition);
        break;
        case 4:  
        button = 0;
        activeButton = 1;
        countCancelled("Operation");
        delay(1000);
        break;
        case 5:
        myDFPlayer.play(songN);  
        button = 0;
        activeButton = 1;
        break;
     }
   }
}







void menuItem1() { // Function executes when you select the 2nd item from main menu
  activeButton = 0;

  while (activeButton == 0) {
    button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
    case 0:
      MenuDraw(alarmMenuItems, alarmmenuPage, alarmmaxMenuPages);
      drawCursor(alarmmenuPage, alarmcursorPosition);
      operateAlarmMenu(); 
      break;
    case 4: // This case will execute if the "back" button is pressed
      button = 0;
      activeButton = 1;
      countCancelled("Operation");
      delay(1000);
      break;
    }
  }

}


void menuItem2() { // Function executes when you select the 2nd item from main menu
  activeButton = 0;

  CountdownTimer();
  if (goback == 1) {
    return;
  }
  while (activeButton == 0) {
    button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
    case 4: // This case will execute if the "back" button is pressed
      button = 0;
      activeButton = 1;
      countCancelled("Operation");
      delay(1000);
      break;
    }
  }
}


void menuItem3() { // Function executes when you select the 3rd item from main menu
  activeButton = 0;

  while (activeButton == 0) {
    button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
    case 0:
      MenuDraw(songMenuItems, songmenuPage, songmaxMenuPages);
      drawCursor(songmenuPage, songcursorPosition);
      operateSongMenu();
      break;
    case 4: // This case will execute if the "back" button is pressed
      button = 0;
      activeButton = 1;
      countCancelled("Operation");
      delay(1000);
      break;
    }
  }

}


void menuItem4() { // Function executes when you select the 4th item from main menu
  activeButton = 0;

  setAlarmVol();
  if (goback == 1) {
    return;
  }
  while (activeButton == 0) {

    button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
    case 4: // This case will execute if the "back" button is pressed
      button = 0;
      activeButton = 1;
      break;
    }
  }
}



void menuItem5() { // Function executes when you select the 4th item from main menu
  activeButton = 0;

  setDateTime();
  if (goback == 1) {
    return;
  }
  while (activeButton == 0) {

    button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
    case 4: // This case will execute if the "back" button is pressed
      button = 0;
      activeButton = 1;
      break;
    }
  }
}




///////////////////////
///////////////////////
// start Countdown


void CountdownTimer() {
  button = 0;

  int CountSeconds = 0;
  int CountMinutes = 0;

  CountSeconds = getCountN("Set seconds", CountSeconds, 59);
  if (goback == 1) {
    countCancelled("Operation");
    delay(1000);
    return;
  }
  if (CountSeconds == 0) {
    CountMinutes = getCountN("Set minutes", CountMinutes, 59);

    if (goback == 1) {
      countCancelled("Operation");
      delay(1000);
      return;
    }

    if (CountMinutes > 0) {
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("Countdown");
      lcd.setCursor(5, 1);
      if (CountMinutes < 10) {
        lcd.print("0");
      }
      lcd.print(CountMinutes);
      lcd.print(":");
      lcd.print("00");
      delay(1000);
      lcd.clear();
      countTheTime(CountMinutes * 60, 0, "Countdown");
    } else {
      countCancelled("Countdown");
      delay(1000);
    }
  }

  if (CountSeconds > 0) {
    CountMinutes = getCountN("Set minutes", CountMinutes, 59);
    if (goback == 1) {
      countCancelled("Operation");
      delay(1000);
      return;
    }
    if (CountMinutes == 0) {
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("Countdown");
      lcd.setCursor(5, 1);
      lcd.print("0");
      lcd.print(CountMinutes);
      lcd.print(":");
      if (CountSeconds < 10)
        lcd.print("0");
      lcd.print(CountSeconds);
      delay(1000);
      lcd.clear();
      countTheTime(CountSeconds, 0, "Countdown");
      return;
    }

    if (CountMinutes > 0) {
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("Countdown");
      lcd.setCursor(5, 1);
      if (CountMinutes < 10) {
        lcd.print("0");
      }
      lcd.print(CountMinutes);
      lcd.print(":");
      if (CountSeconds < 10)
        lcd.print("0");
      lcd.print(CountSeconds);
      delay(1000);
      lcd.clear();
      countTheTime(CountSeconds, CountMinutes, "Countdown");
      return;
    }
  }
}






int getCountN(char countNText[], int startNum, int maxNum)
{
  activeButton = 0;
  int countN = startNum;
  button = 0;
  goback = 0;
  lcd.clear();
  lcd.print(countNText);
  lcd.setCursor(0,1);
        if (countN == lastYear ) {   // inserting years starting from 2000
          lcd.print("2009");
          startone = false;
         } else if (countN == lastMonth ) {   // inserting months and days starting from 1
          startone = true;
            if (lastMonth < 10 ) {
             lcd.print("0");
             lcd.print(lastMonth);
           } else if (lastMonth > 10 ) {
             lcd.print(lastMonth);
           }
         } else if (countN == lastDay ) {   // inserting months and days starting from 1
          startone = true;
            if (lastDay < 10 ) {
             lcd.print("0");
             lcd.print(lastDay);
           } else if (lastDay > 10 ) {
             lcd.print(lastDay);
           }
         } else if (countN == lastHour ) {   // inserting months and days starting from 1
           if (lastHour == 0) {
             lcd.print("00");
           } else if (lastHour < 10 ) {
             lcd.print("0");
             lcd.print(lastHour);
           } else if (lastHour > 10 ) {
             lcd.print(lastHour);
           }
         } else if (countN == lastMinute ) {   // inserting months and days starting from 1
           if (lastMinute == 0) {
             lcd.print("00");
           } else if (lastMinute < 10 ) {
             lcd.print("0");
             lcd.print(lastMinute);
           } else if (lastMinute > 10 ) {
             lcd.print(lastMinute);
           }
         }
         else {
          lcd.print("00");
         }
    while (activeButton == 0) {
    button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
        case 0:  
        break;
        case 1:  // This case will execute if the "right" button is pressed
        button = 0;
        activeButton = 1;
        return countN;
        break;
        case 2:  // This case will execute if the "up" button is pressed
        button = 0;
        countN++;
         if (countN >= maxNum) { 
          countN = maxNum;
         }  
         if (countN < 10) { 
          lcd.setCursor(0,1);
          lcd.print("0");
          lcd.print(countN);
         }  else {
            lcd.setCursor(0,1); 
            lcd.print(countN);
         }
        break;
        case 3:  // This case will execute if the "down" button is pressed
        button = 0;
        countN--;
        if (countN <= 0) { 
          countN = 0;
         }
         if (countN < 10) { 
          lcd.setCursor(0,1);
          lcd.print("0");
          lcd.print(countN);
         } else {
        lcd.setCursor(0,1);
        lcd.print(countN);
         }             

        break;
        case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        countN = 0;
        activeButton = 1;
        countCancelled("Operation");
        goback = 1;
        return;
    }
  }
  return;
}




void countTheTime(int secToCount, int minToCount, char countLabel[]) {
  
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print(countLabel);
  activeButton = 0;


  minToCount = minToCount * 60;
  secToCount = secToCount + minToCount;

  while (secToCount > 0 && minToCount >= 0 && activeButton == 0) {
    button;
    readKey = analogRead(0);
    button = evaluateButton(readKey);
    switch (button) {
      case 4: // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        goback = 1;
        countCancelled("Countdown");
        delay(1000);
        return;
    }

    currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      secToCount--;

      minutes = secToCount / 60;
      seconds = secToCount % 60;

 
    if (minutes >= 0 && minutes < 10 ) {
      lcd.setCursor(5, 1);
      lcd.print("0");
      lcd.print(minutes);
      lcd.print(":00");
      } else {
      lcd.setCursor(5, 1);
      lcd.print(minutes);
      lcd.print(":00");
    }


      if (seconds < 10) {
        lcd.setCursor(8, 1); // ok 8 per sotto il minuto, e per sotto i 10 sec e sopra 10 min
        lcd.print("0");
        lcd.print(seconds);
      } else {
        lcd.setCursor(8, 1); // ok 8 per sotto il minuto, e per sotto i 10 sec  e sopra 10 min
        lcd.print(seconds);
      }

    }
  }
  countCompleted("Countdown");
}


void countCancelled(char countMsgNO[]) {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print(countMsgNO);
  lcd.setCursor(4, 1);
  lcd.print("Deleted");
}

void countCompleted(char countMsgOK[]) {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print(countMsgOK);
  lcd.setCursor(2, 1);
  lcd.print("Finished!!!");
  timedBeep(shortBeep,5);
}



///////////////////////
///////////////////////
// end Countdown




///////////////////////
///////////////////////
// start date/time


void setDateTime() {
  int done = 0;
  activeButton = 0;
  while (activeButton == 0) {
    button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    button = 0;
    goback = 0;
    switch (button) {

    case 0: // When button returns as 0 there is no action taken
      setHour = getCountN("Set Hours", lastHour, 23);
      if (goback == 1) {
        activeButton = 1;
        countCancelled("Operation");
        delay(1000);
        return;
      }
      setMinute = getCountN("Set Minutes", lastMinute, 59);
      if (goback == 1) {
        activeButton = 1;
        countCancelled("Operation");
        delay(1000);
        return;
      }
      setDay = getCountN("Set Day", lastDay, 31);
      if (goback == 1) {
        activeButton = 1;
        countCancelled("Operation");
        delay(1000);
        return;
      }
      setMonth = getCountN("Set Month", lastMonth, 12);
      if (goback == 1) {
        activeButton = 1;
        countCancelled("Operation");
        delay(1000);
        return;
      }
      setYear = getCountN("Set Year", lastYear, 3000);
      if (goback == 1) {
        activeButton = 1;
        countCancelled("Operation");
        delay(1000);
        return;
      }

      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("Date/Time");
      lcd.setCursor(3, 1);
      lcd.print("Saving...");
      rtc.adjust(DateTime(setYear, setMonth, setDay, setHour, setMinute));
      delay(2000);
      activeButton = 1;
      goback = 1;
      return;
    case 1: // This case will execute if the "forward" button is pressed
      button = 0;
      activeButton = 1;
    case 2:
    case 3:
      break;
    case 4: // This case will execute if the "back" button is pressed
      button = 0;
      activeButton = 1;
      countCancelled("Operation");
      break;
    }
  }
}



void ClockDisplay() {
  bool clockPM = 0;
  lastDay = now.day();
  lastMonth = now.month();
  lastYear = now.year();
  lastHour = now.hour();
  lastMinute = now.minute();

  lcd.setCursor(0, 0);
  if (now.day() < 10) {
    lcd.print("0");
    lcd.print(now.day(), DEC);
    lcd.print("/");
  } else {
    lcd.print(now.day(), DEC);
    lcd.print("/");
  }

  if (now.month() < 10) {
    lcd.print("0");
    lcd.print(now.month(), DEC);
    lcd.print("/");
  } else {
    lcd.print(now.month(), DEC);
    lcd.print("/");
  }
  year = now.year();
  one = year % 10;
  year /= 10;
  tens = year % 10;
  tens *= 10;
  year = tens + one;
  if (year < 10) {
    lcd.print("0");
    lcd.print(year);
  } else {
    lcd.print(year);
  }
  lcd.print("  ");
  lcd.setCursor(0, 1); //era 4,1

  if (now.hour() < 10) {
    lcd.print("0");
    lcd.print(now.hour(), DEC);

  } else {
    lcd.print(now.hour(), DEC);
  }

  lcd.print(":");

  if (now.minute() < 10) {
    lcd.print("0");
    lcd.print(now.minute(), DEC);
  } else {
    lcd.print(now.minute(), DEC);
  }

  if (alarmSet) {
    lcd.setCursor(6, 1);
    lcd.print(char(5));
  }
}


///////////////////////
///////////////////////
// end date/time



///////////////////////
///////////////////////
// start TempDisplay


void TempDisplay() {
  h = dht.readHumidity();
  t = dht.readTemperature();
  lcd.setCursor(10, 0);
  lcd.print(t);
  lcd.write(byte(3));
  lcd.setCursor(10, 1);
  lcd.print(h);
  lcd.write(byte(4));
}


///////////////////////
///////////////////////
// end TempDisplay


///////////////////////
///////////////////////
// start setAlarm/clearAlarm


void setAlarm() {
  alarmStop = false;
  int done = 0;
  activeButton = 0;
  while (activeButton == 0) {
    button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    button = 0;
    goback = 0;
    switch (button) {
    case 0: // When button returns as 0 there is no action taken
      alarmHours = getCountN("Set Alarm Hours", lastHour, 23);
      if (goback == 1) {
        activeButton = 1;
        countCancelled("Operation");
        delay(1000);
        return;
      }
      alarmMinutes = getCountN("Set Alarm Minutes", lastMinute, 59);
      if (goback == 1) {
        activeButton = 1;
        countCancelled("Operation");
        delay(1000);
        return;
      }

      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print("Alarm");
      lcd.setCursor(3, 1);
      lcd.print("Saving...");
      alarmSet = true;
      delay(2000);
      activeButton = 1;
      goback = 1;
      return;
    case 1: // This case will execute if the "forward" button is pressed
      button = 0;
      activeButton = 1;
    case 2:
    case 3:
      break;
    case 4: // This case will execute if the "back" button is pressed
      button = 0;
      activeButton = 1;
      countCancelled("Operation");
      break;
    }
  }
}



void clearAlarm() {
  lcd.clear();

  if ( alarmSet == false ) {
  lcd.setCursor(5, 0);
  lcd.print("Alarm");
  lcd.setCursor(3, 1);
  lcd.print("Not Found ");
  goback = 1;
  activeButton = 1;
  delay(2000);
  lcd.clear();
  } else if (alarmSet == true) {
  alarmSet = false;
  alarmStop = true;
  alarmHours = 0;
  alarmMinutes = 1;
  lcd.setCursor(5, 0);
  lcd.print("Alarm");
  lcd.setCursor(4, 1);
  lcd.print("Deleted");
  goback = 1;
  activeButton = 1;
  delay(2000);
  lcd.clear();

  }



}


void stopAlarm() {
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("Alarm");
  lcd.setCursor(4, 1);
  lcd.print("Stopped");
  delay(2000);
  lcd.clear();
  alarmStop = true;
  myDFPlayer.pause();
}


void checkAlarm() {
  if (alarmStop == false) {
    if (alarmHours == lastHour && alarmMinutes == lastMinute && alarmSet == true) {
      setOffAlarm();
      stopAlarm();
    }
  } else if (alarmStop == true) {
    if (alarmHours == lastHour && alarmMinutes != lastMinute && alarmSet == true) {
      alarmStop = false;
    }
  }
}


void setOffAlarm() {
  digitalWrite(backLightPin, HIGH);
  activeButton = 0;
  goback = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Alert Alert");
  lcd.setCursor(1, 1);
  lcd.print("Alert Alert");

  myDFPlayer.play(songN);
  
  while (activeButton == 0) {
    button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
  
    case 4: // This case will execute if the "back" button is pressed
      button = 0;
      activeButton = 1;
      goback = 1;
      break;
    }
    // Leggi lo stato corrente del player audio
  state = myDFPlayer.readState();
  if (state != 1) {
    myDFPlayer.play(songN);
  }
  }
}


///////////////////////
///////////////////////
// end setAlarm/clear/Alarm




///////////////////////
///////////////////////
// start piezo / buzzer


void timedBeep(int beepTime, int beepCount)
{
  for (int i = 0; i < beepCount; i ++)
  {
    digitalWrite(beeper, HIGH);
    delay(beepTime);
    digitalWrite(beeper, LOW);
    delay(beepTime);
  }
}


///////////////////////
///////////////////////
// end piezo / buzzer







///////////////////////
///////////////////////
// start setAlarmSong

void setAlarmSong() {

   lcd.clear();
   lcd.setCursor(4, 0);
   lcd.print("Ringtone");
   lcd.setCursor(4, 1);
   lcd.print("Saving...");
   delay(2000);
   activeButton = 1;
   goback = 1;
   
}



void setAlarmVol() {

  activeButton = 0;
  goback = 0;

  volume = myDFPlayer.readVolume();

  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Current Volume");
  lcd.setCursor(6, 1);
  if (volume < 10) {
    lcd.print("0");
  }
  lcd.print(volume);

  while (activeButton == 0) {
    button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
    case 0: // When button returns as 0 there is no action taken
      break;
    case 1: // This case will execute if the "forward" button is pressed
      button = 0;
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print("Volume");
      lcd.setCursor(4, 1);
      lcd.print("Saving...");
      myDFPlayer.volume(volume);
      delay(2000);
      activeButton = 1;
      goback = 1;
      break;
    case 2:
      button = 0;
      volume++;
      if (volume >= 30) {
        volume = 30;
      }

      lcd.setCursor(6, 1);
      if (volume < 10) {
        lcd.print("0");
      }
      lcd.print(volume);
      break;
    case 3:
      button = 0;
      volume--;
      if (volume <= 0) {
        volume = 0;
      }
      lcd.setCursor(6, 1);
      if (volume < 10) {
        lcd.print("0");
      }
      lcd.print(volume);
      break;
    case 4:
      button = 0;
      activeButton = 1;
      countCancelled("Operation");
      delay(1000);
      break;
    case 5:
      myDFPlayer.play(songN);
      button = 0;
      activeButton = 1;
      break;
    }
  }
}



 ///////////////////////
///////////////////////
// end setAlarmSong
