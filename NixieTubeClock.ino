// Nixie clock (for TFT Display)
//
// Copyright Rob Latour, 2025
// License: MIT
// https://github.com/roblatour/NixieTubeClock
// https://github.com/roblatour/NixieTubeClock
//
// Compile and upload using Arduino IDE (2.0.3 or greater)
//
// Physical board:                 LILYGO T-Display-S3
// Board in Arduino board manager: ESP32S3 Dev Module
//
// Arduino Tools settings:
// USB CDC On Boot:                Enabled
// CPU Frequency:                  240MHz
// USB DFU On Boot:                Enabled
// Core Debug Level:               None
// Erase All Flash Before Upload:  Disabled
// Events Run On:                  Core 1
// Flash Mode:                     QIO 80Mhz
// Flash Size:                     16MB (128MB)
// Arduino Runs On:                Core 1
// USB Firmware MSC On Boot:       Disabled
// PSRAM:                          OPI PSRAM
// Partition Scheme:               16 M Flash (3MB APP/9.9MB FATFS)
// USB Mode:                       Hardware CDC and JTAG
// Upload Mode:                    UART0 / Hardware CDC
// Upload Speed:                   921600
// Programmer                      ESPTool

#include <Arduino.h>
#include <TFT_eSPI.h>          // If using a LilyGo TDisplay S3 please use the TFT_eSPI library found here: https://github.com/Xinyuan-LilyGO/T-Display-S3/tree/main/lib
#include <ESPAsyncWebServer.h> // https://github.com/esphome/ESPAsyncWebServer (put all files in src directory into the ESPAsyncWebServer directory)
#include <WebSocketsClient.h>  // Websockets by Markus Sattler version 2.3.6 https://github.com/Links2004/arduinoWebSockets
#include <ArduinoSort.h>       // Arduino Sort by Emil Vikström version https://github.com/emilv/ArduinoSort
#include <ESP32Time.h>         //
#include <TimeLib.h>           // Time by Michael Margolis version 1.6.1
#include <WiFi.h>
#include <time.h>
#include <EEPROM.h>
#include <esp_sntp.h>
#include <atomic>

#include "pin_config.h"        // found at https://github.com/Xinyuan-LilyGO/T-Display-S3/tree/main/example/factory
#include "user_settings.h"
#include "NixieTubes.h"        // https://github.com/roblatour/NixieTubes


#include "Images/Rob.h"

// SerialMonitor

int serialMonitorSpeed = SERIAL_MONITOR_SPEED;

// Debug

const bool debugIsOn = DEBUG_IS_ON;

// Nixie Tubes
NixieTubes MyNixieTubes;

// Wi-Fi stuff
bool allowAccessToNetwork = ALLOW_ACCESS_TO_NETWORK;

String wifiSSID = "";
String wifiPassword = "";

// Wi-Fi Access Point
const char *AccessPointSSID = "NixieTubeSetup";
IPAddress accessPointIPaddr(123, 123, 123, 123);
IPAddress accessPointIPMask(255, 255, 255, 0);
AsyncWebServer accessPointServer(80);
AsyncEventSource accessPointevents("/events");

// Time stuff

const bool allowAccessToTimeServer = ALLOW_ACCESS_TO_THE_TIME_SERVER;

unsigned long nextTimeCheck;

const char *primaryNTPServer = PRIMARY_TIME_SERVER;
const char *secondaryNTPServer = SECONDARY_TIME_SERVER;
const char *tertiaryNTPServer = TERTIARY_TIME_SERVER;
const char *timeZone = MY_TIME_ZONE;

std::atomic<bool> networkTimeSyncComplete(false);

// Settings stuff
const int TimeAndDate = 0;
const int TimeOnly = 1;
const int DateOnly = 2;
static int displayTimeAndDate = DISPLAY_TIME_AND_DATE;                           // 0: Display the time and date
                                                                                 // 1: Display the time only
                                                                                 // 2: Display the date only
                                                                                 //
static bool displayTimeIn12HourFormat = DISPLAY_TIME_IN_TWELVE_HOUR_FORMAT;      // if set to true for 24 hour time format, set to false for 12 hour time format
                                                                                 //
static bool displayAMPM = DISPLAY_AM_PM;                                         // if displayTimeIn12HourFormat is true, then a AM/PM indicator will be displayed if true otherwise a AM/PM indicator will not be displayed
                                                                                 //
static bool displayTimeWithSeconds = DISPLAY_TIME_WITH_SECONDS;                  // if set to true display seconds, it set to false don't display the seconds
                                                                                 //
static bool displayTimeWith10thsOfASecond = DISPLAY_TIME_WITH_10THS_OF_A_SECOND; // if set to true display 10th of a second, it set to false don't display 10th of a second
                                                                                 //
static int dateFormat = DATE_FORMAT;                                             // Date formats:
                                                                                 //   0: YYYY-MM-DD
                                                                                 //   1: YYYY/MM/DD
                                                                                 //   2: YY-MM-DD
                                                                                 //   3: YY/MM/DD
                                                                                 //   4: MM-DD-YY
                                                                                 //   5: MM/DD/YY
                                                                                 //   6: MM-DD
                                                                                 //   7: MM/DD
                                                                                 //   8: DD-MM
                                                                                 //   9: DD/MM
                                                                                 //
const int showLeadingZero = 0;                                                   //
const int showLeadingZeroAsBlank = 1;                                            //
const int dontShowLeadingZero = 2;                                               //
static int showLeadingZeroTubeStyle = SHOW_LEADING_ZERO_TUBE_STYLE;              // 0: Show a leading zero as a zero tube
                                                                                 // 1: Show a leading zero as a blank tube
                                                                                 // 2: Don't show a leading zero
const int blackBackground = 0;                                                   //
const int whiteBackground = 1;                                                   //
const int colourEverySecond = 2;                                                 //
static int backgroundFormat = BACKGROUND_FORMAT;                                 // Background formats:
                                                                                 //   0: black background
                                                                                 //   1: white background
                                                                                 //   2: unique background colour every second

static int32_t defaultBackgroundColour = TFT_BLACK;
bool showSettingsTriggeredFromAboutWindow = false;

// display stuff
TFT_eSPI showPanel = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&showPanel);

uint32_t noBackgroundColourOverride = TFT_RED;

bool needToDrawTubes;
char timeBuffer[12];
char dateBuffer[10];

const int backgroundColourRotationMethod = BACKGROUND_COLOUR_ROTATION_METHOD;

// EEPROM stuff
const int eepromDataStartingAddress = 0;
const String confirmEEPROMHasBeenIntialize = "EEPROM initialized";

const int eepromMaxSSIDLength = 32;
const int eepromMaxPasswordLength = 128;

const int eepromSettingsStartingAddressForSettings = confirmEEPROMHasBeenIntialize.length() + 1 + eepromMaxSSIDLength + 1 + eepromMaxPasswordLength + 1;

const int numberOfSavedSettingsInEEPROM = 8;

const int eepromMaxSize = eepromSettingsStartingAddressForSettings + numberOfSavedSettingsInEEPROM + 1;

// Button stuff
#define TOP_BUTTON 14   // Button marked 'Key' on LILYGO T-Display-S3
#define BOTTOM_BUTTON 0 // Button marked 'BOT' on LILYGO T-Display-S3

// other misc stuff
int32_t TFT_Width = 320;  // TFT Width
int32_t TFT_Height = 170; // TFT Height

static int lastSignificantTimeChange = -1;

const char *PARAM_INPUT_SSID = "ssid";
const char *PARAM_INPUT_PASSWORD = "password";

const String nothingWasEntered = "**null**";

