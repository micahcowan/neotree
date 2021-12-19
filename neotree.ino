#include <math.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include "wifipass.h"

WebServer server(80);

#define PIN 15
//#define NUM 200
#define NUM 50
#define DELAYVAL 20 // 50 fps
#define TOTALTIME 3000  // 3 secs

typedef uint32_t  color_t;
typedef unsigned long ard_time_t;
bool touched = false;
int threshold = 10;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM, PIN, NEO_RGB | NEO_KHZ800);

color_t rainbot(ard_time_t msecs, int i) {
  double phase = 1. * i / NUM;
  phase += 1. * (msecs % TOTALTIME) / TOTALTIME;
  phase = fmod(phase, 1.0);
  return pixels.ColorHSV(phase * 65536);
}

color_t redgreen(ard_time_t msecs, int i) {
  msecs %= 500;
  return msecs < 250 ? pixels.Color(0, 0, 255) : pixels.Color(0, 255, 0);
}

void fail(void) {
  pixels.fill(pixels.Color(255,0,0));
  pixels.show();
  for (;;) { // ever
    delay(5000);
  }
}

void handleRoot() {
  touched = !touched;
  server.send(200, "text/plain", String("Red/green is ") + (touched? "on" : "off"));
}

void setup() {
  pixels.begin();
  pixels.setBrightness(120);

  // flash red/green while waiting for Wifi to start
  WiFi.mode(WIFI_STA);
  WiFi.begin(MY_SSID, MY_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    ard_time_t t = millis();
    for (int i=0; i != NUM; ++i) {
      pixels.setPixelColor(i, redgreen(t, i));
    }
    pixels.show();
    delay(DELAYVAL);
  }

  if (! MDNS.begin("tree")) {
    fail();
  }

  server.on("/", handleRoot);
  server.begin();

  // half sec of blue when connected
  pixels.fill(pixels.Color(0, 0, 255));
  pixels.show();
  delay(500);
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
