# Camera connection repair 3.0.2

## Upload only the camera

1. Extract this archive to a fresh location.
2. Open AutobotanyCamera/AutobotanyCamera.ino in ArduinoDroid.
3. Select AI Thinker ESP32-CAM, with PSRAM enabled. Upload to the ESP32-CAM using its CAM-MB baseboard. This sketch does not target the ESP32-S3.
4. After uploading, remove any manual GPIO0-to-GND programming jumper. Reset or power-cycle the camera so it runs the application.
5. Power the main S3 first, then power the camera from its normal external 5V USB supply through the CAM-MB. Your phone can act only as the WiFi browser during this test.
6. Join Autobotany Group 1 on your phone, stay connected when Android says no internet, and open http://192.168.4.1. Allow about 20 seconds for initial registration.

Only the camera source changed. The main S3 bundle in this archive is byte-for-byte the previously delivered 3.0.1 source. Its calibration, TFT, RTC, BH1750, dashboard, pin assignments, and saved settings are retained. Re-uploading the S3 is unnecessary for this repair.

The camera uses the same SSID and password as before: Autobotany Group 1 and CSFJ-GRADE11. It obtains its normal address through DHCP and registers with the S3 at http://192.168.4.1/api/camera.

## Diagnose without Serial

If initial WiFi connection or dashboard registration has not succeeded after 45 seconds, the camera starts an additional password-protected access point:

- Network: Autobotany Camera Help
- Password: CSFJ-GRADE11
- Help page: http://192.168.5.1

Join that network on your phone, select stay connected without internet, and open the address above manually. The help network stays available until the camera restarts. The camera continues trying to join the main S3 network while the help network is active. Shared-radio channel changes during connection attempts can briefly interrupt your phone's help connection; reconnect and refresh if needed.

The page displays the camera firmware version, uptime, main-network connection, local address, signal strength, last WiFi disconnect reason, main S3 registration result, camera initialization error, PSRAM presence, and reset cause. Diagnostic details contains the complete JSON report. Send that report or a photo of the help page if the main dashboard still cannot reach the camera.

- Registration 200 means the S3 accepted the camera heartbeat. Check the age to see how recently it succeeded.
- Registration 0 means no registration request has completed yet.
- A negative registration result means the HTTP connection or response failed.
- Registration 403 means the S3 rejected the request.
- Registration 404 means the requested registration endpoint is absent from the running main firmware.
- Camera ready false with error 0 can simply mean the camera is waiting to start.
- Brownout reset true describes the last restart. A very short uptime repeatedly returning to zero indicates repeated resets, even when a complete serial log is unavailable.

Retry connection schedules an immediate station connection attempt when disconnected, or an immediate registration request when already connected. Use Fast mode applies and saves QVGA, retries sensor initialization if necessary, and cancels pending restoration of a larger profile. Open photo requests a finite JPEG from the camera directly.

Once registration succeeds, return your phone to Autobotany Group 1 and open the main dashboard. The normal camera address remains its DHCP address in 192.168.4.x, not the help address.

The help network also needs the board to remain powered. Firmware cannot keep WiFi running through a supply brownout. This build keeps brownout detection enabled.

## Changes

The initial WiFi attempt now gets a 30-second window before any explicit disconnect and retry. One application retry schedule controls reconnection; overlapping core automatic reconnection is disabled. Credentials are not rewritten to flash on each attempt.

Camera startup uses Fast mode and QVGA-sized JPEG buffers. It no longer preallocates SVGA buffers for Fast. With PSRAM it retains two buffers; without PSRAM it uses one DRAM buffer. Moving to a larger profile reallocates the driver buffers under the existing camera mutex, with rollback if allocation or sensor configuration fails. Lower profiles reuse the available capacity.

A saved Balanced or Detail profile is restored once after 30 seconds of stable main-network connection and acknowledged registration. Following a brownout reset, automatic larger-profile restoration is skipped for that boot; the saved preference is not erased. The dashboard profile buttons still work. This keeps Fast available for recovery without forcing a saved high-resolution profile during startup.

Registration connect and read timeouts are 1500 ms and 2000 ms, respectively. Registration continues every five seconds. The camera still uses protocol 3 and the same endpoints expected by the S3 3.0.1 bundle. A failed video-server start retains the health/help server and retries video startup.

The external antenna selection remains controlled by the existing board hardware. No transmit-power increase or antenna GPIO change was introduced.

## Verification and limits

Local C++11 tests passed through both ESP32 core 2.x and 3.x source branches using simulated Arduino and ESP32 interfaces. Twenty scenario runs covered slow 28-second association, unavailable main AP, late main startup, rejected registration, HTTP setup failure, video-server failure, saved-profile restoration, brownout recovery, buffer resizing and rollback, sensor retry, profile validation, missing PSRAM, lost connection, and returned frame buffers. The help-page JavaScript was checked for status rendering, recovery requests, JSON details, and offline behavior.

These are host simulations, not an actual Xtensa or ArduinoDroid build. They do not measure radio performance or validate the electrical supply. The reported camera failure cannot be confirmed fixed on hardware until this sketch is uploaded and tested with sufficient external power.