String inputSSID = nothingWasEntered;
String inputPassword = nothingWasEntered;

bool setupComplete = false;

String currentSSID = "";
String currentPassword = "";
String lastSSIDSelected = "";
String lastPasswordUsed = "";
String availableNetworks = "";

const char htmlHeader[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
 body { background: #555; }
 .content {
   max-width: 500px;
   margin: auto;
   background: white;
   padding: 10px;
   word-wrap: break-word;
 }
 .form-actions {
  display: flex;
  flex-direction: row-reverse; /* reverse the elements inside */
  justify-content: center;
  align-items: center;
}
</style>
</head>
<body>
<div class="content"> 
  <h1><center>Nixie Clock</center></h1>
  <p>
    <form action="/get">
      <center>
)rawliteral";

const char htmlFooter[] PROGMEM = R"rawliteral(
     </center>
    </form>    
  </p>
</div>
<script>
       window.history.replaceState('','','/');
</script>
</body>
</html>
)rawliteral";

const char htmlGetWifiCredentials[] PROGMEM = R"rawliteral(
     <br>
     Network:&nbsp
     <select name="ssid" id="ssid">
     $options$
     </select>
     <br>
     <br>
     Password:&nbsp<input type="password" name="password" maxlength="63"><br>
     <br>     
     <div style="color:red">$errors$</div>
     <br>    
     <input type="submit" alt="Update" value="OK"> 
)rawliteral";

const char htmlConfirmWIFI[] PROGMEM = R"rawliteral(
     The Wi-Fi credentials entered are now being tested.<br>
     <br>
     If they are good, the Nixie Clock will automatically restart and use them from now on.<br><br>
     If in the future they change, just repeat this process by holding down the top button when the Nixie Clock is being powered on.<br>
     <br>
     If they are not good, when prompted please reconnect to the '$ACCESS POINT$' network and try again.<br>
     <br>
     Have a great day!<br>
     <br>
     Please close this webpage.<br>
     <br>
)rawliteral";

void nonBlockingDelay(TickType_t ms)
{

  vTaskDelay(ms / portTICK_PERIOD_MS);
}

void SetupSerialMonitor()
{

  if (debugIsOn)
  {

    Serial.begin(serialMonitorSpeed);

    nonBlockingDelay(500);

    Serial.println("");
    Serial.println("");
    Serial.println("Nixie Clock starting");
  };
}

void SetupButtons()
{

  // Setup buttons on LILYGO T-Display-S3
  pinMode(TOP_BUTTON, INPUT_PULLUP);
  pinMode(BOTTOM_BUTTON, INPUT_PULLUP);
}

bool checkButton(int pinNumber)
{

  bool returnValue = false;

  if (digitalRead(pinNumber) == 0)
  {

    delay(10); // weed out false positives caused by debounce

    if (digitalRead(pinNumber) == 0)
    {

      // hold here until the button is released
      while (digitalRead(pinNumber) == 0)
        delay(10);

      returnValue = true;
    };
  };

  return returnValue;
}

bool IsTheTopButtonPressed()
{
  return checkButton(TOP_BUTTON);
}

bool IsTheBottomButtonPressed()
{
  return checkButton(BOTTOM_BUTTON);
}

void clearDisplay(uint32_t backgroundColourOverride, bool immediatelyClearDisplay = true)
{

  if (backgroundColourOverride == noBackgroundColourOverride)
  {

    if (backgroundFormat == blackBackground)
      sprite.fillSprite(TFT_BLACK);
    else if (backgroundFormat == whiteBackground)
      sprite.fillSprite(TFT_WHITE);
    else
      sprite.fillSprite(GetTimeSensitiveBackgroundColour());
  }
  else
  {
    sprite.fillSprite(backgroundColourOverride);
  };

  if (immediatelyClearDisplay)
  {
    needToDrawTubes = true;
    drawDisplay();
  };
}

void initializeEEPROM()
{

  if (debugIsOn)
    Serial.println("Initializing EEPROM");

  for (int i = 0; i < confirmEEPROMHasBeenIntialize.length(); i++)
    EEPROM.write(eepromDataStartingAddress + i, confirmEEPROMHasBeenIntialize[i]);

  for (int i = confirmEEPROMHasBeenIntialize.length(); i < eepromMaxSize; i++)
    EEPROM.write(eepromDataStartingAddress + i, 0);

  loadDefaultSettings();
  StoreSettingsInNonVolatileMemory();

  EEPROM.commit();
  delay(1000);
}

void clearWifiCredentialsfromNonVolatileMemory()
{

  if (debugIsOn)
    Serial.println("Clearing Wi-Fi credentials from non volatile memory");

  const int WifiSSIDStartingAddress = eepromDataStartingAddress + confirmEEPROMHasBeenIntialize.length() + 1;
  const int WifiPasswordStartingAddress = WifiSSIDStartingAddress + eepromMaxSSIDLength + 1;

  for (int i = 0; i < eepromMaxSSIDLength; i++)
    EEPROM.write(WifiSSIDStartingAddress + i, 0);

  for (int i = 0; i < eepromMaxPasswordLength; i++)
    EEPROM.write(WifiPasswordStartingAddress + i, 0);

  EEPROM.commit();
  delay(1000);
}

void StoreWifiSSIDandPasswordInNonVolatileMemory()
{

  const int WifiSSIDStartingAddress = eepromDataStartingAddress + confirmEEPROMHasBeenIntialize.length() + 1;
  const int WifiPasswordStartingAddress = WifiSSIDStartingAddress + eepromMaxSSIDLength + 1;

  bool commitRequired = false;

  for (int i = 0; i < wifiSSID.length(); i++)
  {
    if (EEPROM.read(WifiSSIDStartingAddress + i) != wifiSSID[i])
    {
      EEPROM.write(WifiSSIDStartingAddress + i, wifiSSID[i]);
      commitRequired = true;
    };
  };

  for (int i = 0; i < wifiPassword.length(); i++)
  {
    if (EEPROM.read(WifiPasswordStartingAddress + i) != wifiPassword[i])
    {
      EEPROM.write(WifiPasswordStartingAddress + i, wifiPassword[i]);
      commitRequired = true;
    };
  };

  if (commitRequired)
    EEPROM.commit();
}

void LoadWifiSSIDandPasswordFromNonVolatileMemory()
{

  const int SSIDStartingAddress = eepromDataStartingAddress + confirmEEPROMHasBeenIntialize.length() + 1;
  const int WifiStartingAddress = SSIDStartingAddress + eepromMaxSSIDLength + 1;

  char c;
  int i;

  wifiSSID = "";
  wifiPassword = "";

  i = 0;
  c = EEPROM.read(SSIDStartingAddress + i++);
  while (c != 0)
  {
    wifiSSID.concat(String(c));
    c = EEPROM.read(SSIDStartingAddress + i++);
  };

  i = 0;
  c = EEPROM.read(WifiStartingAddress + i++);
  while (c != 0)
  {
    wifiPassword.concat(String(c));
    c = EEPROM.read(WifiStartingAddress + i++);
  };
}

void loadDefaultSettings()
{

  displayTimeAndDate = DISPLAY_TIME_AND_DATE;
  displayTimeIn12HourFormat = DISPLAY_TIME_IN_TWELVE_HOUR_FORMAT;
  displayTimeWithSeconds = DISPLAY_TIME_WITH_SECONDS;
  displayAMPM = DISPLAY_AM_PM;
  displayTimeWith10thsOfASecond = DISPLAY_TIME_WITH_10THS_OF_A_SECOND;
  dateFormat = DATE_FORMAT;
  showLeadingZeroTubeStyle = SHOW_LEADING_ZERO_TUBE_STYLE;
  backgroundFormat = BACKGROUND_FORMAT;
};

