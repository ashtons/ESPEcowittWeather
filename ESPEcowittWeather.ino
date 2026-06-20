#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "secrets.h"

const unsigned long REQUEST_INTERVAL_MS = 30000;
const unsigned long DISPLAY_INTERVAL_MS = 5000;
const int TFT_BACKLIGHT_PIN = 4;

TFT_eSPI tft = TFT_eSPI();

unsigned long lastRequestAt = 0;
unsigned long lastDisplayToggleAt = 0;
bool hasReading = false;
bool showIndoor = true;
String lastStatus = "Starting";
float indoorTemperatureC = 0.0;
float outdoorTemperatureC = 0.0;

void drawStatus(const String &status)
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.drawString(status, tft.width() / 2, tft.height() / 2);
}

void drawTemperature()
{
    if (!hasReading)
    {
        drawStatus(lastStatus);
        return;
    }

    const float temperature = showIndoor ? indoorTemperatureC : outdoorTemperatureC;
    const String location = showIndoor ? "Indoor" : "Outdoor";

    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);

    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextFont(7);
    tft.drawString(String(temperature, 1) + " C", tft.width() / 2, (tft.height() / 2) - 24);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(4);
    tft.drawString(location, tft.width() / 2, (tft.height() / 2) + 48);
}

String buildWeatherUrl()
{
    String url = "https://api.ecowitt.net/api/v3/device/real_time";
    url += "?application_key=";
    url += APPLICATION_KEY;
    url += "&api_key=";
    url += API_KEY;
    url += "&mac=";
    url += MAC_ADDRESS;
    url += "&call_back=all&temp_unitid=1&rainfall_unitid=12&wind_speed_unitid=7&pressure_unitid=3";

    return url;
}

void connectToWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to WiFi SSID: ");
    Serial.println(WIFI_SSID);
    drawStatus("Connecting WiFi");

    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        attempt++;
        Serial.print("WiFi connection attempt ");
        Serial.println(attempt);
    }

    lastStatus = "WiFi connected";
    Serial.print("WiFi connected. IP address: ");
    Serial.println(WiFi.localIP());
    drawStatus(lastStatus);
}


void fetchAndDisplayWeather()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi disconnected before request");
        lastStatus = "WiFi reconnecting";
        drawStatus(lastStatus);
        connectToWiFi();
    }

    WiFiClientSecure client;
    // Avoid storing a root certificate on the device for this small read-only API call.
    client.setInsecure();

    HTTPClient http;
    const String url = buildWeatherUrl();
    http.setTimeout(10000);

    Serial.println("Requesting Ecowitt weather data");
    // Serial.println(url);
    drawStatus("Updating");

    if (!http.begin(client, url))
    {
        lastStatus = "HTTP setup failed";
        Serial.println(lastStatus);
        drawStatus(lastStatus);
        return;
    }

    const int statusCode = http.GET();
    if (statusCode != HTTP_CODE_OK)
    {
        lastStatus = "HTTP error " + String(statusCode);
        Serial.println(lastStatus);
        drawStatus(lastStatus);
        http.end();
        return;
    }

    StaticJsonDocument<512> filter;
    filter["code"] = true;
    filter["msg"] = true;
    filter["data"]["indoor"]["temperature"]["value"] = true;
    filter["data"]["outdoor"]["temperature"]["value"] = true;

    if (filter.overflowed())
    {
        lastStatus = "JSON filter too small";
        Serial.println(lastStatus);
        drawStatus(lastStatus);
        http.end();
        return;
    }

    const String responseBody = http.getString();
    Serial.println("Ecowitt raw response:");
    // Serial.println(responseBody);

    StaticJsonDocument<768> doc;
    DeserializationError error = deserializeJson(
        doc,
        responseBody,
        DeserializationOption::Filter(filter));

    http.end();

    if (error)
    {
        lastStatus = "JSON parse failed";
        Serial.print(lastStatus);
        Serial.print(": ");
        Serial.println(error.c_str());
        drawStatus(lastStatus);
        return;
    }

    if (doc.overflowed())
    {
        lastStatus = "JSON doc too small";
        Serial.println(lastStatus);
        drawStatus(lastStatus);
        return;
    }

    JsonVariant apiCodeVariant = doc["code"];
    if (apiCodeVariant.isNull())
    {
        lastStatus = "Missing API code";
        Serial.println(lastStatus);
        drawStatus(lastStatus);
        return;
    }

    // API may serialize code as either 0 or "0"
    const int apiCode = apiCodeVariant.as<int>();
    if (apiCode != 0)
    {
        const char *apiMessage = doc["msg"] | "Unknown API error";
        lastStatus = "API error " + String(apiCode);
        Serial.print(lastStatus);
        Serial.print(": ");
        Serial.println(apiMessage);
        drawStatus(lastStatus);
        return;
    }

    const char *indoorValue = doc["data"]["indoor"]["temperature"]["value"];
    const char *outdoorValue = doc["data"]["outdoor"]["temperature"]["value"];
    if (indoorValue == nullptr || outdoorValue == nullptr)
    {
        lastStatus = "Missing temp data";
        Serial.println(lastStatus);
        drawStatus(lastStatus);
        return;
    }

    indoorTemperatureC = String(indoorValue).toFloat();
    outdoorTemperatureC = String(outdoorValue).toFloat();
    hasReading = true;
    lastStatus = "Updated";

    Serial.print("Indoor temperature C: ");
    Serial.println(indoorTemperatureC, 1);
    Serial.print("Outdoor temperature C: ");
    Serial.println(outdoorTemperatureC, 1);

    lastDisplayToggleAt = millis();
    drawTemperature();
}

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println("ESPEcowittWeather starting");

    if (TFT_BACKLIGHT_PIN >= 0) {
      pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
      digitalWrite(TFT_BACKLIGHT_PIN, HIGH);
      Serial.print("LCD backlight enabled on pin ");
      Serial.println(TFT_BACKLIGHT_PIN);
    }

    tft.init();
    tft.setRotation(1);
    Serial.print("LCD initialized. Width: ");
    Serial.print(tft.width());
    Serial.print(", height: ");
    Serial.println(tft.height());
    drawStatus("Starting");

    connectToWiFi();
    fetchAndDisplayWeather();
    lastRequestAt = millis();
}

void loop()
{
    unsigned long now = millis();
    if (hasReading && now - lastDisplayToggleAt >= DISPLAY_INTERVAL_MS)
    {
        lastDisplayToggleAt = now;
        showIndoor = !showIndoor;
        drawTemperature();
    }

    if (now - lastRequestAt >= REQUEST_INTERVAL_MS)
    {
        lastRequestAt = now;
        Serial.println("Request interval elapsed");
        fetchAndDisplayWeather();
    }
}
