# Validation and remaining hardware work

This package contains source code. A real ESP32 board compiler, ArduinoDroid installation, physical boards, and browser rendering environment were unavailable during creation. No board `.bin` files are included, and no claim of a completed device upload is made.

## Checks completed

- Compiled the main sketch and its headers with a host C++11 compiler against mocked Arduino, ESP32, sensor, display, filesystem, and network interfaces.
- Compiled both the Arduino ESP32 2.x and 3.x conditional code paths for the main sketch.
- Compiled both conditional camera configuration paths with mocked ESP32 camera and HTTP interfaces.
- Exercised initial probe acquisition, stabilization, averaging, power-off, two-hour sample validity, and schedules beyond 32-bit millisecond rollover.
- Exercised timer callback expiry without calling the main loop, manual fan control, automatic-mode restoration, pump calibration gating, pending reservoir checks, low-reservoir stopping, and required reservoir sampling during pump operation.
- Exercised button debounce, 12-page wraparound, no SELECT action on the rain page, fan selection, blocker selection, and brief servo PWM release.
- Exercised web command headers, rejected commands, numeric threshold validation, stage persistence paths, camera heartbeat registration, and serialization of generated JSON.
- Parsed generated API JSON and checked scheduled-probe metadata, SoftAP status, camera addresses, and commanded-position labels.
- Checked CSV rows and calibration metadata for a consistent column count using a mocked SD filesystem.
- Exercised the dashboard script with a mocked DOM and device API, including freshness, sleeping sensors, calibration labels, connection loss, controls, unsaved form preservation, RTC requests, camera discovery, video, snapshots, pause, and camera-offline handling.
- Checked HTML tag balance, unique IDs, label references, matching embedded HTML, pin assignment uniqueness, and the absence of code comments, em dashes, and emoji in the deliverable.

Mocks check application logic and C++ structure. They do not prove binary compatibility with the exact libraries installed on your phone or electrical compatibility with a particular module.

## Before normal operation

Compile each sketch for its actual board in ArduinoDroid. Install the specified libraries and configure flash/PSRAM settings. Confirm the two proposed MicroSD pins, calibrate probes, set output polarities, and test the servo travel with your linkage. Confirm the DS3231 clock, SD writes, camera capture and video, timer expiry, button actions, and the installed pump's low-reservoir stop behavior.

The fan and pump report commanded outputs. Cover statuses are commanded positions. There are no specified tachometer, flow, or end-position feedback inputs. The firmware cannot confirm physical fan rotation, water delivery, or cover arrival without additional feedback hardware.
