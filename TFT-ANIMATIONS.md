# TFT animation guide — v3.6.0

Every designated card has a separate scene renderer. Shared helpers draw primitives such as lines, arcs, leaves and droplets. Animation choices describe samples or commanded states; they do not change calibration, automation thresholds, motor polarity, pump protection, servo commands or run limits.

## Pages and interactions

| Card | Scene and motion | SELECT |
|---|---|---|
| 1 Reservoir | Graduated tank, moving water surface, accelerating drops, splashes and fading ripples; empty/unknown indicators | Existing reservoir sampling command |
| 2 Soil moisture | Root bed, flexing stem, veined leaves, shaded petals and an orbiting bee; wilting, petal fall or dead plant as appropriate | Existing soil sampling command |
| 3 Rain | Drifting cloud over a collection gauge, accelerating rain and impact splashes; dry air puffs or missing-data indicator | Cloud burst when raining; expanding puffs otherwise |
| 4 Temperature | Thermometer mercury and ticks with rising heat wisps, cool streaks or falling icy sparkles | Thermometer bounce and expanding thermal ring |
| 5 Humidity | Veined leaf with dew, evaporating bubbles, sliding drops or dry-air motes | Dew bubbles scatter and pop into sparkles |
| 6 Sunlight | Breathing gold core and rotating rays; drifting cloud, moon/stars, rain or filament bulb when appropriate | Rays disperse and return; cloud displacement, night meteor, bulb flare or fault sparks |
| 7 Cooling fan | Framed, shaded five-blade rotor and moving airflow; static when OFF | Existing manual fan toggle |
| 8 Heat index | Breathing/blinking face with blush, sweat, scarf or shiver according to the reading | Wink, little hop and expressive sparkles |
| 9 Sun blocker | Louvered greenhouse shade, moving light/rain streaks, weather cue and swaying plant | Existing sun-cover command |
| 10 Rain blocker | Open umbrella roof or folded shelter, ribs, intercepted rain/runoff and plant/puddles | Existing rain-cover command |
| 11 Pump | Graduated supply tank, pipe particles, turning impeller and outlet drops/ripple; static when OFF | Existing protected pump command |
| 12 Chip temperature | Animated traces, signal bars, heat wisps, cooling streaks or frost | Expanding circuit pulse |
| 13 Website / SoftAP | Pulsing Wi-Fi and a camera orbit beside animated connected devices | Broadcast ring plus device/packet burst or globe spin boost |
| 14 Your Plant | Sprite of the selected preset growing in a pot, with a growth-stage rail and the saved profile on the right | Animation only: sap pulse up the rail, ring at the current stage, plant waves, sparkles rise |

Action cards (reservoir, soil, fan, sun blocker, rain blocker, pump) run their command and, when it succeeds, play a transition: a probe dips into the tank or soil; the fan or pump impeller spins up or coasts down (the motion rejoins the steady speed without a jump); the shade cloth slides; the umbrella opens or folds. A refused command shows its message and no transition. The visual SELECT actions (now eight, including card 14) are cosmetic, last 2.2 seconds and can be restarted. Navigation cancels the reaction. The same button press used to wake from display idle is consumed and never triggers an action.

## Context rules

Missing, stale and uncalibrated readings select an unknown/fault variation. Displayed older numbers retain their stale labels. A fresh, calibrated rain value above zero means rain. Night is 18:00–05:59 in the configured Philippines timezone, requiring a present RTC and valid clock. Cloud/sun images are brightness illustrations inferred from lux, not cloud measurements. A failed/stale BH1750 uses the broken bulb even during night or rain.

| Input | Visual thresholds |
|---|---|
| Reservoir | Above 50%: four drops; above 25% through 50%: two; above zero through 25%: one; zero/below: empty |
| Soil | Above 50%: healthy; above 25% through 50%: wilted; above zero through 25%: petal fall; zero/below: dead |
| Temperature | Below 10°C: icy; 10–below 22°C: cool; 22–below 32°C: warm; 32°C and above: hot |
| Humidity | Below 35%: dry; 35–below 70%: bubbles; 70% and above: drops |
| Sunlight | After night/rain priority: below 50 lux bulb, 50–999 clouds, 1,000–9,999 sun/cloud, 10,000+ sun |
| Heat index | At/below 0°C cold sad; above 0–below 15°C cold happy; 15–below 27°C happy; 27–below 35°C warm happy; 35°C+ hot sad |
| Chip | Below 10°C frost; below configured fan-on threshold normal traces; at/above threshold heat with cooling streaks when fan commanded ON |

