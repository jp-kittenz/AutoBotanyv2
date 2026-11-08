# Plant presets — v3.5.0

The catalogue contains 34 familiar Philippine crops, herbs and nursery plants. Targets below are the exact **base engineering defaults** in the source for an established, nonjuvenile plant before exposure/event adjustments. They have not been validated in crop trials and are not published species-specific recommendations. Several plants intentionally share broad ranges; false precision would not make the sensors more accurate.

Relative soil percentages depend on calibration and medium. Brightness is measured in lux, not PAR. Air humidity/temperature limits are advisory monitoring ranges; the cooling fan still follows the chip temperature. The lighting cover responds to the saved brightness limit using the existing controller. Pot planning volumes are approximate hints for one compact plant or a small leafy clump, not guarantees of mature yield.

The website accepts **1–20 L**. Seed/sprout/seedling profiles allow nursery pots. At later stages, a pot below its planning size requires a repot acknowledgement. Profiles above 20 L are nursery-only within this product range. Check drainage, pot depth, cultivar and root growth as well as volume.

| Plant | Planning pot (L) | Relative soil (%) | Brightness (lux) | Air (°C) | RH (%) |
|---|---:|---|---|---|---|
| Pechay | 3 | 40–75 | 4,000–28,000 | 18–30 | 45–85 |
| Kangkong | 5 | 50–85 | 6,000–40,000 | 22–34 | 50–90 |
| Mustasa | 3 | 40–75 | 5,000–30,000 | 18–30 | 45–85 |
| Alugbati | 8 | 40–80 | 6,000–40,000 | 22–34 | 50–85 |
| Saluyot | 8 | 40–75 | 8,000–45,000 | 22–35 | 45–85 |
| Kulitis | 5 | 35–75 | 8,000–45,000 | 22–35 | 40–85 |
| Lettuce | 3 | 40–75 | 4,000–25,000 | 16–28 | 45–80 |
| Malunggay | 30 | 30–65 | 8,000–50,000 | 22–35 | 35–80 |
| Kamatis (compact) | 15 | 40–75 | 10,000–50,000 | 18–32 | 40–80 |
| Talong (compact) | 15 | 40–75 | 10,000–50,000 | 22–34 | 45–85 |
| Siling labuyo | 10 | 35–70 | 10,000–45,000 | 20–33 | 40–80 |
| Siling haba | 10 | 35–75 | 10,000–45,000 | 20–33 | 40–80 |
| Bell pepper | 10 | 40–75 | 8,000–40,000 | 18–30 | 40–80 |
| Okra (dwarf) | 15 | 35–75 | 10,000–50,000 | 22–35 | 40–85 |
| Sitaw (bush) | 12 | 35–75 | 8,000–45,000 | 20–34 | 40–85 |
| Pipino (compact) | 15 | 45–80 | 8,000–45,000 | 20–32 | 45–85 |
| Ampalaya | 25 | 40–80 | 10,000–50,000 | 22–34 | 45–85 |
| Labanos | 5 | 40–75 | 6,000–35,000 | 16–30 | 40–80 |
| Carrot (short-root) | 8 | 40–75 | 6,000–35,000 | 16–29 | 40–80 |
| Kamote (leaf harvest) | 20 | 35–70 | 8,000–45,000 | 22–34 | 40–85 |
| Luya / ginger | 10 | 45–80 | 3,000–25,000 | 20–32 | 50–85 |
| Luyang dilaw / turmeric | 10 | 45–80 | 4,000–30,000 | 20–32 | 50–85 |
| Basil | 5 | 40–75 | 6,000–35,000 | 20–32 | 40–80 |
| Mint | 3 | 45–80 | 3,000–25,000 | 18–30 | 45–85 |
| Philippine oregano | 3 | 25–60 | 4,000–30,000 | 20–34 | 35–80 |
| Wansoy / coriander | 3 | 40–75 | 4,000–28,000 | 16–29 | 40–80 |
| Kulantro | 3 | 45–80 | 2,000–20,000 | 20–32 | 50–85 |
| Kinchay | 5 | 45–80 | 3,000–25,000 | 16–29 | 50–85 |
| Tanglad | 15 | 35–75 | 8,000–45,000 | 22–35 | 40–85 |
| Pandan | 10 | 45–80 | 3,000–25,000 | 22–33 | 50–85 |
| Spring onion | 3 | 35–70 | 6,000–35,000 | 18–32 | 40–80 |
| Chives | 3 | 35–70 | 5,000–30,000 | 16–30 | 40–80 |
| Bawang / garlic greens | 3 | 30–65 | 6,000–35,000 | 16–30 | 35–80 |
| Calamansi (young/dwarf) | 25 | 30–70 | 8,000–50,000 | 20–34 | 40–85 |

## Setup adjustments

- **Seed / sprout / seedling:** raise the relative soil lower target by 5 points, capped at 60%; cap the upper brightness target at 22,000 lux.
- **Just transplanted / recently rooted cutting:** cap the upper brightness target at 20,000 lux. This event does not expire automatically. Review it after recovery/root establishment and change to established when appropriate.
- **Bright shade:** lower the minimum brightness target to at most 3,000 lux and cap the upper at 22,000 lux. Other exposure options retain the species range; choosing shade does not make a sun-loving crop shade-adapted.
- **Medium and pot size:** stored for guidance and future tuning. They do not rescale sensor percentages without calibration or derive a watering volume.
- **Flowering / fruiting / mature:** use the species base targets; this first version does not claim a validated distinct irrigation schedule for every stage. Fruiting appears only for the catalogue's fruiting/vine profiles.
- **Custom thresholds:** remain editable after applying a preset. Reapplying the setup proposes fresh preset targets and marks the profile as using presets. Reservoir, heat-index and chip-fan limits are retained.

