#pragma once
#include <stdint.h>

namespace AB {
constexpr char AP_NAME[] = "Autobotany Group 1";
constexpr char AP_PASSWORD[] = "CSFJ-GRADE11";
constexpr int DHT_PIN = 4;
constexpr int WATER_ADC = 1;
constexpr int WATER_ENABLE = 21;
constexpr int SOIL_ADC = 2;
constexpr int SOIL_ENABLE = 5;
constexpr int RAIN_ADC = 6;
constexpr int RAIN_ENABLE = 14;
constexpr int SUN_SERVO = 18;
constexpr int RAIN_SERVO = 17;
constexpr int FAN_PIN = 7;
constexpr int PUMP_PIN = 15;
constexpr int TFT_SCK = 13;
constexpr int TFT_MOSI = 11;
constexpr int TFT_RST = 10;
constexpr int TFT_DC = 9;
constexpr int TFT_CS = 8;
constexpr int SD_MISO = 12;
constexpr int SD_CS = 16;
constexpr int I2C_SDA = 41;
constexpr int I2C_SCL = 42;
constexpr int SELECT_PIN = 40;
constexpr int NEXT_PIN = 38;
constexpr int PREVIOUS_PIN = 39;
constexpr bool SENSOR_ACTIVE_HIGH = true;
constexpr bool FAN_ACTIVE_HIGH = false; // Fan relay on COM/NO: LOW=ON, HIGH=OFF.
constexpr bool PUMP_ACTIVE_HIGH = false; // Pump relay on COM/NO: LOW=ON, HIGH=OFF.
constexpr int SOIL_DRY_RAW = 3000;
constexpr int SOIL_WET_RAW = 1200;
constexpr int WATER_EMPTY_RAW = 200;
constexpr int WATER_FULL_RAW = 2800;
constexpr int RAIN_DRY_RAW = 3500;
constexpr int RAIN_WET_RAW = 1200;
constexpr bool SOIL_CALIBRATED = false;
constexpr bool WATER_CALIBRATED = false;
constexpr bool RAIN_CALIBRATED = false;
constexpr float RAIN_TRIGGER_PERCENT = 35.0f;
constexpr uint64_t PROBE_INTERVAL_MS = 7200000ULL;
constexpr uint64_t RAIN_INTERVAL_MS = 1800000ULL;
constexpr uint64_t PROBE_WARMUP_MS = 1000;
constexpr uint64_t ADC_SPACING_MS = 20;
constexpr uint8_t ADC_SAMPLES = 16;
constexpr uint64_t MANUAL_RUN_MS = 120000;
constexpr uint64_t SERVO_DRIVE_MS = 1200;
constexpr int SUN_OPEN_DEGREES = 15;
constexpr int SUN_COVER_DEGREES = 100;
constexpr int RAIN_OPEN_DEGREES = 15;
constexpr int RAIN_COVER_DEGREES = 105;
constexpr int SERVO_MIN_US = 600;
constexpr int SERVO_MAX_US = 2400;
constexpr uint8_t BH1750_ADDRESS = 0x23;
constexpr uint8_t BH1750_MTREG = 32;
constexpr uint64_t ENVIRONMENT_INTERVAL_MS = 2500;
constexpr uint64_t SD_LOG_INTERVAL_MS = 60000;
constexpr bool AUTOMATIC_WATERING = false;
constexpr uint64_t AUTO_WATER_PULSE_MS = 5000;
constexpr uint64_t AUTO_WATER_COOLDOWN_MS = 900000;
constexpr uint32_t TFT_SPI_HZ = 16000000;
constexpr uint64_t TFT_POLL_MS = 200;
constexpr uint32_t TFT_FRAME_MS = 40; // Animation target: 25 Hz, cooperative loop.
constexpr uint32_t TFT_INTRO_MS = 4000;
constexpr uint32_t TFT_FADE_MS = 480;
constexpr uint32_t TFT_SLEEP_MS = 30000; // Display idle only; controller stays awake.
constexpr char TFT_CREDIT[] = "Developed by CSFJ";
constexpr uint8_t TFT_TAB = 2;
constexpr uint8_t TFT_ROTATION = 0;
constexpr bool TFT_INVERT = false;
constexpr int WIFI_CHANNEL = 1;
constexpr int MAX_CLIENTS = 6;
}
