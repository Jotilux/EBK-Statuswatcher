#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#define LED D4  // D4 GPIO2 onBoardLED
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "OLEDDisplayUi.h"
// Include custom images
#include "images.h"
#include "config.h"

const char* host = "eigenbaukombinat.de";
const int httpsPort = 443;
const char* fingerprint = "EC:E2:82:C6:50:9D:B1:99:CE:33:0D:B4:5B:BC:02:AB:D7:A1:0C:62"; // SHA1 fingerprint of the certificate
String status;
String Termin;
String TerminMorgen;
String LastModdate;
String LastModjson;
int count;
int countVer;
String url= "/status/status.json";
String url2 = "/unsere-Veranstaltungen/";

SSD1306  display(0x3c, D3, D5); //i2c addr, SDA, SCL
OLEDDisplayUi ui     ( &display );

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(128, 0, String(status));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  //  display->drawString(0, 0, String(count) + " " + String(countVer));  // DEBUG Recheck counter for debug
  display->drawString(0, 0, String(LastModjson.substring(1, 12)));
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x + 32, y + 0, EBK_Logo_width, EBK_Logo_height, EBK_Logo_bits);    //EBK_Logo
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Demonstrates the 3 included default sizes. The fonts come from SSD1306Fonts.h file
  // Besides the default fonts there will be a program to convert TrueType fonts into this format
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(2 + x, 8 + y, "JOTI's EBK\nSpacestatus\nwatcher");

}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Text alignment demo
  display->setFont(ArialMT_Plain_24);
  // The coordinates define the center of the text
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 8 + y, "Space is\n" + String(status));
}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawStringMaxWidth(0 + x, 8 + y, 128, "Host: \n" + String(host) + ":" + String(httpsPort) + " \n" + String(LastModjson));
}

void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawStringMaxWidth(0 + x, 10 + y, 128,  "SHA1 fingerprint:\n" + String(fingerprint));
}

void drawFrame6(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawStringMaxWidth(0 + x, 8 + y, 128,  "HEUTE:");
  display->drawStringMaxWidth(0 + x, 17 + y, 128,  String(Termin));
}

void drawFrame7(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawStringMaxWidth(0 + x, 8 + y, 128,  "MORGEN:");
  display->drawStringMaxWidth(0 + x, 17 + y, 128,  String(TerminMorgen));
}

void drawFrame8(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x, y, T21_Logo_width, T21_Logo_height, T21_Logo_bits);    //T21_Logo
}

void drawFrame9(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x + 34, y + 11, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);    //Wifi_Logo
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 46 + y, "ssid: " + String(ssid));
}
// This array keeps function pointers to all frames, frames are the single views that slide in
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5, drawFrame6, drawFrame7, drawFrame8, drawFrame9 };

// how many frames are there?
int frameCount = 9;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;


void setup() {
  count = 0;
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  Serial.begin(115200);
  
// The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(30);
  // Customize the active and inactive symbol
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);
  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);
  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);
  // Add frames
  ui.setFrames(frames, frameCount);
  // Add overlays
  ui.setOverlays(overlays, overlaysCount);
  // Initialising the UI will init the display too.
  ui.init();
  display.flipScreenVertically();

  Serial.println();
  Serial.print("##### \n connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED, LOW);
    delay(500);
    Serial.print(".");
    digitalWrite(LED, HIGH);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();  
  
} 

void loop() {
  ui.update();
  WiFiClientSecure client;
  if (count == 0){
   
  status = "CHECKING";
  Serial.print("##### \n connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    status = "connection failed";
    ui.update();
    digitalWrite(LED, LOW);
    delay(1000);
    digitalWrite(LED, HIGH);
    return;
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
    status = "certificate doesn't match";
    ui.update();
    digitalWrite(LED, LOW);
    delay(1000);
    digitalWrite(LED, HIGH);
    return;
  }
  
  count = 3000; // reset counter if connection is ok
  Serial.print("##### \n requesting URL: ");
  Serial.println(url);
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "User-Agent: BuildFailureDetectorESP8266\r\n" + "Connection: close\r\n\r\n");
  Serial.println("##### \n request sent");
  Serial.println("##### \n  header " + url);

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    Serial.println(line); // print full header json
    if (line.indexOf("Last-Modified")>=0) {   // Searchstring exists?
      String stundeSubString = line.substring(32,34);
      int stunde = atoi(stundeSubString.c_str());
      if (stunde==23) stunde=0; else stunde++;      
      if (stunde==0) Serial.print(" + 1 Tag"); else Serial.println();  // Tagsprung
      LastModjson = line.substring(19, 31) + " " + String(stunde) + line.substring(34, 40);
      //Serial.println(LastModjson); // DEBUG
    }
   if (line == "\r") {
      Serial.println("headers received");
      break;
    }}

