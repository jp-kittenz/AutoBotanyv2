# Project Autobotany firmware

Source firmware for the ESP32-S3 N16R8 controller, plus a separate ESP32-CAM sketch and the integrated local dashboard. The program uses your supplied pins. The two additional MicroSD pins are proposed assignments because SD wiring was not supplied: MISO GPIO12 and CS GPIO16. SD shares SPI SCK GPIO13 and MOSI GPIO11 with the TFT.

The display is a 128x160 ST7735S SPI TFT. The code implements the 12 pages in your list, in that order.

## Package

| Location | Purpose |
| --- | --- |
| `ProjectAutobotany/ProjectAutobotany.ino` | Main ESP32-S3 sketch |
| `ProjectAutobotany/Config.h` | Pins, calibration, intervals, servo angles, output polarities, and AP credentials |
| `ProjectAutobotany/AutobotanyCore.h` | Sampling schedule, calibration, thresholds, and button debounce |
| `ProjectAutobotany/AutobotanyApp.h` | Sensors, RTC, SD logging, SoftAP, actuator control, and persistence |
| `ProjectAutobotany/TimedOutput.h` | Fan and pump timing using ESP timer callbacks |
| `ProjectAutobotany/ServoBlocker.h` | Brief MG90S drive with Arduino ESP32 2.x and 3.x LEDC paths |
| `ProjectAutobotany/AutobotanyDisplay.h` | TFT interface and button actions |
| `ProjectAutobotany/AutobotanyWeb.h` | Dashboard server and device APIs |
| `ProjectAutobotany/AutobotanyUI.h` | Dashboard stored in flash using PROGMEM |
| `ProjectAutobotany/index.html` | Editable dashboard source |
| `AutobotanyCamera/AutobotanyCamera.ino` | Camera capture, MJPEG video, AP connection, and discovery heartbeat |
| `WIRING.md` | Complete wiring table and power connections |
| `VALIDATION.md` | Checks performed and limits of verification |

There are no comments in the delivered code. Setup explanations are in these Markdown files. There are no precompiled `.bin` files: compile each sketch for its own board in ArduinoDroid.

## ArduinoDroid setup

The developer's current ArduinoDroid listing includes ESP32-S3 support. Your installed version must have an ESP32-S3 board profile and its SDK downloaded. Firmware cannot add a missing compiler or target to an older installation.

1. Extract this package. Keep `ProjectAutobotany` and `AutobotanyCamera` as separate sketch folders.
2. In ArduinoDroid, open `ProjectAutobotany/ProjectAutobotany.ino`. Keep every `.h` in that same folder.
3. Select your ESP32-S3 profile, typically **ESP32S3 Dev Module**, not the original ESP32 profile.
4. For the usual ESP32-S3-WROOM-1 N16R8 configuration, select **16 MB flash** and **OPI PSRAM** for the 8 MB octal PSRAM. Select the flash mode matching your actual module, commonly QIO. A module marked N16R8V or a different module can require different flash settings. Do not infer flash mode from PSRAM mode alone.
5. Use the available default partition scheme if the program fits. This program does not need all 16 MB of flash. A custom partition table is not required.
6. Select the USB upload method corresponding to your physical port. Use USB CDC on boot when using native USB for serial output, or the USB-to-UART port/profile where provided. 115200 upload speed is a conservative starting setting.
7. Install the libraries below using ArduinoDroid's library manager or library ZIP import. Install their dependencies too.
8. Configure calibration and power polarity in `Config.h`, then compile and upload.
9. Open the Serial Monitor at 115200 baud. The main board creates the local network and serves the dashboard at `http://192.168.4.1`.

### Main sketch libraries

| Library | Publisher | Use |
| --- | --- | --- |
| DHT sensor library | Adafruit | DHT22 temperature, humidity, and heat index |
| Adafruit Unified Sensor | Adafruit | DHT library dependency |
| RTClib | Adafruit | DS3231 RTC |
| Adafruit BusIO | Adafruit | RTC and display dependencies |
| Adafruit GFX Library | Adafruit | TFT drawing |
| Adafruit ST7735 and ST7789 Library | Adafruit | ST7735S-compatible display commands and initialization |

