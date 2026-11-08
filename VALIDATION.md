# Validation — AutoBotany 3.6.0

## 3.6.0 checks

These checks were run in a Linux container. They use host builds of the shipped sources against stand-in Arduino/ESP32/library headers, a host re-implementation of the Adafruit GFX drawing primitives, and Chromium through Playwright. No Xtensa/ArduinoDroid compiler or physical hardware was available.

- The complete sketch (`.ino` plus all 20 `.cpp` files) compiles and links as one host program with `-std=gnu++11`. The sketch and display/TFT sources also pass `gnu++17` and `gnu++2b` syntax checks. The new and changed TFT, display and plant-web code produces no `-Wall -Wextra` warnings.
- Host simulation runs the real `setup()`/`loop()` through startup, intro and white fade, then presses NEXT/PREVIOUS/SELECT through all 14 cards. It saves card 14 profiles through the real `/api/plants/apply` handler. Captured framebuffers cover card 14 with no profile and with six presets, the card 14 entrance and SELECT reaction, fan on/off, sun cover, umbrella, and sun-rain/moon-rain. The largest single TFT transfer measured was 6,032 pixels (the card 14 pane), within the 6,144-pixel canvas.
- All 34 presets were rendered at every allowed stage (sprite sheets in `AnimationPreview/v3.6.0/`) and inspected visually for clipping against the rail, pane and info column.
- Browser end-to-end: the shipped dashboard was served by the host-compiled firmware routes. The test walked the plant setup wizard, taught 3 + 3 labelled photo files, confirmed a duplicate photo is refused with its reason, analysed two held-out photos without a camera (both matched their taught class), and exported, cleared and re-imported the examples (6 → 0 → 6). The only console error was the expected 409 for the duplicate. The photos were synthetic drawings, so this checks the workflow, not real recognition accuracy.
- `rebuild-website.py` regenerates the embedded assets. Unchanged assets are byte-identical to 3.5.0. The new logo was checked composited on light and dark backgrounds for leftover white fringes.

Still required on hardware: target compile and flash size in ArduinoDroid, real TFT frame rate with card 14's procedural drawing, SPI timing, and phone-browser photo handling (large photos, HEIC support depends on the phone's browser).

## 3.5.0 checks

The release is checked with desktop C++11 builds and simulated Arduino hardware APIs. The tests exercise the delivered controllers, API handlers, model and drawing code. They do not substitute for an Xtensa/ArduinoDroid build or hardware commissioning.

## Checks

- All **19 C++ source files** compile independently and link with the main sketch. Arduino's predefined `PI` macro is present; the drawing code uses nonconflicting constants.
- Main controller simulations use Arduino major-version 2 and 3 APIs. Startup tests cover normal startup, held buttons, configuration failure, AP retry, missing I²C devices, timer failure, SD/runtime timeouts, warm panic, brownout, staggered servo startup and cold reset for both APIs.
- Plant tests cover all 34 profiles across stages, media and exposures; species-stage restrictions; retained hardware limits; unknown age, RTC and uptime fallback; legacy settings migration; failed NVS writes; preview/review/revision checks; repot acknowledgement; new-plant model isolation; five-second protected manual pumping.
- Vision tests cover RGB565 payload length/content, colour masks, dark/bright/empty quality gates, untrained output, synthetic nearest-neighbour classification, distant-input rejection, frame identity, repeat/duplicate examples, failed saves, crop mismatch, persistence, reset and JSON export. Synthetic tests verify algorithm behaviour, not real recognition accuracy.
- Website tests execute the shipped garden JavaScript in a Node event/DOM model: 34 catalogue choices, species stage filtering, the three-step wizard, invalid inputs, failed save/retry, concurrent settings changes, unknown/date age, repot acknowledgement, exact-frame labelling, untrained/stale/offline handling, camera failures, capture pause/resume and visible-page observation timing. RTC format tests include zero-padding and a local-midnight boundary.
- TFT tests cover 13-page navigation, animated intro and exact credit timing, white transition, 30-second idle, all wake-only keys, release/debounce guards, allocation fallback, bounded transfers and background controller work. No unintended hardware/network changes occur from the seven cosmetic SELECT actions.
- Threshold/categorical input partitions reach exactly **88 state keys** included in the gallery. All **45 eligible SELECT variants** visibly change pixels and return to the same phase as the normal scene. Tests also cover clipping across 640 art/sky/dome combinations at 53 times.
- Network checks cover camera freshness/timeout, total/user counts, stopped AP, credentials/IP text and dates. The catalogue covers the supplied maximum of six AP stations.
- Website assets are served byte-for-byte (including gzip decoding). The original supplied web images, camera sketch and partition-recovery binary are preserved. Website and logo generation scripts reproduce the packaged sources.
- The offline gallery's manifest, GIF dimensions/durations and JavaScript syntax are checked. The compact overview and selected renderer frames were visually inspected. ZIP integrity and package contents are compared exactly.

## Limits

No physical ESP32-S3, ESP32-CAM or TFT was available, and no Xtensa/ArduinoDroid target compilation was performed. Firmware flash/RAM usage on target, TFT frame timing/colour, browser layout, camera capture/CORS on the actual phone, electrical startup behaviour and pump delivery still require hardware validation. A browser UI session was not available; website tests are simulated event/API tests.

The TFT target interval is 40 ms. The reusable 128×48 RGB565 canvas occupies 12,288 bytes; the measured simulated maximum buffered transfer is 5,632 pixels per tick. These are implementation properties, not a measured 25-fps hardware guarantee.

The classifier ships without examples and has no measured accuracy on real plant images. Thresholds/pot hints are initial commissioning settings, not crop-trial results. Automatic watering remains disabled in the supplied configuration. User labels and visual predictions do not actuate hardware.