void StoreSettingsInNonVolatileMemory()
{

  byte Setting[numberOfSavedSettingsInEEPROM];

  Setting[0] = displayTimeAndDate;
  Setting[1] = (displayTimeIn12HourFormat == 0);
  Setting[2] = (displayTimeWithSeconds == 0);
  Setting[3] = (displayAMPM == 0);
  Setting[4] = (displayTimeWith10thsOfASecond == 0);
  Setting[5] = dateFormat;
  Setting[6] = showLeadingZeroTubeStyle;
  Setting[7] = backgroundFormat;

  // write the settings to their respective eeprom storage locations only if they need updating

  bool commitRequired = false;

  for (int i = 0; i < numberOfSavedSettingsInEEPROM; i++)
  {
    if (EEPROM.read(eepromSettingsStartingAddressForSettings + i) != Setting[i])
    {
      EEPROM.write(eepromSettingsStartingAddressForSettings + i, Setting[i]);
      commitRequired = true;
    };
  };

  if (commitRequired)
    EEPROM.commit();
};

void LoadSettingsFromNonVolatileMemory()
{

  byte Setting[numberOfSavedSettingsInEEPROM];

  for (int i = 0; i < numberOfSavedSettingsInEEPROM; i++)
    Setting[i] = EEPROM.read(eepromSettingsStartingAddressForSettings + i);

  displayTimeAndDate = Setting[0];
  displayTimeIn12HourFormat = (Setting[1] == 0);
  displayTimeWithSeconds = (Setting[2] == 0);
  displayAMPM = (Setting[3] == 0);
  displayTimeWith10thsOfASecond = (Setting[4] == 0);
  dateFormat = Setting[5];
  showLeadingZeroTubeStyle = Setting[6];
  backgroundFormat = Setting[7];
}

bool confirmUserWantsToResetEEPROM()
{

  bool returnValue;

  if (debugIsOn)
    Serial.println("Confirming startup EEPROM reset");

  // Display the confirmation screen

  clearDisplay(TFT_BLACK);

  sprite.setTextColor(TFT_WHITE, TFT_BLACK);

  sprite.setTextDatum(TR_DATUM);
  sprite.drawString("Yes ->", TFT_Width, 11, 1);

  sprite.setTextDatum(TL_DATUM);
  sprite.drawString("Would you like to clear your Wi-Fi credentials?", 5, 77, 2);

  sprite.setTextDatum(BR_DATUM);
  sprite.drawString("No ->", TFT_Width, TFT_Height - 12, 1);

  drawDisplay();

  while (true)
  {

    if (IsTheTopButtonPressed())
    {
      returnValue = true;
      break;
    };

    if (IsTheBottomButtonPressed())
    {
      returnValue = false;
      break;
    };
  }

  return returnValue;
};

void SetupEEPROM()
{

  // The EEPROM is used to save and store settings so that they are retained when the device is powered off and restored when it is powered back on
  // data is stored in the EEPROM as follows:
  //
  //      starting at eepromInitializationConfirmationAddress:
  //
  //      a string with the value of "EEPROM initialized" (without the quotes)
  //
  //      (eepromMaxSSIDLength + 1) bytes; used to store the Wi-Fi network name (SSID) as a string
  //
  //      (epromMaxPasswordLength + 1) bytes; used to store the Wi-Fi password as a string
  //
  //      the following number of bytes: numberOfSavedSettingsInEEPROM, used as follows:
  //          displayTimeAndDate
  //          displayTimeIn12HourFormat
  //          displayAMPM
  //          displayTimeWithSeconds
  //          displayTimeWith10thsOfASecond
  //          dateFormat
  //          showLeadingZeroTubeStyle
  //          backgroundFormat
  //
  EEPROM.begin(eepromMaxSize);

  // if the  user is holding down the top or bottom button at power on, check if they want to clear the the Wi-Fi credentials
  bool userRequestingWifiReset = (checkButton(TOP_BUTTON) || checkButton(BOTTOM_BUTTON));
  bool userConfirmedWifiReset;

  if (userRequestingWifiReset)
    userConfirmedWifiReset = confirmUserWantsToResetEEPROM();

  bool eepromHasBeenInitialize = true;
  // test that the EEPROM has been initialized; if it has not been initialized then do that now
  for (int i = 0; i < confirmEEPROMHasBeenIntialize.length(); i++)
    if (EEPROM.read(i + eepromDataStartingAddress) != confirmEEPROMHasBeenIntialize[i])
      eepromHasBeenInitialize = false;

  if (!eepromHasBeenInitialize)
    initializeEEPROM();
  else if (userConfirmedWifiReset)
    clearWifiCredentialsfromNonVolatileMemory();

  LoadWifiSSIDandPasswordFromNonVolatileMemory();
  LoadSettingsFromNonVolatileMemory();
};

void drawDisplay()
{

  sprite.pushSprite(0, 0);
}

uint32_t GetTimeSensitiveBackgroundColour()
{

  // note: the colour range for a TFT display is red (0-31); green (0-63); blue (0-31)

  static uint32_t red = random(32);
  static uint32_t green = random(64);
  static uint32_t blue = random(32);

  const int lastRed = 0;
  const int lastGreen = 1;
  const int lastBlue = 2;
  static int ColourToChange = lastBlue;

  switch (backgroundColourRotationMethod)
  {

  // Method 0:
  // Start at a random colour and then progressively rotate through all feasable colours each time this routine is called
  // increasing red, then green, then blue each in turn
  case 0:
  {

    if (ColourToChange == lastRed)
    {
      red++;
      if (red > 31)
        red = 0;
      ColourToChange = lastGreen;
    }
    else
    {
      if (ColourToChange == lastGreen)
      {
        green++;
        if (green > 63)
          green = 0;
        ColourToChange = lastBlue;
      }
      else
      {
        blue++;
        if (blue > 31)
          blue = 0;
        ColourToChange = lastRed;
      };
    };

    break;
  }

  // Method 1:
  // start at a random colour and then progressively rotate through all feasable colours each time this routine is called
  // by increasing red from 0 to 31, then within that green from 0 to 63, then within that blue from 0 to 31
  case 1:
  {

    red++;
    if (red > 31)
    {
      red = 0;
      green++;
      if (green > 63)
      {
        green = 0;
        blue++;
        if (blue > 31)
        {
          blue = 0;
        };
      };
    };

    break;
  }

    // Method 2: each second of the day will have a unique colour associated with it
    // red will be used for hours, blue will be used for minutes, green will be used for seconds

  case 2:
  {

    struct timeval tvTime;
    gettimeofday(&tvTime, NULL);

    int iTotal_seconds = tvTime.tv_sec;
    struct tm *ptm = localtime((const time_t *)&iTotal_seconds);

    int iHour = ptm->tm_hour;
    int iMinute = ptm->tm_min;
    int iSec = ptm->tm_sec;

    uint32_t red = iHour + 3;
    uint32_t blue = (iMinute / 2) + 1;
    uint32_t green = iSec + 1;

    break;
  };
  };

  uint32_t result = (blue << (6 + 5)) | (green << 5) | red;

  return result;
}

