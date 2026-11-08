# Local plant vision — v3.6.0

This is a small, teachable nearest-neighbour classifier, with feature extraction and inference on the ESP32-S3. There are no pretrained disease, species or age weights. A selected preset supplies the plant's identity and the permitted teaching stages; the camera is not asked to identify 34 species.

## What it reports

| Output | Meaning |
|---|---|
| Colour observations | Fraction of crop pixels matching green, yellow or brown rules |
| Condition | Similarity to your labelled normal-appearance / yellowing / browning / drooping examples |
| Visual stage | Similarity to your separately labelled sprout, seedling, vegetative, flowering, fruiting or mature examples, where allowed by the profile |
| Recorded age | Age/date entered in setup plus elapsed RTC time; never inferred from an image |
| Unknown | No supported match, an untrained output, a stale observation or an unsuitable image |

These are coarse appearance cues. Brown soil, variegation, shadows and warm light can change the features. A colour change or droop does not establish disease, nutrient deficiency or water stress. A small static crop cannot reliably count leaves, resolve tiny flowers, or distinguish every developmental stage. The interface keeps the saved stage under your control.

## Capture and learning

The existing ESP32-CAM provides a CORS-enabled JPEG snapshot. The dashboard takes the centre square at 100%, 75% or 50% scale, displays a 240×240 crop, and downsamples it to **24×24 RGB565**. The 2,304-character hex payload is sent to the S3. A fresh camera heartbeat is required for camera snapshots. **Analyse a photo…** and **Teach without the camera** run the same crop and downsampling on an image file chosen in the browser, so no camera is needed. This deliberately avoids adding a JPEG decoder or a new camera firmware dependency to ArduinoDroid.

The S3 extracts 16 features: green/yellow/brown fractions, plant-like coverage, mean RGB, bounding-box width/height, centroid X/Y, lower/left/central/edge distribution and box density. Each is normalised to 0–1000. The colour masks are mutually exclusive. Image quality rejects crops with over 65% very dark pixels, over 65% near-white pixels, or under 3.5% plant-like coverage. These simple gates do not prove a plant is correctly framed.

An example stores the feature vector and your condition and/or stage label. The two outputs learn independently. Each requires at least 3 distinct feature examples in each of 2 classes. Exact duplicates are rejected. Three nearest eligible examples vote; at least two must agree. The closest distance and the third-neighbour distance must also pass rejection thresholds. These cutoffs are initial engineering settings, not calibrated probabilities. Adding many near-identical views does not establish general accuracy.

Keep the plant, camera distance/angle, backdrop and light consistent. Gather natural variation over time; do not damage, overwater or deprive a plant to create labels. Until genuine alternative conditions/stages exist, colour observations and recorded age remain useful while predictions stay unknown. Validate with new snapshots on later days, including unsuitable/empty scenes. Treat mistakes as a reason to improve framing or retrain, not a reason to increase confidence in the label.

## Behaviour and persistence

- The manual button captures on demand. Opt-in observation runs every 60 seconds while the dashboard is visible, connected and not in setup/teaching/reset confirmation. Closing the page stops analysis; no cloud service is involved.
- The existing MJPEG view pauses during capture and resumes according to the user's camera setting. Camera errors clear the pending frame and leave teaching disabled.
- Each snapshot receives a frame ID. Teaching requires that exact current frame, matching profile revision, usable image and a frame not already labelled. Another browser replacing the frame invalidates the teaching button.
- An observation is shown as current for up to 120 seconds. The server accepts a correctly identified training frame for up to five minutes, although the dashboard disables teaching once it is no longer current.
- The model holds 64 examples, saved in NVS and tied to the plant record's cycle and crop. Replacing the plant changes cycle; previous examples are not used for the new record. Changing stage for the same plant can retain examples.
- Clear examples is confirmed explicitly and applies the chosen crop. Export downloads feature vectors and labels as JSON; *Teach without the camera → Examples file* imports it again for the same preset and crop.
- Predictions never trigger the pump, change cover/fan commands, rewrite stage/age or adjust thresholds.

When the RTC is unavailable, a known age can advance using powered-on elapsed time in the current session. Time while powered off cannot be recovered without a reliable clock. Set the RTC and correct the recorded age if needed. Transplanting is separate from chronological age.

## API

| Route | Purpose |
|---|---|
| GET `/api/plants` | Catalogue and permitted stages |
| POST `/api/plants/preview` | Validate details and return proposed limits without saving |
| POST `/api/plants/apply` | Revalidate, check review/repot acknowledgement and save profile |
| POST `/api/vision/observe` | Extract features and infer from a 24×24 RGB565 crop (`source=photo` skips the camera-online check) |
| POST `/api/vision/teach` | Save labels for the exact accepted frame |
| POST `/api/vision/clear` | Confirm/reset the model and crop |
| POST `/api/vision/import` | `mode=photo`: one labelled 24×24 photo crop, with the same quality, duplicate and capacity checks as teaching. `mode=features`: up to 64 exported rows `condition,stage,f0…f15;…`, refused for a different preset, with duplicates and unsupported stages skipped and one NVS write |
| GET `/api/vision/export` | Download feature examples (now also includes numeric `conditionIndex`/`stageIndex`) |
| GET `/api/status` | Profile, observation, labels, counts, device status |

Writes require the dashboard request header, completed startup/storage and the current profile revision. Plant application is refused during watering or calibration. Profile and its limits are saved together; failed writes retain the in-memory prior configuration. Teaching saves occur only on explicit labels, not on every observation.

## Why this first version

A small feature model fits the existing S3 Arduino application without pretending that an untrained neural network can recognise every plant. A later validated quantised CNN could replace the feature classifier after collecting a representative labelled dataset; it would require separate training, memory/latency measurements and integration work.

Primary references used for design context:
- [Espressif camera driver](https://github.com/espressif/esp32-camera)
- [Espressif ESP-DL introduction](https://docs.espressif.com/projects/esp-dl/en/latest/getting_started/readme.html)
- [Mohanty et al., image-based plant disease detection](https://www.frontiersin.org/journals/plant-science/articles/10.3389/fpls.2016.01419/full) — motivates evaluating on new capture conditions, not just training images.