Stage and planting event are separate. Just sown requires Seed; a rooted cutting cannot be Seed. For age, enter days since sowing/propagation or the known date, otherwise choose unknown. A transplant does not reset an existing plant's age.

## Crop notes

- **Pechay** (*Brassica rapa subsp. chinensis*): Harvest young leaves; give afternoon shade in hot weather.
- **Kangkong** (*Ipomoea aquatica*): This profile is for drained pot culture, not flooded or hydroponic growing.
- **Mustasa** (*Brassica juncea*): Baby-leaf harvest suits small containers; heat can encourage bolting.
- **Alugbati** (*Basella alba*): Provide a small trellis; prune the vine and harvest leaves regularly.
- **Saluyot** (*Corchorus olitorius*): Pinch tall growth; a small pot is a temporary nursery container.
- **Kulitis** (*Amaranthus spp.*): Use edible locally identified varieties; harvest as baby greens in small pots.
- **Lettuce** (*Lactuca sativa*): Choose heat-tolerant loose-leaf varieties; midday heat can cause bolting.
- **Malunggay** (*Moringa oleifera*): Nursery / pruned leaf harvest only here. A mature tree needs substantially more root space.
- **Kamatis (compact)** (*Solanum lycopersicum*): Choose a dwarf/bush cultivar, stake it and repot before heavy fruiting.
- **Talong (compact)** (*Solanum melongena*): Choose a compact cultivar; support heavy fruit and keep drainage open.
- **Siling labuyo** (*Capsicum frutescens*): One plant per pot; avoid persistently saturated roots.
- **Siling haba** (*Capsicum annuum*): Support fruiting stems and increase root space as the canopy grows.
- **Bell pepper** (*Capsicum annuum*): A compact cultivar is best; hot afternoons may affect fruit set.
- **Okra (dwarf)** (*Abelmoschus esculentus*): Choose a dwarf cultivar and a deep pot; mature standard plants need more space.
- **Sitaw (bush)** (*Vigna unguiculata*): Choose bush/small cultivars; climbing yardlong beans need a trellis and more root room.
- **Pipino (compact)** (*Cucumis sativus*): Use a compact variety and trellis; avoid crowding leaves around the sensor.
- **Ampalaya** (*Momordica charantia*): A small pot is nursery-only. Use a large final container and a strong trellis.
- **Labanos** (*Raphanus sativus*): Use a short-root cultivar and a pot deeper than its expected root length.
- **Carrot (short-root)** (*Daucus carota subsp. sativus*): Choose short/round cultivars and loose deep medium; avoid transplanting taproots.
- **Kamote (leaf harvest)** (*Ipomoea batatas*): Small-pot mode is for leaves / starts; tuber production needs a larger container.
- **Luya / ginger** (*Zingiber officinale*): Use a wide drained pot and bright filtered light; rhizomes need room to spread.
- **Luyang dilaw / turmeric** (*Curcuma longa*): Use a wide pot; natural end-of-season yellowing is not automatically disease.
- **Basil** (*Ocimum basilicum*): Pinch growing tips; flowering changes appearance and leaf growth.
- **Mint** (*Mentha spp.*): Keep in its own pot; give afternoon shade in hot exposed locations.
- **Philippine oregano** (*Coleus amboinicus*): Fleshy leaves need free drainage; let the medium partially dry between watering.
- **Wansoy / coriander** (*Coriandrum sativum*): Sow directly; hot weather can cause early bolting.
- **Kulantro** (*Eryngium foetidum*): Bright shade suits this herb; keep it distinct from wansoy/coriander.
- **Kinchay** (*Apium graveolens*): Harvest leaves and keep moisture even; avoid harsh midday heat.
- **Tanglad** (*Cymbopogon citratus*): Divide crowded clumps; a mature clump quickly outgrows a small pot.
- **Pandan** (*Pandanus amaryllifolius*): Use filtered light and repot as the clump expands.
- **Spring onion** (*Allium fistulosum*): Grow a small clump in a drained pot; harvest outer leaves.
- **Chives** (*Allium schoenoprasum*): Allow airflow through the clump and avoid standing water.
- **Bawang / garlic greens** (*Allium sativum*): Small-pot mode targets greens; bulb production also depends on cultivar and season.
- **Calamansi (young/dwarf)** (*Citrus x microcarpa*): Young/dwarf nursery profile. A productive mature tree needs a larger final container.

## Source context and commissioning

These primary extension sources inform container choice, drainage, cultivar/stage guidance and the need to adapt watering. They do **not** validate the numeric relative-sensor targets above.

- [Philippine ATI: Container gardening](https://ati2.da.gov.ph/ati-car/content/sites/default/files/2022-12/container_gardening.pdf)
- [Philippine ATI: Seed sowing](https://ati2.da.gov.ph/ati-8/content/sites/default/files/2023-02/seed%20sowing.pdf)
- [DA High Value Crops: Production manuals](https://hvcdp.da.gov.ph/production-manuals/)
- [University of Maryland Extension: Maintaining container-grown vegetables](https://extension.umd.edu/resource/maintaining-container-grown-vegetables)
- [University of Maryland Extension: Growing vegetables in containers](https://extension.umd.edu/resource/growing-vegetables-containers-and-salad-tables)

Commission with the actual probe, drained medium, pump and container. Record how much a five-second pulse dispenses and how the soil reading changes afterward. Observe over several days before trusting a threshold; the supplied build keeps automatic watering disabled. Vision labels are advisory and cannot enable watering.