void SetupDisplay()
{

  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);

  // Setup show on LILYGO T-Display-S3
  showPanel.init();

  showPanel.begin();

  sprite.createSprite(TFT_Width, TFT_Height);
  sprite.setSwapBytes(true);

  MyNixieTubes.setDisplayDimensions(TFT_Width, TFT_Height);

  showPanel.setRotation(1); // 0 = 0 degrees , 1 = 90 degrees, 2 = 180 degrees, 3 = 270 degrees

  randomSeed(analogRead(0));

  clearDisplay(TFT_BLACK);

  // Opening messages
  /*

  if (String(OPENING_MESSAGE_LINE1).length() > 0)
    MyNixieTubes.DrawNixieTubes(sprite, NixieTubes::small, 0, 7, true, false, OPENING_MESSAGE_LINE1);

  if (String(OPENING_MESSAGE_LINE2).length() > 0)
    MyNixieTubes.DrawNixieTubes(sprite, NixieTubes::small, 0, 88, true, false, OPENING_MESSAGE_LINE2);

  if ((String(OPENING_MESSAGE_LINE1).length() > 0) || (String(OPENING_MESSAGE_LINE2).length() > 0)) {
    drawDisplay();
    delay(5000);
    clearDisplay(TFT_BLACK);
  };

  */
}

bool SetupWiFiWithExistingCredentials(bool updateDisplay = true)
{

  bool notyetconnected = true;
  int attempt = 1;
  int waitThisManySecondsForAConnection;
  float timeWaited;

  const int leftBoarder = 0;
  const int displayLineNumber[] = {0, 16, 32, 48, 64, 80};
  int lineCounter = 0;

  String message;

  if (updateDisplay)
  {
    clearDisplay(TFT_BLACK);

    showPanel.setTextColor(WIFI_CONNECTING_STATUS_COLOUR, TFT_BLACK);
    sprite.setTextDatum(TL_DATUM); // position text at the top left

    message = "Attempting to connect to ";
    message.concat(wifiSSID);
  };

  if (debugIsOn)
    Serial.println(message);

  if (updateDisplay)
    showPanel.drawString(message, leftBoarder, displayLineNumber[0], 2);

  const int maxConnectionAttempts = 5;

  while ((notyetconnected) && (attempt <= maxConnectionAttempts))
  {

    waitThisManySecondsForAConnection = 5 * attempt;

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

    unsigned long startedWaiting = millis();
    unsigned long waitUntil = startedWaiting + (waitThisManySecondsForAConnection * 1000);

    while ((WiFi.status() != WL_CONNECTED) && (millis() < waitUntil))
    {

      timeWaited = millis() - startedWaiting;

      if (updateDisplay)
      {
        message = "Attempt ";
        message.concat(attempt);
        message.concat(" of ");
        message.concat(String(maxConnectionAttempts));
        message.concat("    "); // add a few spaces to the end of the message to effectively clear the balance of the current line

        showPanel.drawString(message, leftBoarder, displayLineNumber[2], 2);

        message = "Waited ";

        message.concat(String(timeWaited / 1000, 1));

        message.concat(" seconds in this attempt");
        message.concat("     "); // add a few spaces to the end of the message to effectively clear the balance of the show line

        showPanel.drawString(message, leftBoarder, displayLineNumber[3], 2);
      }
      else
      {

        if (setupComplete)
          UpdateClock();
      };
    };

    if (WiFi.status() == WL_CONNECTED)
      notyetconnected = false;
    else
    {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    };

    attempt++;
  };

  if (notyetconnected)
  {

    message = "Could not conenct";

    if (debugIsOn)
      Serial.println(message);
    if (updateDisplay)
      showPanel.drawString(message, leftBoarder, displayLineNumber[5], 2);
  }
  else
  {

    message = "Connected at IP address ";
    message.concat(WiFi.localIP().toString());

    if (debugIsOn)
      Serial.println(message);

    if (updateDisplay)
      showPanel.drawString(message, leftBoarder, displayLineNumber[5], 2);
  };

  return !notyetconnected;
};

void scanAvailableNetworks()
{

  if (debugIsOn)
    Serial.println("** Scan Networks **");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int numSsid = WiFi.scanNetworks();

  if (numSsid < 1)
  {

    if (debugIsOn)
      Serial.println("Couldn't find any Wi-Fi networks");
  }
  else
  {

    // print the list of networks seen:
    if (debugIsOn)
    {
      Serial.print("Number of available networks: ");
      Serial.println(numSsid);
    };

    // build list of options for Wi-Fi selection window
    // format per SSID found:   <option value="SSID">SSID</option>

    String discoveredSSIDs[numSsid];

    for (int i = 0; i < numSsid; i++)
    {

      if (debugIsOn)
      {
        Serial.print(i);
        Serial.print(") ");
        Serial.print(WiFi.SSID(i));
        Serial.print("\tSignal: ");
        Serial.print(WiFi.RSSI(i));
        Serial.println(" dBm");
      };

      discoveredSSIDs[i] = WiFi.SSID(i);
    };

    sortArray(discoveredSSIDs, numSsid);

    availableNetworks = "";
    String lastEntryAdded = "";

    for (int i = 0; i < numSsid; i++)
    {

      // ensure duplicate SSIDS are not added twice
      if (discoveredSSIDs[i] != lastEntryAdded)
      {

        lastEntryAdded = discoveredSSIDs[i];

        availableNetworks.concat("<option value=\"");
        availableNetworks.concat(discoveredSSIDs[i]);
        availableNetworks.concat("\">");
        availableNetworks.concat(discoveredSSIDs[i]);
        availableNetworks.concat("</option>");
      };
    };
  };
}