Install additional dependencies requested by the selected display library release, such as Adafruit seesaw. WiFi, Wire, SPI, SD, Preferences, WebServer, ESP timer, and camera support come from the Arduino ESP32 core. Do not install an unrelated SD library over the ESP32 core's SD implementation. No ESP32Servo, AsyncWebServer, ArduinoJson, or separate BH1750 library is required.

The code includes conditional LEDC and camera SCCB field names for Arduino ESP32 2.x and 3.x. It does not target Arduino ESP32 1.x.

### Camera sketch

Open `AutobotanyCamera/AutobotanyCamera.ino` separately and select **AI Thinker ESP32-CAM**, with PSRAM enabled. Flash it to the camera module using its ESP32-CAM-MB programmer, not to the S3 controller.

ESP32-CAM-MB is the USB programmer/baseboard. The camera sketch assumes the common AI Thinker style ESP32-CAM with an OV2640 sensor and its standard pin map. If the camera module is a different design, replace its camera pins and board target before uploading.

The camera needs only the ESP32 core's bundled libraries. It uses QVGA 320x240 JPEG images, with separate HTTP tasks for capture and video. With working PSRAM it uses two frame buffers; otherwise it falls back to one DRAM buffer.

## Local network and camera

| Setting | Value |
| --- | --- |
| SoftAP SSID | `Autobotany Group 1` |
| SoftAP password | `CSFJ-GRADE11` |
| Dashboard | `http://192.168.4.1` |
| Radio channel | 6 |
| Maximum associated clients | 6 |

The ESP32-CAM gets an address from the S3's DHCP server and registers with the controller every 15 seconds. The dashboard discovers its `/capture` and `:81/stream` addresses automatically. No fixed camera address is required. A camera with no heartbeat for 45 seconds is marked offline.

Connect your phone to the same SoftAP and choose to stay connected even if Android reports no internet. Open the local HTTP dashboard. Live video is the default camera mode; snapshots are selectable in Camera settings. Blank URL means automatic discovery. Use `/capture` for a finite JPEG image and `:81/stream` for MJPEG video.

The website is served by the S3. Camera image bytes travel from the camera to the browser through the local network; the main S3 does not copy or store each video frame in its application. The camera sends its discovery heartbeat to the S3. This leaves the S3 available for controls and sensors. Use one active video viewer at a time with this camera server. SD logging records sensor data, not video.

## Sampling and data freshness

| Component | Default acquisition behavior |
| --- | --- |
| Water level | Immediate boot sample, then 2 hours after each completed sample; manual reading from website or TFT |
| Soil moisture | Immediate boot sample, then 2 hours after each completed sample; manual reading from website or TFT |
| HM rain probe | Immediate boot sample, then 30 minutes after each completed sample; manual website reading |
| DHT22 | Every 2.5 seconds |
| BH1750 | Every 2.5 seconds after initialization |
| S3 internal temperature | Every 2.5 seconds |
| Website telemetry | Polls 3 seconds after each completed request |
| TFT | Refreshes its current page approximately once per second |
| CSV log | Every 60 seconds and after relevant events, with event writes spaced at least 5 seconds apart |

Each resistive probe is powered for a 1-second stabilization period, then averaged across 16 ADC samples spaced 20 ms apart. It is switched off afterward. A cycle is approximately 1.3 seconds plus normal loop and I/O latency. A manual reading resets that probe's next scheduled interval. Pressing its control again cancels an active measurement.

While the pump runs, the reservoir is sampled again approximately every 8 seconds. A reservoir sample older than 15 seconds is not accepted for continued pumping. This is additional to the normal two-hour schedule.

ACTIVE shows whether the probe is powered now, not whether its last reading is usable. It is normally NO between measurements. The page shows raw ADC values to help calibration.

For scheduled probes, UP-TO-DATE VALUES lasts 1 minute after acquisition. CURRENT VALUES covers the remainder of the schedule plus a 5-minute allowance. OUTDATED VALUES means that allowance has expired, the sample is invalid, or the dashboard has lost its connection. Continuous sensor data uses a 5-second fresh window and becomes outdated after 20 seconds.

Last-read times represent acquisition, not page refresh. A sensor cannot produce a newer value without another hardware reading. With the requested 30-minute rain interval, new rain may remain undetected for nearly 30 minutes. Shorten `RAIN_INTERVAL_MS` if you need faster rain-blocker response.

