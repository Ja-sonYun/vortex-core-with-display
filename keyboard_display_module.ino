// Wifi setting
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// get current time
#include <time.h>

// display setting
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define JST 3600* 9

#define WORD_WIDTH 21
#define WORD_HEIGHT 4

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

// wed,12/32 00:00:00  |
//                     |
// lan: kr             |
//up:00:00:00 wifi:234 |

ESP8266WebServer server(80);

uint8_t minutes = 0;
uint8_t hours = 0;
uint8_t seconds = 0;

boolean isDisconnected = false;
boolean connectedWifi = false;
boolean isStationModeOn = false;

String screenBuffer[WORD_HEIGHT] = {"", "turning on wifi...", "", ""};

int timezone = 3;
int dst = 0;

unsigned long previousMillis = 0;
unsigned long previousMillis_wifi = 0;
const long interval = 1000;
const long interval_wifi = 5000;

String RSSI_temp = "??";

/* void putStringToBuffer(String str, int line) { */

void printBuffer() {
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  for(int i = 0; i < WORD_HEIGHT; i++) {
    display.println(screenBuffer[i]);
  }
  display.display();
}

void printString(String, boolean = true, boolean = true, int = 1);

void printString(String str, boolean newLine, boolean clearBeforeDisplay, int Size) {
  if(clearBeforeDisplay || newLine) {
    display.clearDisplay();
  }
  display.setTextSize(Size);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  if(newLine) {
    display.println(str);
  } else {
    display.print(str);
  }
  display.display();
}

void handleRoot() {
  server.send(200, "text/plain", "keyboard helper!");
}

void printServerArg() {
  if(server.uri().charAt(1) == '!') {
    screenBuffer[2] = server.uri().substring(2);
  } else  {
    screenBuffer[1] = server.uri().substring(1);
  }
}

void displayFirstSetup() {
  Wire.begin(2, 0);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    for(;;); // Don't proceed, loop forever
  }
  printBuffer();
}

void serverRouteSetup() {
  server.onNotFound(printServerArg);
  server.begin();

  delay(2000);
}

void webServerFirstSetup() {
  // Wait for connection
  screenBuffer[1] = WiFi.localIP().toString();
  printBuffer();
  delay(5000);

  if (MDNS.begin("esp8266")) {
    screenBuffer[2] = "MDNS res started";
  }
  printBuffer();
}

void configModeCallback (WiFiManager *wifiManager) {
  screenBuffer[2] = "ap on";
  printBuffer();
}

void timeGetterSetup() {
  configTime(9*3600, 0, "pool.ntp.org", "time.nist.gov");
  while (!time(nullptr)) {
    delay(1000);
  }
}

void getCurrentTimeStamp() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    char buffer[21];
    time_t now = time(nullptr);
    struct tm * timeinfo;
    time(&now);
    timeinfo = localtime(&now);
    strftime(buffer, 21, " %a,%b %d %H:%M:%S", timeinfo);
    screenBuffer[0] = String(buffer);
    printUpTime();
  }
}

// after printUpTime called.
void getWifiSignalStrength() {
  if(WiFi.status() != 3) {
    isDisconnected =  true;
    screenBuffer[1] = "Wifi disconnected..";
    screenBuffer[2] = "---------------------";
  }

  if(!isDisconnected) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis_wifi >= interval_wifi) {
      previousMillis_wifi = currentMillis;
      RSSI_temp = String(WiFi.RSSI());
    }
  
    screenBuffer[3] = String(screenBuffer[3] + " wifi:" + RSSI_temp);
  }
}

void setup() {
  isStationModeOn = true;
  displayFirstSetup();
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(60);
  if(wifiManager.autoConnect("Vortex Core Keyboard")) {
    webServerFirstSetup();
    serverRouteSetup();
    timeGetterSetup();
    connectedWifi = true;
  }
}

void loop() {
  if(connectedWifi) {
    getCurrentTimeStamp();
    getWifiSignalStrength();
    server.handleClient();
    MDNS.update();
    printBuffer();
  } else {
    if(isStationModeOn) {
      isStationModeOn = false;
      WiFi.mode(WIFI_OFF);
    }
    display.clearDisplay();
    display.setTextSize(2);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.setCursor(0,0);             // Start at top-left corner
    display.println("==UPTIME==");
    printUpTime();
    display.println(String(" " + screenBuffer[3].substring(3)));
    display.display();
    delay(1000);
  }
}

void printUpTime() {
  if(seconds > 58) {
    minutes++;
    seconds = 0;
  } else {
    seconds++;
  }
  if(minutes > 58) {
    hours++;
    minutes = 0;
  }
  String h = hours < 10 ? String('0') + String(hours) : String(hours);
  String m = minutes < 10 ? String('0') + String(minutes) : String(minutes);
  String s = seconds < 10 ? String('0') + String(seconds) : String(seconds);
  screenBuffer[3] = String("up:" + h + ":" + m + ":" + s);
}