bool SetupWifiWithNewCredentials()
{

  const int leftBoarder = 0;

  int MaxLines = 20;
  int displayLineNumber[MaxLines];
  int lineCounter = 0;

  for (int i = 0; i < MaxLines; i++)
    displayLineNumber[i] = i * 16;

  clearDisplay(TFT_BLACK);
  showPanel.setTextColor(WIFI_CONNECTING_STATUS_COLOUR, TFT_BLACK);
  sprite.setTextDatum(TL_DATUM); // position text at the top left

  String message = "Nixie Clock setup ...";
  showPanel.drawString(message, leftBoarder, displayLineNumber[lineCounter++], 2);

  scanAvailableNetworks();

  bool accessPointNeedsToBeConfigured = true;

  while (accessPointNeedsToBeConfigured)
  {

    if (debugIsOn)
      Serial.println("Configuring access point...");

    WiFi.softAP(AccessPointSSID);
    WiFi.softAPConfig(accessPointIPaddr, accessPointIPaddr, accessPointIPMask);
    IPAddress myIP = WiFi.softAPIP();

    if (debugIsOn)
    {
      Serial.print("Access Point IP Address: ");
      Serial.println(myIP);
    };

    accessPointServer.onNotFound([](AsyncWebServerRequest *request)
                                 {
      if (debugIsOn)
        Serial.println("Not found: " + String(request->url()));
      request->redirect("/"); });

    accessPointServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                         {
      String html = String(htmlHeader);
      html.concat(htmlGetWifiCredentials);
      html.replace("$ssid$", "");
      html.replace("$errors$", " ");
      html.replace("$options$", availableNetworks);
      html.concat(htmlFooter);
      request->send(200, "text/html", html); });

    accessPointServer.on("/confirmed", HTTP_GET, [](AsyncWebServerRequest *request)
                         {
      String html = String(htmlHeader);
      html.concat(htmlConfirmWIFI);
      html.replace("$ACCESS POINT$", String(AccessPointSSID));
      html.concat(htmlFooter);
      request->send(200, "text/html", html); });

    accessPointServer.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
                         {
      String html = String(htmlHeader);
      String errorMessage = "";

      if (request->hasParam(PARAM_INPUT_SSID)) {
        inputSSID = request->getParam(PARAM_INPUT_SSID)->value();
        if (inputSSID.length() == 0) {
          inputSSID = nothingWasEntered;
        };
      } else {
        inputSSID = nothingWasEntered;
      };

      if (inputSSID != nothingWasEntered)
        lastSSIDSelected = inputSSID;

      if (request->hasParam(PARAM_INPUT_PASSWORD)) {
        inputPassword = request->getParam(PARAM_INPUT_PASSWORD)->value();
        if (inputPassword.length() == 0) {
          inputPassword = nothingWasEntered;
        };
      } else {
        inputPassword = nothingWasEntered;
      };

      if (inputPassword != nothingWasEntered)
        lastPasswordUsed = inputPassword;

      if (debugIsOn) {
        Serial.println("------------------");
        Serial.println(inputSSID);
        Serial.println(inputPassword);
        Serial.println("------------------");
      };

      if (inputSSID == nothingWasEntered)
        errorMessage.concat("The SSID was not selected");

      if (inputPassword == nothingWasEntered)
        if (errorMessage.length() == 0)
          errorMessage.concat("The password was not entered");
        else
          errorMessage.concat(";<br>the password was not entered");

      if (errorMessage.length() == 0) {

        currentSSID = inputSSID;
        currentPassword = inputPassword;

        inputSSID = "";
        inputPassword = "";

        request->redirect("/confirmed");

      } else {

        errorMessage.concat(".");
        const String quote = String('"');
        html.concat(htmlGetWifiCredentials);
        html.replace("$options$", availableNetworks);

        if (lastSSIDSelected.length() > 0) {
          String original = "<option value=" + quote + lastSSIDSelected + quote + ">" + lastSSIDSelected + "</option>";
          String revised_ = "<option value=" + quote + lastSSIDSelected + quote + " selected=" + quote + "selected" + quote + ">" + lastSSIDSelected + "</option>";
          html.replace(original, revised_);
        };

        if (lastPasswordUsed.length() > 0) {
          String original = "name=" + quote + "password" + quote;
          String revised_ = "name=" + quote + "password" + quote + " value=" + quote + lastPasswordUsed + quote;
          html.replace(original, revised_);
        };

        html.replace("$errors$", errorMessage);

        html.concat(htmlFooter);
        request->send(200, "text/html", html);
      }; });

    accessPointevents.onConnect([](AsyncEventSourceClient *client)
                                {
      if (debugIsOn)
        Serial.println("Access Point server connected"); });

    accessPointServer.addHandler(&accessPointevents);

    accessPointServer.begin();

    if (debugIsOn)
      Serial.println("Access Point server started");

    lineCounter++;

    message = "Step 1: Connect to Wi-Fi network " + String(AccessPointSSID);
    showPanel.drawString(message, leftBoarder, displayLineNumber[lineCounter++], 2);

    lineCounter++;

    message = "Step 2: Open your browser in Incognito mode";
    showPanel.drawString(message, leftBoarder, displayLineNumber[lineCounter++], 2);

    lineCounter++;

    message = "Step 3: Browse to http://" + myIP.toString();
    showPanel.drawString(message, leftBoarder, displayLineNumber[lineCounter++], 2);

    lineCounter++;

    message = "Step 4: Enter your Wi-Fi network information";
    showPanel.drawString(message, leftBoarder, displayLineNumber[lineCounter++], 2);

    if (debugIsOn)
      Serial.println("Waiting for user to update Wi-Fi info in browser");

    // assumes network requires a password

    while ((currentSSID == "") || (currentPassword == ""))
    {

      if (checkButton(TOP_BUTTON) || checkButton(BOTTOM_BUTTON))
      {
        // early out
        allowAccessToNetwork = false;
        setTimeAndDateToTheTurnOfTheCentury();
        return false;
      };

      delay(10);
    };

    delay(1000);

    String newSSID = currentSSID;
    String newPassword = currentPassword;

    wifiSSID = newSSID;
    wifiPassword = newPassword;

    // set the current SSID and Passwordto null
    // these will be reloaded if the Wi-Fi connection can be established
    currentSSID = "";
    currentPassword = "";

    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);

    while (WiFi.status() == WL_CONNECTED)
      delay(100);

    if (debugIsOn)
      Serial.println("user has updated Wi-Fi info in browser");

    // confirm Wi-Fi credentials

    bool WifiSetupSucceeded = false;

    if (SetupWiFiWithExistingCredentials())
    {

      WifiSetupSucceeded = true;

      StoreWifiSSIDandPasswordInNonVolatileMemory();

      message = "Wi-Fi name and password confirmed!";
      showPanel.drawString(message, leftBoarder, displayLineNumber[lineCounter++], 2);
      delay(5000);

      accessPointNeedsToBeConfigured = false;

      return true;
    }
    else
    {

      lineCounter++;

      message = "Failed to connect to Wi-Fi";
      showPanel.drawString(message, leftBoarder, displayLineNumber[lineCounter++], 2);

      lineCounter++;

      message = "Automatically restarting in ten seconds ... ";
      showPanel.drawString(message, leftBoarder, displayLineNumber[lineCounter++], 2);

      delay(10000);
      ESP.restart();
    };
  };
};

// ************************************************************************************************************************************************************

bool setLocalTimeFromNTPTime()
{

  bool returnValue = false;

  struct tm timeinfo;

  if (getLocalTime(&timeinfo, 10000U))
  {
    time_t t = mktime(&timeinfo);
    struct timeval now = {.tv_sec = t};
    settimeofday(&now, NULL);
    if (debugIsOn)
      Serial.printf("Time set to: %s", asctime(&timeinfo));
    returnValue = true;
  }
  else
  {
    if (debugIsOn)
      Serial.println("Failed to obtain local time");
  };

  return returnValue;
}

void setTimeAndDateToTheTurnOfTheCentury()
{

  Serial.println("Setting time to the turn of the century.");

  ESP32Time rtc(0);

  int Year = 2000;
  int Month = 1;
  int Day = 1;
  int Hour = 0;
  int Minute = 0;
  int Second = 0;

  rtc.setTime(Second, Minute, Hour, Day, Month, Year);
}