## TFT pages and buttons

Connect one side of each tactile button to its assigned GPIO and the other side to GND. All three use INPUT_PULLUP, with software debounce and one action per press. No external button resistors are needed.

NEXT and PREVIOUS wrap through these 12 pages. SELECT behaves as follows:

| Page | SELECT action |
| --- | --- |
| Reservoir | Start a brief sample, or cancel a sample already in progress |
| Soil Moisture | Start a brief sample, or cancel a sample already in progress |
| Is Raining | None |
| Temperature | None |
| Humidity | None |
| Sunlight | None |
| Cooling Fan | Toggle manual ON for up to 2 minutes or immediate OFF |
| Heat Index | None |
| Sun Blocker | Toggle covering command; drive briefly, then release PWM |
| Rain Blocker | Toggle covering command; drive briefly, then release PWM |
| Pump Status | Start a reservoir check and a manual run for up to 2 minutes, or stop immediately |
| Board Temperature | None |

The TFT displays values, units, condition labels, acquisition time, raw probe ADC, commanded blocker state, run time, time remaining, and short action messages. It uses a dark green layout with color-coded status labels.

`TFT_TAB = 2` selects the black-tab initialization. If your panel has shifted pixels or incorrect colors, test the panel's required tab initialization: 0 for green tab or 1 for red tab. `TFT_INVERT` can be changed for panels that require inversion. Keep portrait rotation 0 or 2 for this fixed 128x160 layout.

## Fan, pump, and blockers

The fan specification shown on the website is **40x40x10mm 4010 5V Brushless DC Fan**.

Automatic cooling starts at the saved fan ON chip temperature and stops at the lower OFF temperature. Initial examples are 55 C ON and 48 C OFF. They are editable commissioning values, not a thermal rating for your board. Chip temperature is distinct from DHT22 ambient air temperature.

Manual fan ON uses a two-minute timer. At expiry the output turns OFF and stays in manual mode until automatic mode is restored from the website. Manual OFF is immediate. Restoring automatic mode removes the manual timer and resumes temperature control. If chip readings become invalid during automatic operation, the fan retains its last commanded state.

Manual pump operation also has a two-minute timer. A calibrated reservoir reading is required before the pump can start. If that reading is older than 15 seconds, the firmware first takes a new reading and shows CHECKING. The pump stops if the reservoir becomes empty, below its saved minimum, or outdated. Fan and pump deadlines use ESP timer callbacks so a slow screen or SD operation does not require the main loop to notice the deadline first. They are software timers, not a physical safety relay.

Automatic watering starts disabled because an automatic watering duration and policy were not specified. The requested manual pump action works after reservoir calibration. To opt into the supplied example policy, set `AUTOMATIC_WATERING = true`. With soil and reservoir calibration enabled, it uses a five-second pulse when soil is below the minimum, requests another soil reading afterward, and allows at least 15 minutes before another automatic attempt. It avoids automatic watering when the latest calibrated rain reading reports rain. Adjust this policy to your plant and pump delivery rate before enabling it.

The Partial Sun Blocker follows the sunlight upper limit in automatic mode. It uncovers below 80 percent of that limit to reduce repeated movements. The Rain Blocker follows the latest calibrated rain sample. Each MG90S receives PWM for about 1.2 seconds after a move command, then PWM is detached. The servo has no holding PWM afterward; the linkage must mechanically remain in position.

Blocker positions are **commanded positions**. MG90S servos provide no external feedback to prove that a cover physically moved. The UI labels this explicitly. TFT or website movement commands select manual mode. Use the website's automatic-mode button to restore automation for that blocker.

On boot, the pump is OFF, the fan starts in automatic mode, blockers are commanded to the configured open positions, and all three probes take an initial sample. Fan and blocker modes are not persisted across reboot. Thresholds and plant stage are persisted.

## Calibration and mechanical setup

The raw calibration constants in `Config.h` are examples. They cannot be determined from a component name or pin map.

