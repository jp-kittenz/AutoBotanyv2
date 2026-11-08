# AutoBotany 3.6.0

## What's new in 3.6.0

- **TFT card 14, “Your Plant”.** The left side shows an animated sprite of the selected preset (34 plant designs). It changes with the saved growth stage and grows with the recorded age. Beside it, a vertical growth-stage rail runs from seed (bottom) to mature (top), with dots only for the stages that plant uses. The right side lists the plant, stage, age, pot, medium, exposure, planting event and its soil/lux/air/humidity targets. SELECT only plays an animation.
- **Smoother SELECT on action cards.** Reservoir and soil sampling show a probe dipping in. The fan and pump impellers spin up and coast down. The sun cover slides and the rain umbrella opens and closes, so the change reads smoothly. Every card's artwork also fades and rises in when you change page.
- **Rain clouds return** on the sunlight card for sun-rain and moon-rain, and on the sun- and rain-blocker cards whenever it rains.
- **Teach without the camera.** You can analyse a photo file from your phone or computer, teach labelled batches of photos, or re-import an exported examples file. See *Teach the small classifier*.
- **Website logo** now has a transparent background. Changed website files moved to `/assets/v3.6.0/`, so phones do not keep a cached old copy.

34 Philippine plant profiles, a guided website setup for small to medium pots, and a local teachable vision classifier. This package also includes the complete TFT animation gallery: 88 card states, 45 SELECT reactions, all 13 full cards, intro, sleep and wake.

## Install

1. Extract into a fresh folder. Open `AutobotanyS3V350/AutobotanyS3V350.ino` in ArduinoDroid. Keep every supplied header and **all 20 .cpp files** beside it. Compile the complete folder, not just the main tab.
2. Keep the ESP32-S3 board/flash/PSRAM/USB settings that work with your hardware. Compile and upload. If old files appear in errors, clear the build cache and check that the extracted folder is selected.
3. Connect to the Wi-Fi name on TFT card 13, then open its IP in your browser. The supplied defaults are `Autobotany Group 1`, password `CSFJ-GRADE11`, IP `192.168.4.1`. The ESP32-CAM firmware is unchanged and does not need reflashing for this update.

If ArduinoDroid overwrites the app partition with its default table, the existing recovery remains available: write `Recovery/autobotany-s3-partitions-3MiB.bin` at **0x8000**, with whole-flash erase disabled, then reset. `partitions.csv` is also included for builders that honor it. A later ArduinoDroid upload can overwrite the table again.

## Choose a plant on the website

Open **Plant setup**:

1. Choose a plant by local/scientific name. Mark **New plant** when replacing the plant; this starts a new record and clears its teaching examples.
2. Select its observed stage, recent planting event, pot volume, medium, exposure and known age/start date. Unknown age is allowed. “Just transplanted” is an event; it does not make an older plant zero days old.
3. Review the proposed soil, brightness, temperature and humidity limits before applying. Smaller-than-planned pots show a repotting acknowledgement. The device checks the proposal again when saving.

The pot input range is **1–20 litres**. Compact crops and leafy harvests are the intended use. Ampalaya, malunggay and calamansi have nursery-only guidance within that range; larger mature plants need more space. The exact cultivar, drainage, root depth and number of plants matter.

Thresholds are **adjustable engineering starting targets**, not experimentally validated crop prescriptions. Soil percentages are relative to your probe's calibration in that medium; lux is a brightness measurement, not PAR. Calibrate with the actual medium and adjust from observations. Medium and pot size are recorded and guide setup; they do not invent a universal moisture conversion.

Applying a profile preserves reservoir, heat-index and chip-cooling limits. It changes the protected manual pump request to **5 seconds**. Measure your pump's actual delivery before using that pulse on a small pot. Automatic watering remains **disabled in the supplied configuration**; selecting a plant does not enable it. Existing reservoir checks, timed shutoff and startup protection remain active.

See `PLANT-PRESETS.md` for all 34 profiles, base limits and stage adjustments.

## Teach the small classifier

The model starts **untrained**. It can learn normal appearance, yellowing, browning and drooping from your labelled snapshots, with a separate output for visual growth stage. It will not know all 34 plants on first boot.

1. Select a plant profile, connect the camera and frame one plant with little soil/background.
2. Click **Analyse snapshot**. The browser crops/resizes the camera image; the S3 extracts features and runs the model locally.
3. Open **Teach this snapshot**, label what you actually see, and save. Each output needs at least **three distinct snapshots per class in at least two classes** before it can predict. Do not stress a plant to obtain examples; leave an output untrained until genuine examples are available.
4. Judge it on fresh images that were not used for teaching. Distant/unsupported inputs return unknown. The displayed neighbour vote is similarity support, not an accuracy percentage or diagnosis.

**No camera, or want to teach faster?** Open **Teach without the camera**:

