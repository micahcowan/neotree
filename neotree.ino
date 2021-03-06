#include <math.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#define PIN 15
//#define NUM 200
#define NUM 50
#define DELAYVAL 20 // 50 fps
#define TOTALTIME 3000  // 3 secs

typedef uint32_t  color_t;
typedef unsigned long ard_time_t;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM, PIN, NEO_RGB | NEO_KHZ800);

ard_time_t  lastChangeTime = 0;
#define CHANGE_TIME 600000  // 10 mins

#include "wifipass.h"
#include "neotree_effects.h"    // defines "Effect", "cur_effect"
#include "neotree_server.h"     // defines "server"

void setup() {
    Serial.begin(115200);
    pixels.begin();
    pixels.setBrightness(80);

    serverSetup();
}

void loop() {
    ard_time_t t = millis();
    server.handleClient();

    if (t - lastChangeTime > CHANGE_TIME) {
        lastChangeTime = t;
        ++currentEffect;
    }

    doPixels(*currentEffect);
    ard_time_t u = millis();

    ard_time_t d = (u > t + DELAYVAL)? 1u : DELAYVAL - (u - t);
    delay(d);
}
