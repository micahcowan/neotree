// neotree_server.h
//
// Manage web server running on ESP32

WebServer server(80);

void fail(void) {
    pixels.fill(pixels.Color(255,0,0));
    pixels.show();
    for (;;) { // ever
        delay(5000);
    }
}

void handleRoot() {
    ++currentEffect;
    server.send(200, "text/plain", "Yup, okay.");
}

void serverSetup() {
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
