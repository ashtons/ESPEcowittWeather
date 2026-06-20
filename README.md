# ESPEcowittWeather

An ESP32 Arduino sketch that fetches live temperature readings from an Ecowitt weather station and displays them on a TFT screen.

The display alternates between indoor and outdoor temperature readings, refreshing weather data from the Ecowitt API every 30 seconds.

## Features

- Connects to Wi-Fi from local credentials
- Calls the Ecowitt real-time device API over HTTPS
- Displays indoor and outdoor temperatures in Celsius
- Alternates the display every 5 seconds
- Shows simple status messages while connecting, updating, or handling errors

## Hardware

- ESP32-compatible board
- TFT display supported by the `TFT_eSPI` library
- Ecowitt weather station with API access enabled

The sketch currently enables the TFT backlight on GPIO 4:

```cpp
const int TFT_BACKLIGHT_PIN = 4;
```

Change this value in `ESPEcowittWeather.ino` if your display uses a different backlight pin.

## Arduino Libraries

Install these libraries in the Arduino IDE Library Manager:

- `TFT_eSPI`
- `ArduinoJson`

The sketch also uses ESP32 core libraries:

- `WiFi`
- `HTTPClient`
- `WiFiClientSecure`

## Setup

1. Copy `sample_secrets.h` to `secrets.h`.

   ```bash
   cp sample_secrets.h secrets.h
   ```

2. Edit `secrets.h` with your Wi-Fi and Ecowitt details:

   ```cpp
   const char *WIFI_SSID = "YOUR_WIFI_SSID";
   const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

   const char *APPLICATION_KEY = "YOUR_ECOWITT_APPLICATION_KEY";
   const char *API_KEY = "YOUR_ECOWITT_API_KEY";
   const char *MAC_ADDRESS = "YOUR_ECOWITT_DEVICE_MAC";
   ```

3. Configure `TFT_eSPI` for your display.

   `TFT_eSPI` requires board and display settings in its own configuration files. Follow the `TFT_eSPI` setup instructions for your specific screen and ESP32 board.

4. Open `ESPEcowittWeather.ino` in the Arduino IDE.

5. Select your ESP32 board and port.

6. Upload the sketch.

## Notes

- `secrets.h` contains private credentials and should not be committed to git.
- `sample_secrets.h` is safe to commit and shows the required values.
- HTTPS certificate validation is disabled with `client.setInsecure()` to keep the device setup simple for this read-only API call.
- Temperature values are requested in Celsius using `temp_unitid=1`.

## License

This project is licensed under the MIT License. See `LICENSE` for details.