Slow pump artwork means a low indicated supply; it does not adjust speed or permit a run that protection would reject. Fan/pump/cover drawings show commands, because there is no RPM, flow or cover-position feedback sensor.

## Network states

AP ready plus active AP mode produces the Wi-Fi tile. A camera heartbeat at most 45 seconds old, with at least one associated station, adds the red orbit and fading trail. A stopped AP produces alternating ! and ?. The right tile maps user-device count to waiting phone (0), phone (1), phone/laptop (2), two phones/laptop (3), or spinning globe (4+). USERS excludes one fresh camera; ALL includes every associated station. These are AP devices, not browser sessions or individual people. Device types are illustrative, and the globe does not indicate Internet access.

## Layout and timing

Portrait TFT resolution is 128x160. Each scene clips to its own 40x40 tile. The smallest text uses the classic 6x8 font.

| Vertical pixels | Cards 1–12 | Card 13 |
|---|---|---|
| 0–31 | Brand, page number, AP/SD/RTC indicators, title | Same header with 13/14 |
| 32–75 | Icon beside value and unit | Wi-Fi at x=16; devices at x=72 |
| 76–97 | Meter and reading/command status | Online/offline, users, IP |
| 98–119 | Age/raw reading or control details | Wi-Fi name; scrolling if needed |
| 120–147 | Tip counter, 13-position dots, two-line tip | Password, camera/total devices, YYYY-MM-DD |
| 148–159 | Navigation and SEL or PLAY cue | Navigation and PLAY cue |

## Card 14 — Your Plant

| Area | Contents |
|---|---|
| x 0–51, y 32–147 | Rounded pane. The stage rail is at x=5: seed at the bottom, mature at the top. Allowed stages are dots, stages the preset skips are short dashes, completed stages are filled, and the current stage pulses. The rail fills toward the next stage as recorded age advances. The plant is clipped to x ≥ 10 so it never touches the rail. |
| x 55–127, y 34–141 | Eleven 10-px rows: name (scrolls if longer than 12 characters), stage, age, pot (amber with ! when repotting is due), medium, exposure, planting event, a divider, then soil %, lux, air °C and RH % targets. |

The sprite comes from one of seven procedural plant forms: rosette, bush, climbing vine, trailing vine, grass/strap leaves, ginger-type stems and small tree. Each of the 34 presets has its own leaf shape, colours, flowers, fruit, pot props and height. Seed → sprout → seedling → vegetative → flowering → fruiting → mature changes the drawing, and recorded age grows the plant within each stage. Unknown age draws the middle of the stage. The pot width follows the saved pot volume. When fresh, calibrated soil moisture is below the target, the plant droops and a drop blinks on the pot. At night, fireflies replace the daytime butterfly or bee. With no saved profile, an empty pot and a bobbing seed packet point to Plant setup. The art is illustrative: it is drawn from the saved profile, not from the camera.

The pane is 52×116 = 6,032 pixels and reuses the existing 12,288-byte canvas, so no new RAM buffer is needed. Header and footer bands are unchanged. On this card, the middle bands only send the info column.

Three tips per original card rotate every eight seconds. The four-second intro has growing vines, particle drift, staged logo movement, orbit trails and a delayed credit reveal, followed by 480 ms fades through white. Updates target 40 ms; actual hardware timing depends on other controller work. One bounded bitmap is sent per buffered display tick. The 30-second idle screen and guarded wake behavior remain active, and automation continues during intro, reactions and idle.

See the included GIF previews for motion. They are software renders, not recordings of the physical TFT.

## Full review gallery

`AnimationPreview/v3.6.0/` holds the 3.6.0 renders, all made by the shipped C++ through the host simulation: `plant-sprites-1/2.png` (34 plants × stages), `growth-*.gif` timelapses, `card14-screens.png`, `all-cards.png`, and GIFs of the card 14 SELECT reaction, fan spin-up/down, sun cover, umbrella, and sun-rain/moon-rain clouds. The older gallery below predates the 3.6.0 transitions and rain clouds.

Open `AnimationPreview/Animation-gallery.html` for 88 distinct reachable state keys, 45 SELECT comparisons, 13 full cards, intro, sleep and wake. It contains its images and works offline. Use the card filter, pause switch and rework notes export. GIFs are also supplied separately. Network counts cover this build's six-station AP limit, including the camera. Artwork bands remain illustrative; preset-dependent status text uses the configured thresholds.