void printNTPStatus()
{
  Serial.println("NTP Debug Info:");
  Serial.print("Primary NTP: ");
  Serial.println(primaryNTPServer);
  Serial.print("Secondary NTP: ");
  Serial.println(secondaryNTPServer);
  Serial.print("Tertiary NTP: ");
  Serial.println(tertiaryNTPServer);
  Serial.print("Timezone: ");
  Serial.println(timeZone);
  Serial.print("WiFi Status: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
}

void SetupNTPTime()
{

  const unsigned long oneDayFromNow = 24 * 60 * 60 * 1000;

  if (debugIsOn)
    Serial.println("Setting time ... ");

  bool timeWasSuccessfullySet = false;

  if (allowAccessToTimeServer)
  {

    tm *timeinfo;
    timeval tv;
    time_t now;

    networkTimeSyncComplete = false;

    sntp_set_time_sync_notification_cb([](struct timeval *tv)
                                       { networkTimeSyncComplete = true; });

    configTzTime(timeZone, primaryNTPServer, secondaryNTPServer, tertiaryNTPServer);

    if (debugIsOn)
    {
      Serial.println("");
      Serial.println("Waiting for time synchronization");
    };

    const unsigned long fortySecondTimeout = 40000UL;
    unsigned long startWaitTime = millis();

    while (!networkTimeSyncComplete && (WiFi.status() == WL_CONNECTED) && ((millis() - startWaitTime) < fortySecondTimeout))
    {
      if (debugIsOn)
        Serial.print("*");
      nonBlockingDelay(10);
    }

    if (debugIsOn)
      Serial.println("");

    if (networkTimeSyncComplete)
    {

      time(&now);
      timeinfo = localtime(&now);

      if (debugIsOn)
      {
        Serial.print("Time synchronized to: ");
        Serial.println(String(asctime(timeinfo)));
      }
      else
      {
        if (debugIsOn)
          Serial.print("Time synchronization failed!");
      }

      printNTPStatus();

      timeWasSuccessfullySet = setLocalTimeFromNTPTime();
    }
    else
    {
      Serial.println("Allow access to time server disallowed in settings ");
      setTimeAndDateToTheTurnOfTheCentury();
    };

    if (!timeWasSuccessfullySet)
    {

      if (debugIsOn)
        Serial.println("Time could not be set from NTP server");

      setTimeAndDateToTheTurnOfTheCentury();
    };

    nextTimeCheck = millis() + oneDayFromNow;
  }
}

void SetupTimeAndDate()
{

  if (allowAccessToNetwork)
  {

    bool wifiWasConnected;

    if (wifiSSID.length() == 0)
      wifiWasConnected = SetupWifiWithNewCredentials();
    else
      wifiWasConnected = SetupWiFiWithExistingCredentials();

    if (wifiWasConnected)
    {
      nonBlockingDelay(1000);
      SetupNTPTime();
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    }
    else
    {
      setTimeAndDateToTheTurnOfTheCentury();
    }
  }
  else
    setTimeAndDateToTheTurnOfTheCentury();
}

void RefreshTimeOnceADay()
{

  if (allowAccessToNetwork && allowAccessToTimeServer)
  {

    if (millis() > nextTimeCheck)
    {

      Serial.println("Next time check reached");

      const unsigned long oneDayFromNow = 24 * 60 * 60 * 1000;
      const unsigned long oneHourFromNow = 60 * 60 * 1000;

      bool timeWasSuccessfullySet = false;

      if (SetupWiFiWithExistingCredentials(false))
      {
        timeWasSuccessfullySet = setLocalTimeFromNTPTime();
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
      };

      if (timeWasSuccessfullySet)
        nextTimeCheck = millis() + oneDayFromNow;
      else
        nextTimeCheck = millis() + oneHourFromNow;
    };
  };
}

void showClock()
{

  static int lastdisplayOption = 0;
  static int lastLine1Length;
  static int lastLine2Length;
  static bool colonFlash;

  int tubeSize;

  String line1;
  String line2;
  String ampmIndicator;

  struct timeval tvTime;

  gettimeofday(&tvTime, NULL);

  int iTotal_seconds = tvTime.tv_sec;
  struct tm *ptm = localtime((const time_t *)&iTotal_seconds);

  int iYear = ptm->tm_year - 100;
  int iMonth = ptm->tm_mon + 1;
  int iDay = ptm->tm_mday;
  int iHour = ptm->tm_hour;
  int iMinute = ptm->tm_min;
  int iSecond = ptm->tm_sec;
  int iTenthOfASecond = tvTime.tv_usec / 100000;

  if (iHour < 12)
    ampmIndicator = "AM";
  else
    ampmIndicator = "PM";

  if ((displayTimeIn12HourFormat) && (iHour > 12))
    iHour -= 12;

  // Time

  if ((displayTimeAndDate == TimeAndDate) || (displayTimeAndDate == TimeOnly))
  {

    if (displayTimeWith10thsOfASecond)
      sprintf(timeBuffer, "%02d:%02d:%02d.%01d", iHour, iMinute, iSecond, iTenthOfASecond);
    // else if ((displayTimeWithSeconds && displayAMPM) && (iHour < 10))
    //   sprintf(timeBuffer, "%01d:%02d:%02d %s", iHour, iMinute, iSecond, ampmIndicator);
    else if (displayTimeWithSeconds && displayAMPM)
      sprintf(timeBuffer, "%02d:%02d:%02d%s", iHour, iMinute, iSecond, ampmIndicator);
    else if (displayTimeWithSeconds && !displayAMPM)
      sprintf(timeBuffer, "%02d:%02d:%02d", iHour, iMinute, iSecond);
    else if (!displayTimeWithSeconds && displayAMPM)
      sprintf(timeBuffer, "%02d:%02d%s", iHour, iMinute, ampmIndicator);
    else
      sprintf(timeBuffer, "%02d:%02d", iHour, iMinute);

    if (!displayTimeWithSeconds)
    {
      colonFlash = !colonFlash;
      if (colonFlash)
        timeBuffer[2] = ' ';
    };

    if (displayTimeIn12HourFormat)
    {

      if ((timeBuffer[0] == '0') & (timeBuffer[1] == '0'))
      {
        timeBuffer[0] = '1';
        timeBuffer[1] = '2';
      };
    };

    if ((showLeadingZeroTubeStyle == showLeadingZeroAsBlank) || (showLeadingZeroTubeStyle == dontShowLeadingZero))
      if (timeBuffer[0] == '0')
        timeBuffer[0] = ' ';

    line1 = String(timeBuffer);
  };

  // Date
  if ((displayTimeAndDate == TimeAndDate) || (displayTimeAndDate == DateOnly))
  {

    switch (dateFormat)
    {

    case 0:
    {
      int dYear = 2000 + iYear;
      sprintf(dateBuffer, "%04d-%02d-%02d", dYear, iMonth, iDay);
      break;
    }

    case 1:
    {
      int dYear = 2000 + iYear;
      sprintf(dateBuffer, "%04d/%02d/%02d", dYear, iMonth, iDay);
      break;
    }

    case 2:
    {
      sprintf(dateBuffer, "%02d-%02d-%02d", iYear, iMonth, iDay);
      break;
    }

    case 3:
    {
      sprintf(dateBuffer, "%02d/%02d/%02d", iYear, iMonth, iDay);
      break;
    }

    case 4:
    {
      sprintf(dateBuffer, "%02d-%02d-%02d", iMonth, iDay, iYear);
      break;
    }

    case 5:
    {
      sprintf(dateBuffer, "%02d/%02d/%02d", iMonth, iDay, iYear);
      break;
    }

    case 6:
    {
      sprintf(dateBuffer, "%02d-%02d", iMonth, iDay);
      break;
    }

    case 7:
    {
      sprintf(dateBuffer, "%02d/%02d", iMonth, iDay);
      break;
    }

    case 8:
    {
      sprintf(dateBuffer, "%02d-%02d", iDay, iMonth);
      break;
    }

    case 9:
    {
      sprintf(dateBuffer, "%02d/%02d", iDay, iMonth);
      break;
    }
    };

    if ((showLeadingZeroTubeStyle == showLeadingZeroAsBlank) || (showLeadingZeroTubeStyle == dontShowLeadingZero))
      if (dateBuffer[0] == '0')
        dateBuffer[0] = ' ';

    if (displayTimeAndDate == DateOnly)
      line1 = String(dateBuffer);
    else
      line2 = String(dateBuffer);
  };

  if (showLeadingZeroTubeStyle == dontShowLeadingZero)
  {
    line1.trim();
    line2.trim();
  };

  tubeSize = MyNixieTubes.large;

  bool displayOptionIncludesTwoLines = (displayTimeAndDate == TimeAndDate);

  int maxTubesToBeDrawnAcrossTheDisplay = max(line1.length(), line2.length());

  if (maxTubesToBeDrawnAcrossTheDisplay > 5)
    tubeSize = MyNixieTubes.medium;

  if (displayOptionIncludesTwoLines || (maxTubesToBeDrawnAcrossTheDisplay > 8))
    tubeSize = MyNixieTubes.small;

  bool useTimeSensitiveBackground = (backgroundFormat == colourEverySecond);

  bool transparent = (backgroundFormat != blackBackground);

  if (transparent)
  {
    needToDrawTubes = true;
    clearDisplay(noBackgroundColourOverride, false);
  }
  else if ((lastLine1Length != line1.length()) || (lastLine2Length != line2.length()))
  {
    lastLine1Length = line1.length();
    lastLine2Length = line2.length();
    needToDrawTubes = true;
    clearDisplay(noBackgroundColourOverride, false);
  };

  int xOffset, yOffset;
  int screenWidth, screenHeight;
  MyNixieTubes.getScreenDimensions(tubeSize, screenWidth, screenHeight);

  if (displayOptionIncludesTwoLines)
  {

    xOffset = (TFT_Width / 2) - ((screenWidth * line1.length()) / 2);
    yOffset = (TFT_Height - (2 * screenHeight)) / 4;

    MyNixieTubes.DrawNixieTubes(sprite, tubeSize, xOffset, yOffset, needToDrawTubes, transparent, line1);

    xOffset = (TFT_Width / 2) - ((screenWidth * line2.length()) / 2);
    yOffset += (TFT_Height / 2);

    MyNixieTubes.DrawNixieTubes(sprite, tubeSize, xOffset, yOffset, needToDrawTubes, transparent, line2);
  }
  else
  {
    xOffset = (TFT_Width / 2) - ((screenWidth * line1.length()) / 2);
    yOffset = (TFT_Height / 2) - (screenHeight / 2);
    MyNixieTubes.DrawNixieTubes(sprite, tubeSize, xOffset, yOffset, needToDrawTubes, transparent, line1);
  };

  drawDisplay();
  needToDrawTubes = false;
}

// ************************************************************************************************************************************************************

void UpdateClock()
{

  struct timeval tvTime;
  gettimeofday(&tvTime, NULL);
  int iTotal_seconds = tvTime.tv_sec;
  struct tm *ptm = localtime((const time_t *)&iTotal_seconds);

  bool timeIsBeingShown = (displayTimeAndDate != DateOnly);

  if (timeIsBeingShown && displayTimeWith10thsOfASecond)
  {

    // update frequency: every 10th of a second

    int iTenthOfASecond = tvTime.tv_usec / 100000;

    if (iTenthOfASecond != lastSignificantTimeChange)
    {
      lastSignificantTimeChange = iTenthOfASecond;
      showClock();
    };
  }
  else
  {

    if (timeIsBeingShown || (backgroundFormat == colourEverySecond))
    {
      // update frequency: every second

      int iSecond = ptm->tm_sec;

      if (iSecond != lastSignificantTimeChange)
      {
        lastSignificantTimeChange = iSecond;
        showClock();
      };
    }
    else
    {
      // update frequency: every day

      int iDay = ptm->tm_mday;

      if (iDay != lastSignificantTimeChange)
      {
        lastSignificantTimeChange = iDay;
        showClock();
      };
    };
  }
}

void FullyRefreshTheClock()
{

  clearDisplay(noBackgroundColourOverride);

  needToDrawTubes = true;
  lastSignificantTimeChange = -1;

  UpdateClock();
}

void showSettings()
{

  if ((showSettingsTriggeredFromAboutWindow) || IsTheTopButtonPressed())
  {
  }
  else
    return;

  clearDisplay(TFT_BLACK);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);

  sprite.setTextDatum(TR_DATUM);

  showSettingsTriggeredFromAboutWindow = false;

  if (debugIsOn)
    Serial.println("Displaying the Settings window");

  const int32_t numberOfSettingsShownOnSettingScreen = numberOfSavedSettingsInEEPROM + 2; // count includes the Exit selection; the actual Settings and the Setting options can be seen below
  const int32_t maxOptionsPerSetting = 10;                                                // each Setting has up to this many options

  static String selection[numberOfSettingsShownOnSettingScreen][maxOptionsPerSetting];
  static int selectectionsChosenOption[numberOfSettingsShownOnSettingScreen];

  const int32_t spacingBetweenLines = 15;

  static bool initialized = false;

  bool aButtonHasBeenPushed = true;

  int currentSettingIndex = 0;
  int currentOptionIndex = 0;

  if (!initialized)
  {

    // this only needs to be done once

    for (int i = 0; i < numberOfSettingsShownOnSettingScreen; i++)
      for (int j = 0; j < maxOptionsPerSetting; j++)
        selection[i][j] = "";

    for (int i = 0; i < numberOfSettingsShownOnSettingScreen; i++)
      selectectionsChosenOption[i] = 0;

    selection[0][0] = "Display the time and date";
    selection[0][1] = "Display the time only";
    selection[0][2] = "Display the date only";

    selection[1][0] = "Time displayed in 12 hour format";
    selection[1][1] = "Time displayed in 24 hour format";

    selection[2][0] = "Time displayed with seconds";
    selection[2][1] = "Time displayed without seconds";

    selection[3][0] = "Time displayed with AM/PM indicator";
    selection[3][1] = "Time displayed without AM/PM indicator";

    selection[4][0] = "Time displayed with 1/10th seconds";
    selection[4][1] = "Time displayed without 1/10th seconds";

    selection[5][0] = "Date format: YYYY-MM-DD";
    selection[5][1] = "Date format: YYYY/MM/DD";
    selection[5][2] = "Date format: YY-MM-DD";
    selection[5][3] = "Date format: YY/MM/DD";
    selection[5][4] = "Date format: MM-DD-YY";
    selection[5][5] = "Date format: MM/DD/YY";
    selection[5][6] = "Date format: MM-DD";
    selection[5][7] = "Date format: MM/DD";
    selection[5][8] = "Date format: DD-MM";
    selection[5][9] = "Date format: DD/MM";

    selection[6][0] = "Show a leading zero as a zero tube";
    selection[6][1] = "Show a leading zero as a blank tube";
    selection[6][2] = "Don't show a leading zero";

    selection[7][0] = "Show black background";
    selection[7][1] = "Show white background";
    selection[7][2] = "Show unique background colour every second";

    selection[8][0] = "Save these settings";
    selection[8][1] = "Discard changes";
    selection[8][2] = "Reset default settings";
    selection[8][3] = "Reset default settings and clear Wi-Fi credentials";

    selection[9][0] = "Exit";

    initialized = true;
  };

  LoadSettingsFromNonVolatileMemory();

  selectectionsChosenOption[0] = displayTimeAndDate;

  if (displayTimeIn12HourFormat)
    selectectionsChosenOption[1] = 0;
  else
    selectectionsChosenOption[1] = 1;

  if (displayTimeWithSeconds)
    selectectionsChosenOption[2] = 0;
  else
    selectectionsChosenOption[2] = 1;

  if (displayAMPM)
    selectectionsChosenOption[3] = 0;
  else
    selectectionsChosenOption[3] = 1;

  if (displayTimeWith10thsOfASecond)
    selectectionsChosenOption[4] = 0;
  else
    selectectionsChosenOption[4] = 1;

  selectectionsChosenOption[5] = dateFormat;

  selectectionsChosenOption[6] = showLeadingZeroTubeStyle;

  selectectionsChosenOption[7] = backgroundFormat;

  selectectionsChosenOption[8] = 0;

  // continue to loop here until the user chooses to exit the Settings windows

  bool showTheSettingsWindow = true;
  while (showTheSettingsWindow)
  {

    if (checkButton(TOP_BUTTON))
    {

      aButtonHasBeenPushed = true;

      if (currentSettingIndex == (numberOfSettingsShownOnSettingScreen - 1))
      {
        // User has choosen to return to the top setting
        currentSettingIndex = -1;
      }
      else
      {

        // User has choosen to advance to the next setting

        // special processing if user selects to display time without seconds
        if (currentSettingIndex == 2)
        {
          if (selectectionsChosenOption[2] == 1)
          {
            selectectionsChosenOption[4] = 1; // unset display time with 10ths of a second
          }
        };

        // special processing if user had selected to display time with the am/pm indicator
        if (currentSettingIndex == 3)
          if (selectectionsChosenOption[3] == 0)
            selectectionsChosenOption[4] = 1; // unset display time with 10ths of a second

        // special processing if user selects to display time with 10ths of a second
        if (currentSettingIndex == 4)
          if (selectectionsChosenOption[4] == 0)
          {
            selectectionsChosenOption[2] = 0; // set display option to display in seconds
            selectectionsChosenOption[3] = 1; // unset display am/pm
          };

        currentSettingIndex++;
            };
    };

    if (checkButton(BOTTOM_BUTTON))
    {

      if (currentSettingIndex == (numberOfSettingsShownOnSettingScreen - 1))
      {

        // User has chosen to exit

        if (selectectionsChosenOption[numberOfSettingsShownOnSettingScreen - 2] == 0)
        {

          // save settings

          displayTimeAndDate = selectectionsChosenOption[0];
          displayTimeIn12HourFormat = (selectectionsChosenOption[1] == 0);
          displayTimeWithSeconds = (selectectionsChosenOption[2] == 0);
          displayAMPM = (selectectionsChosenOption[3] == 0);
          displayTimeWith10thsOfASecond = (selectectionsChosenOption[4] == 0);
          dateFormat = selectectionsChosenOption[5];
          showLeadingZeroTubeStyle = selectectionsChosenOption[6];
          backgroundFormat = selectectionsChosenOption[7];

          StoreSettingsInNonVolatileMemory();
        };

        if (selectectionsChosenOption[numberOfSettingsShownOnSettingScreen - 2] == 2)
        {

          // retore default settings

          loadDefaultSettings();

          StoreSettingsInNonVolatileMemory();
        };

        if (selectectionsChosenOption[numberOfSettingsShownOnSettingScreen - 2] == 3)
        {

          // Restore default settings and clear Wi-Fi credentials

          initializeEEPROM();

          clearDisplay(TFT_BLACK);

          String message = "Settings restored";
          showPanel.drawString(message, 0, 16, 2);

          message = "Wi-Fi credentials cleared";
          showPanel.drawString(message, 0, 48, 2);

          message = "Automatically restarting in ten seonds ... ";
          showPanel.drawString(message, 0, 80, 2);

          delay(10000);
          ESP.restart();
        };

        // return without saving any changes
        showTheSettingsWindow = false;
      }
      else
      {

        // Change setting
        aButtonHasBeenPushed = true;

        currentOptionIndex = selectectionsChosenOption[currentSettingIndex] = selectectionsChosenOption[currentSettingIndex] + 1;

        if ((currentOptionIndex == maxOptionsPerSetting) || (selection[currentSettingIndex][currentOptionIndex] == ""))
          currentOptionIndex = 0;

        selectectionsChosenOption[currentSettingIndex] = currentOptionIndex;
      };
    };

    if (aButtonHasBeenPushed)
    {

      // update the display to reflect the change resulting from a button being pushed

      clearDisplay(TFT_BLACK);

      // align text to Top Left
      sprite.setTextDatum(TL_DATUM);

      sprite.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      sprite.drawString("Settings:", 0, 0, 2);

      for (int32_t i = 0; i < numberOfSettingsShownOnSettingScreen; i++)
      {

        if (i == currentSettingIndex)
        {

          sprite.setTextColor(TFT_WHITE, TFT_BLACK);
          sprite.drawString(String("*"), 0, (i + 1) * spacingBetweenLines, 2);
          sprite.drawString(selection[i][selectectionsChosenOption[i]], 15, (i + 1) * spacingBetweenLines, 2);
        }
        else
        {

          sprite.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
          sprite.drawString(selection[i][selectectionsChosenOption[i]], 15, (i + 1) * spacingBetweenLines, 2);
        }
      };

      if (currentSettingIndex == numberOfSettingsShownOnSettingScreen - 1)
      {

        sprite.setTextDatum(TR_DATUM);
        sprite.drawString("Return to top ->", TFT_Width, 11, 1);

        sprite.setTextDatum(BR_DATUM);
        String abbreviatedExitOption = "";
        switch (selectectionsChosenOption[numberOfSettingsShownOnSettingScreen - 2])
        {
        case 0:
        {
          sprite.setTextColor(TFT_GREEN, TFT_BLACK);
          abbreviatedExitOption = "Save and exit ->";
          break;
        }
        case 1:
        {
          sprite.setTextColor(TFT_YELLOW, TFT_BLACK);
          abbreviatedExitOption = "Discard changes and exit ->";
          break;
        }
        case 2:
        {
          sprite.setTextColor(TFT_YELLOW, TFT_BLACK);
          abbreviatedExitOption = "Reset default settings and exit ->";
          break;
        };
        case 3:
        {
          sprite.setTextColor(TFT_RED, TFT_BLACK);
          abbreviatedExitOption = "Full reset and restart ->";
          break;
        };
        };

        sprite.drawString(abbreviatedExitOption, TFT_Width, TFT_Height - 12, 1);
      }
      else
      {

        sprite.setTextDatum(TR_DATUM);
        sprite.drawString("Next ->", TFT_Width, 11, 1);

        sprite.setTextDatum(BR_DATUM);

        if (currentSettingIndex == numberOfSettingsShownOnSettingScreen - 2)
        {
          sprite.drawString("Change ->", TFT_Width, TFT_Height - 12, 1);
        }
        else
        {
          sprite.drawString("Change setting ->", TFT_Width, TFT_Height - 12, 1);
        };
      };

      drawDisplay();

      // prevents the screen from being updated again until the user presses another button
      aButtonHasBeenPushed = false;
    };
  };

  if (debugIsOn)
    Serial.println("Returning from the Settings window");

  LoadSettingsFromNonVolatileMemory();
  FullyRefreshTheClock();
};