- **Analyse a photo…** (next to Analyse snapshot) analyses an image file instead of a camera snapshot. Its result can then be taught with *Teach this snapshot* like a camera frame.
- **Labelled photos**: pick a condition and/or stage, select several photos showing exactly that, and click *Teach selected photos*. Unclear, dark, glare-filled or duplicate photos are skipped, and the reason is listed.
- **Examples file**: re-import a `.json` saved with *Export examples*, for example after clearing, or to share examples between devices growing the same plant. Examples taught for a different preset are refused.

The website cannot download plant pictures from the internet for you. The S3's network is a local SoftAP with no internet connection. Stock photos also differ from your camera's distance, angle, background and light, which is what the small model actually learns, so they would make predictions less reliable. Your own photos work best.

There are 64 saved feature examples per plant record. Examples survive restart. Changing species/new plant clears the active model; changing crop requires clearing its examples. Export is available before clearing. No photographs are stored with the examples.

Manual analysis works while the website is open. The optional **every 60 seconds** setting runs only while the page is visible; it pauses during plant setup and while the teaching panel is open. Analysis still needs the browser to capture and resize images, whether from the camera or a photo file. It does not analyse images on its own while the website is closed.

Recorded age comes from your supplied age/date and RTC. Unknown age remains unknown. The visual stage suggestion never silently changes the saved stage, thresholds or actuators. See `VISION.md` for the learning workflow, limitations and technical details.

## Review every TFT animation

Open **`AnimationPreview/Animation-gallery.html`** in a browser. It is self-contained and works offline; it is a review file, not a page served by the ESP32.

- Start with Intro & full cards, then select a card or All 88 states.
- Compare normal motion with its SELECT reaction. Pause to inspect a frame.
- Mark states for rework, add notes and export them as JSON.
- Individual GIFs, `All-states.png`, `Overview.png` and the state manifest are included in the same folder.

The images are produced by the delivered C++ renderer with simulated readings. They preview the animation logic and pixels; physical TFT colour and frame rate still require a device check. The seven cosmetic SELECT actions are on rain, temperature, humidity, sunlight, heat index, chip temperature and network. Other cards retain their sampling/hardware actions. See `TFT-ANIMATIONS.md` for triggers and interactions.

Card 14, **Your Plant**, is described in `TFT-ANIMATIONS.md`; `AnimationPreview/v3.6.0/` has its sprite sheets, growth timelapses and the new transition GIFs.

Card 13 shows AP status, IP, Wi-Fi name, password, user-device count and camera status. Its two animations distinguish a running/offline AP, fresh camera heartbeat, and 0/1/2/3/4+ client illustrations. Clients are associated devices, not unique people. A fresh camera is excluded from USERS but included in ALL. The globe does not imply an Internet connection.

Dates use **YYYY-MM-DD**. The TFT and plant start-date calculation use Philippine time; the existing website clock display uses the viewing browser's timezone. RTC storage remains UTC. After 30 seconds without button input, the display sleeps with floating Zs; the first button gesture wakes only.

## Edit and rebuild

**GSAP 3.15.0** (core) is bundled for website animation work: `Website/assets/vendor/gsap-3.15.0/gsap.min.js`, served by the S3 at `/assets/vendor/gsap-3.15.0/gsap.min.js` (gzipped, about 28 KB of flash). It is served from the device because the SoftAP has no internet, so a CDN link would not load. No page loads it yet. To use it on a page, add `<script src="/assets/vendor/gsap-3.15.0/gsap.min.js"></script>` before your own script, then run `python3 rebuild-website.py`. GSAP is free under its standard no-charge license (https://gsap.com/standard-license). To add a plugin such as ScrollTrigger, copy its `.min.js` beside it and add an entry to `rebuild-website.py`.

Website sources are under `Website/`. Run `python3 rebuild-website.py` after editing them, then recompile/upload the complete sketch. Run `python3 rebuild-tft-assets.py` with Pillow installed to regenerate logo data. Both scripts preserve controller/configuration files.

Plant defaults live in `AutobotanyPlants.cpp`; model features/rules in `AutobotanyVision.cpp`; plant/vision routes in `AutobotanyPlantWeb.h`; TFT scenes in the `AutobotanyTFT*.cpp` modules. The pre-existing wiring is documented in `WIRING.md`.

## Verification status

See `VALIDATION.md` for the completed host simulations and limits. All firmware source files are compiled independently as C++11 with Arduino's `PI` macro present, and the controller is exercised against Arduino 2/3 API stubs. This catches the previous `constexpr float PI` collision and many integration errors.

**No Xtensa/ArduinoDroid target build, physical hardware test or live browser/camera test was available in this environment.** The website was syntax-checked and its event/API flows exercised in a Node DOM simulation. Firmware flash size, real TFT timing, camera/browser CORS and pump delivery need verification on your hardware. Plant recognition accuracy has not been measured on a real plant dataset.
