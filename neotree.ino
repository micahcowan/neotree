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
bool touched = false;

#include "wifipass.h"
#include "neotree_effects.h"    // defines "Effect", "cur_effect"
#include "neotree_server.h"     // defines "server"

void setup() {
    Serial.begin(115200);
    pixels.begin();
    pixels.setBrightness(120);

    serverSetup();
}

void loop() {
    server.handleClient();

    ard_time_t t = millis();
    //pixels.rainbow(65535. * atime / TOTALTIME);
    for (int i=0; i != NUM; ++i) {
        if (!touched)
            pixels.setPixelColor(i, rainbot(t, i));
        else
            pixels.setPixelColor(i, redgreen(t, i));
    }
    pixels.show();

    delay(DELAYVAL);
}
