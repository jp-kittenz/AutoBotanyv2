# Wiring for Project Autobotany

GPIO numbers below are ESP32-S3 GPIO numbers, not header position numbers. Keep grounds common between the controller, sensors, camera supply, motor drivers, and servo supply.

| Component signal | ESP32-S3 connection | Configuration |
| --- | --- | --- |
| DHT22 DATA | GPIO4 | DHT22 |
| Water level AO | GPIO1 | ADC1 input |
| Water sensor power enable | GPIO21 | Active HIGH by default |
| Soil moisture AO | GPIO2 | ADC1 input |
| Soil sensor power enable | GPIO5 | Active HIGH by default |
| HM rain sensor AO | GPIO6 | ADC1 input; use AO, not DO |
| Rain sensor power enable | GPIO14 | Active HIGH by default |
| Sun blocker MG90S signal | GPIO18 | 50 Hz PWM during movement |
| Rain blocker MG90S signal | GPIO17 | 50 Hz PWM during movement |
| Cooling fan driver input | GPIO7 | Active HIGH by default |
| Water pump driver input | GPIO15 | Active HIGH by default |
| TFT SCL / SCK | GPIO13 | SPI clock |
| TFT SDA / MOSI | GPIO11 | SPI data to display |
| TFT RST | GPIO10 | Display reset |
| TFT DC | GPIO9 | Display command/data |
| TFT CS | GPIO8 | Display chip select |
| TFT BLK | 3V3 | Backlight always on, as supplied in your map |
| DS3231 SDA | GPIO41 | Shared I2C SDA |
| DS3231 SCL | GPIO42 | Shared I2C SCL |
| BH1750 SDA | GPIO41 | Shared I2C SDA |
| BH1750 SCL | GPIO42 | Shared I2C SCL |
| SELECT button | GPIO40 to GND when pressed | INPUT_PULLUP |
| NEXT button | GPIO38 to GND when pressed | INPUT_PULLUP |
| PREVIOUS button | GPIO39 to GND when pressed | INPUT_PULLUP |
| MicroSD SCK | GPIO13 | Shared TFT SPI clock |
| MicroSD MOSI / DI | GPIO11 | Shared TFT SPI MOSI |
| MicroSD MISO / DO | GPIO12 | Proposed additional assignment |
| MicroSD CS | GPIO16 | Proposed additional assignment |

The display's pins marked SDA and SCL are SPI signals in this wiring. They are separate from the DS3231/BH1750 I2C bus on GPIO41/42. TFT CS and SD CS must remain separate. The libraries use SPI transactions to share the bus.

GPIO35, GPIO36, and GPIO37 are not used by this firmware because they can be reserved for the octal memory interface on relevant ESP32-S3 modules. Native USB pins GPIO19 and GPIO20 are also left unused by the main controller's peripherals. Confirm your board's exact header labels before connecting it.

## Power and signal levels

| Connection | Required arrangement |
| --- | --- |
| Three tactile buttons | GPIO to GND; internal pull-ups are enabled, with no external button resistors |
| DHT22 | Use 3.3V-compatible logic. A bare DHT22 needs its specified data pull-up; a module with an onboard pull-up avoids adding a separate resistor |
| Probe activation pins | Connect to appropriate sensor power-switch or enable circuits. The code treats GPIO21/5/14 as control signals. High-side switching keeps the sensor ground reference connected |
| Water, soil, and rain AO | Keep the voltage presented to each ESP32 ADC input within 3.3V logic limits. A 5V-powered module may need output level adaptation |
| Fan and pump | Use properly rated MOSFET or relay drivers, a suitable 5V supply, and the suppression required by the actual motor/load. GPIO7 and GPIO15 do not supply motor power |
| MG90S servos | Supply from a suitable servo power rail, with common ground. GPIO17/18 provide signal only |
| RTC and light sensor | Use 3.3V-compatible I2C levels. Check module pull-ups; they must not pull ESP32 GPIOs to 5V |
| MicroSD adapter | Follow that module's supply requirements and ensure its MISO output is 3.3V-compatible |
| Camera | Power its camera board through its supported supply input. Do not power it from an S3 GPIO |

Choose a supply for the combined startup and stall current of the pump, fan, and both servos. The board's 3.3V regulator is not the motor supply. The requested INPUT_PULLUP arrangement applies to the buttons; it does not replace sensor pull-ups, power drivers, or signal-level requirements elsewhere.

A two-wire fan and an ordinary switched pump provide no rotation or flow feedback. Their website states and run times report commanded outputs. The two MG90S states likewise report commanded cover positions.

## Camera module pin map used by the camera sketch

This map belongs to the assumed AI Thinker ESP32-CAM module, not to the S3 N16R8:

| Camera signal | ESP32-CAM GPIO |
| --- | --- |
| PWDN | 32 |
| RESET | Not connected |
| XCLK | 0 |
| SCCB SDA | 26 |
| SCCB SCL | 27 |
| D0 through D7 | 5, 18, 19, 21, 36, 39, 34, 35 |
| VSYNC | 25 |
| HREF | 23 |
| PCLK | 22 |

The ESP32-CAM-MB programmer handles USB upload for the camera board. Upload the camera sketch with its own board target, then let it join the S3 SoftAP over WiFi. No UART or I2C connection between the two boards is required.
