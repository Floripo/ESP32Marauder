#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

class WebUI {
public:
    WebUI();

    void start();

    bool running() const;

private:
    AsyncWebServer server;
    bool active;

    void setupRoutes();

    static void handleRoot(AsyncWebServerRequest *request);
    static void handleStatus(AsyncWebServerRequest *request);
    static void handleWifiScan(AsyncWebServerRequest *request);
    static void handleReboot(AsyncWebServerRequest *request);
};

extern WebUI webui_obj;