// start waiting for the response   
    unsigned long lasttime = millis();
    while (!client.available() && millis() - lasttime < 1000) {delay(1);}  // wait max 1s for data
    String line = client.readStringUntil('\n');
  if (line.indexOf("{\"open\": true") >= 0){
    //Serial.println(line);
    Serial.println(">> Space open");
    status = "OPEN";
    digitalWrite(LED, LOW);
  } else if (line.indexOf("{\"open\": false") >= 0) {
    Serial.println(">> Space closed");
    status = "CLOSED";
    digitalWrite(LED, LOW);
    delay(1);
    digitalWrite(LED, HIGH);
  } else {
    Serial.println(">> Space undefind");
    status = "UNDEFIND";
    digitalWrite(LED, LOW);
    delay(1000);
    digitalWrite(LED, HIGH);
    return;
    }
  Serial.println("closing connection");
  } else {
   count--;  //count down to zero
   }
  
  if (countVer == 0) {
    Serial.print("##### \n connecting to ");
    Serial.println(host);
   if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    status = "connection failed";
    ui.update();
    digitalWrite(LED, LOW);
    delay(5000);
    digitalWrite(LED, HIGH);
    return;
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
    status = "certificate doesn't match";
    ui.update();
    digitalWrite(LED, LOW);
    delay(1000);
    digitalWrite(LED, HIGH);
    return;
  }
  countVer = 20000;  // reset counter if connection is ok
  Serial.print("##### \n requesting URL: ");
  Serial.println(url2);
  client.print(String("GET ") + url2 + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "User-Agent: BuildFailureDetectorESP8266\r\n" + "Connection: close\r\n\r\n");
  Serial.println(" ##### \n request sent");
  Serial.println("##### \n  header " + url2);
  
  String Tag;
  String TagMorgen;
  String Monat;
  int MonatNr = 0;
  Termin = "";
  TerminMorgen = "";
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);  // print full header Veranstaltungen
     if (line.indexOf("Date")>=0) {   // Searchstring exists?
      //line = "Date: Wed, 05 Feb 2018 17:50:49 GMT"; // DEBUG Fakedate
      String stundeSubString = line.substring(24,25);
      int stunde = atoi(stundeSubString.c_str());
      if (stunde==23) stunde=0; else stunde++;      
      if (stunde==0) Serial.print(" + 1 Tag"); else Serial.println();  // Tagsprung
      LastModdate = line.substring(10, 22) + " " + String(stunde) + line.substring(26, 35);
      if (line.substring(11,12) == "0"){
       Tag = line.substring(12, 13);
       } else {
       Tag = line.substring(11, 13);
        };
      Monat = line.substring(14, 17);
      //Serial.println("Tag: " + Tag); //DEBUG
      //Serial.println("Monat: " + Monat);  //DEBUG
      if (Monat == "Jan") {
        MonatNr = 1;
      } else if (Monat == "Feb"){
        MonatNr = 2;
      } else if (Monat == "Mar"){
        MonatNr = 3;
      } else if (Monat == "Apr"){
        MonatNr = 4;
      } else if (Monat == "May"){
        MonatNr = 5;
      } else if (Monat == "Jun"){
        MonatNr = 6;
      } else if (Monat == "Jul"){
        MonatNr = 7;         
      } else if (Monat == "Aug"){
        MonatNr = 8;
      } else if (Monat == "Sep"){
        MonatNr = 9;
      } else if (Monat == "Okt"){
        MonatNr = 10;        
      } else if (Monat == "Nov"){
        MonatNr =11;
      } else if (Monat == "Dez"){
        MonatNr = 12;
      }
      //Serial.println(MonatNr);  //DEBUG gesamte Suchstring
      //Serial.println("LastModdate: " + LastModdate);  //DEBUG gesamte Suchstring
    }
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
   
}

  while (client.available()) {
    
    String line = client.readStringUntil('\n');
    //Serial.println(line);   //DEBUG gesamte Webseite
    if (line.indexOf("</a><br />") >= 0) { // Searchstring exists?
       if (line.indexOf("Impressum") >= 0) {
        break; } else {
      //Serial.println(line);  //DEBUG gesamte Suchstring
      int vonPos = line.indexOf(",");
      int bisPos = line.indexOf("\" href");
      int midPos = line.indexOf("<a title=");
      String Datum = line.substring(vonPos +1, midPos);
      String DatumHeute = Tag + ". " + MonatNr + ". ";
      int TagNr;
      TagNr = Tag.toInt();    // Tag String -> int
      TagNr = TagNr + 1;      // nächster Tag
      TagMorgen = String(TagNr); // Tag int -> String
      String DatumMorgen = TagMorgen + ". " + MonatNr + ". ";
      Datum = Datum.substring(1, midPos -2);
      //Serial.print(Datum);  //DEBUG gesamte Suchstring
      //Serial.println();     //DEBUG gesamte Suchstring
      //Serial.print(DatumHeute); //DEBUG gesamte Suchstring
      //Serial.println();         //DEBUG gesamte Suchstring
      if (Datum == DatumHeute){
        Serial.println(line.substring(vonPos +1, midPos) + "Thema " + line.substring(midPos +10, bisPos));  
        Termin = Termin + line.substring(midPos +10, bisPos) + ", ";
        //String Thema = line.substring(midPos +10, bisPos);
      } 
      if (Datum == DatumMorgen){
        Serial.println(line.substring(vonPos +1, midPos) + "Thema " + line.substring(midPos +10, bisPos)); 
        TerminMorgen = TerminMorgen + line.substring(midPos +10, bisPos) + ", ";
      }
   
    }}}
  Serial.println("closing connection");
  } else {
    countVer--;
    }
 
   int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
 delay(remainingTimeBudget); 
}}
 
 