void showAbout()
{

  if (!IsTheBottomButtonPressed())
    return;

  if (debugIsOn)
    Serial.println("Displaying the About window");

  // Display the About screen

  clearDisplay(TFT_BLACK);

  sprite.setTextColor(TFT_WHITE, TFT_BLACK);

  sprite.setTextDatum(TR_DATUM);
  sprite.drawString("Settings ->", TFT_Width, 11, 1);

  sprite.setTextDatum(TL_DATUM);
  sprite.drawString("Nixie Clock", 202, 38, 2);
  sprite.drawString("v3", 273, 44, 1);
  sprite.drawString("Copyright", 202, 54, 2);
  sprite.drawString("Rob Latour, 2025", 202, 70, 2);
  sprite.drawString("rlatour.com", 202, 86, 2);

  sprite.setTextDatum(BR_DATUM);
  sprite.drawString("Main screen ->", TFT_Width, TFT_Height - 12, 1);

  sprite.pushImage(0, 0, 192, 170, image_data_Rob);
  drawDisplay();

  int i = 0;
  const unsigned long secondsBetweenChanges = 5 * 1000;
  unsigned long nextChange = 0;

  if (debugIsOn)
    Serial.println("Returning from the About window");

  FullyRefreshTheClock();
}

void setup()
{

  SetupSerialMonitor();
  SetupButtons();
  SetupDisplay();
  SetupEEPROM();
  SetupTimeAndDate();
  FullyRefreshTheClock();
  setupComplete = true;
};

void loop()
{

  showAbout();
  showSettings();
  UpdateClock();
  RefreshTimeOnceADay();
}