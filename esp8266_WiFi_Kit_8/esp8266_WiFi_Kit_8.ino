

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "SSD1306Wire.h"
#include <ArduinoJson.h>
#include "pitches.h"

SSD1306Wire display(0x3c, 4, 5, GEOMETRY_128_32);

#include "defs.h"


const char* ssid     = STASSID;
const char* password = STAPSK;

int buttonState = 0;
int buttonMillis = 0;
int buttonCount = 0;

// notes in the melody - popcorn by hot butter:
int melody[] = {
  NOTE_B6, NOTE_A6, NOTE_B6, NOTE_FS6, NOTE_D6, NOTE_FS6, NOTE_B5, 
  NOTE_B6, NOTE_A6, NOTE_B6, NOTE_FS6, NOTE_D6, NOTE_FS6, NOTE_B5, 
  NOTE_B6, NOTE_CS6, NOTE_D7, NOTE_CS6, NOTE_D7, NOTE_B6, NOTE_CS6, NOTE_B6, NOTE_CS6, NOTE_A6, NOTE_B6, NOTE_A6, NOTE_B6, NOTE_G6, NOTE_B6
};

int noteDurations[] = {
  8, 8, 8, 8, 8, 8, 4, 
  8, 8, 8, 8, 8, 8, 4, 
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 4
};


void ICACHE_RAM_ATTR pinWatch() {
  if (digitalRead(D3) != buttonState) {
  buttonState = digitalRead(D3);
  if (millis() > buttonMillis + 60) {
  buttonMillis = millis();
    if (buttonState==LOW) {
      buttonCount++;
      Serial.print("Button pressed, count: ");
      Serial.println(buttonCount);
    }
  }
  }
}

void setup() {
  
  Serial.begin(115200);

  StaticJsonDocument<200> doc;

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  //set up button - d3 is also gpio0
  pinMode(D3, INPUT);
  attachInterrupt(digitalPinToInterrupt(D3),pinWatch,CHANGE);

  //set up LED
  pinMode(D6, OUTPUT);
  digitalWrite(D6, LOW);

  //Manual reset of the OLED display
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW);
  delay(50);
  digitalWrite(16, HIGH);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);


  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.clear();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.drawString(64, 8, "connecting...");
    display.display();
  }

  Serial.println("");
  Serial.println("WiFi connected");
  display.clear();
  display.drawString(64, 8, "connected!");
  display.display();

  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  buttonState = digitalRead(D3);

//            WiFiClient client;
            std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
            client->setFingerprint(fingerprint);

            HTTPClient http;

            Serial.print("[HTTP] begin...\n");
            // configure traged server and url
            http.begin(*client, GET_ENDPOINT); //HTTP
            http.addHeader("Content-Type", "application/json");
            int httpCode = http.GET();

            // httpCode will be negative on error
            if (httpCode > 0) {
              // HTTP header has been send and Server response header has been handled
              Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        
              // file found at server
              if (httpCode == HTTP_CODE_OK) {
                const String& payload = http.getString();
                Serial.println("received payload:\n<<");

                DeserializationError error = deserializeJson(doc, payload);

                if (error) {
                  Serial.print(F("deserializeJson() failed: "));
                  Serial.println(error.c_str());
                } else {
                  String machine = doc["machine"];
                  int count = doc["count"];
                  Serial.print("Popcorn Machine: ");
                  Serial.println(machine);
                  Serial.print("Existing Count: ");
                  Serial.println(count);
  
                  buttonCount = count;
                }                
              }
            } else {
                  Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
                  display.clear();
                  display.setFont(ArialMT_Plain_24);
                  display.drawString(64, 8, "Fail :(");
                  display.display();
                  delay(500);
                  return;
            }

            http.end();

}

void playtune() {
      // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 31; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    digitalWrite(D6, HIGH);
    tone(D7, melody[thisNote], noteDuration);

    delay(noteDuration);
    digitalWrite(D6, LOW);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * .35;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(D7);
  }
}

void pushcount() {
              display.clear();
            display.setFont(ArialMT_Plain_24);
            display.drawString(64, 8, "Requesting..");
            display.display();
          
            // Use WiFiClient class to create TCP connections
            std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
            client->setFingerprint(fingerprint);
            HTTPClient http;


            Serial.print("[HTTP] begin...\n");
            // configure traged server and url
            http.begin(*client, POST_ENDPOINT); //HTTP
            http.addHeader("Content-Type", "application/json");
            int httpCode = http.POST("{\"machine\":\""+String(MACHINE_ID)+"\",\"count\":\""+String(buttonCount)+"\",\"auth\":\""+String(API_TOKEN)+"\"}");

            // httpCode will be negative on error
            if (httpCode > 0) {
              // HTTP header has been send and Server response header has been handled
              Serial.printf("[HTTP] POST... code: %d\n", httpCode);
        
              // file found at server
              if (httpCode == HTTP_CODE_OK) {
                const String& payload = http.getString();
                Serial.println("received payload:\n<<");
                Serial.println(payload);
                Serial.println(">>");
              }
            } else {
                  Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
                  display.clear();
                  display.setFont(ArialMT_Plain_24);
                  display.drawString(64, 8, "Fail :(");
                  display.display();
                  delay(500);
                  return;
            }

            http.end();
  

            display.clear();
            display.setFont(ArialMT_Plain_24);
            display.drawString(64, 8, "POPCORN!");
            display.display();
}

void loop() {

      // check if the pushbutton is pressed.
      // if it is, the buttonState is LOW:
      if (buttonState == LOW) {
            //post
            int pushedCount = buttonCount;
            pushcount();
            playtune();
            // if the animals kept pushing during the song, let's push the update to the server
            if (buttonCount > pushedCount) { pushcount(); }
      } else {
            display.clear();
            display.setFont(ArialMT_Plain_16);
            display.drawString(64, 8, "POPCORN! ("+String(buttonCount)+")");
            display.display();

      }
     
}