1. Open the website and take readings with the probes installed and powered as they will be used.
2. Record `SOIL_DRY_RAW` in your dry reference condition and `SOIL_WET_RAW` in your wet reference condition. Set `SOIL_CALIBRATED = true` after entering those measured values.
3. Record `WATER_EMPTY_RAW` and `WATER_FULL_RAW` at your intended reservoir reference levels. Set `WATER_CALIBRATED = true`. Until this flag is true, pump activation is disabled.
4. Record `RAIN_DRY_RAW` and `RAIN_WET_RAW`. Set `RAIN_CALIBRATED = true`. Tune `RAIN_TRIGGER_PERCENT` with your module and placement. Until calibrated, rain automation is inhibited and its reading is labeled for calibration.
5. Adjust `SUN_OPEN_DEGREES`, `SUN_COVER_DEGREES`, `RAIN_OPEN_DEGREES`, and `RAIN_COVER_DEGREES` to your linkages. Test movement before fastening a mechanism that could bind at the example angles.
6. Set sensor power, fan, and pump active polarity to match the actual driver modules.

Water and soil percentages are normalized sensor readings. They are not automatically tank-volume percentage or volumetric soil water content. Condition labels use your saved limits; they are not a plant-health diagnosis. A failed or miscalibrated probe can still produce a plausible ADC reading, so test the pump's stop behavior with the installed system.

## RTC and SD card

DS3231 and BH1750 share SDA41 and SCL42. The RTC is stored as UTC. The TFT displays Philippine time, and the browser displays its local timezone.

Open the dashboard and press **Set RTC from phone time** once. If the RTC reports lost power, firmware leaves timestamps uninitialized rather than inventing a date. Setting the clock also timestamps cached sensor acquisitions from their monotonic ages. Existing CSV rows are not rewritten. A working RTC battery preserves time when the main supply is off.

The SD card uses SPI and must have a format supported by the ESP32 core's SD library. Start with a FAT32 card. CSV files are written to `/autobotany/YYYY-MM-DD.csv`; before time is known, they go to `/autobotany/unsynced.csv`. The dashboard can download the latest log through `/logs/latest.csv`.

Each row contains UTC time, uptime, probe values, raw ADC readings, probe ages, calibration flags, ambient temperature, humidity, heat index, lux, chip temperature, fan state, pump state, and both commanded cover states. An SD failure does not halt sensor acquisition. The firmware retries mounting after a minute while the pump is not running.

## API

The dashboard and firmware are already matched. No API URLs need editing for this local setup.

| Method | Endpoint | Purpose |
| --- | --- | --- |
| GET | `/` | Dashboard from flash |
| GET | `/api/status` | Complete telemetry, schedule metadata, capabilities, and discovered camera URLs |
| POST | `/api/control` | Probe activation/cancel, timed fan/pump control, blocker commands, and automatic-mode restore |
| POST | `/api/thresholds` | Validate and persist all 12 numeric limits |
| POST | `/api/growth` | Save observed growth stage and plant age |
| POST | `/api/time` | Set the DS3231 from an epoch timestamp |
| POST | `/api/camera` | Camera address discovery and heartbeat |
| GET | `/logs/latest.csv` | Download latest CSV log |

POST bodies are URL-encoded and include `X-Autobotany-Request: 1`. The firmware does not enable cross-origin writes. The AP password controls access to this local network; the web UI does not have a separate user account.

## Editing the website

The served page is `AutobotanyUI.h`. `index.html` is its editable source. After changing the HTML, replace the content between the `R"ABHTML(` and `)ABHTML"` delimiters in the header with that updated HTML. Keep the raw-string delimiters themselves intact. Do not apply percent-token substitutions to this page.

## References

- [ArduinoDroid developer listing](https://play.google.com/store/apps/details?id=name.antonsmirnov.android.arduinodroid2)
- [Espressif Arduino ESP32 2.x to 3.x migration](https://docs.espressif.com/projects/arduino-esp32/en/latest/migration_guides/2.x_to_3.0.html)
- [Espressif LEDC API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html)
- [Espressif ESP32-S3 board and reserved-pin guidance](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.0.html)
- [Espressif internal temperature sensor](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/temp_sensor.html)
- [Espressif camera driver](https://github.com/espressif/esp32-camera)
- [Adafruit DHT sensor library](https://github.com/adafruit/DHT-sensor-library)
- [Adafruit RTClib](https://github.com/adafruit/RTClib)
- [Adafruit ST7735 library](https://github.com/adafruit/Adafruit-ST7735-Library)
