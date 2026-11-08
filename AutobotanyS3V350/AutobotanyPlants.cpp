#include "AutobotanyPlants.h"
#include <string.h>
#include <math.h>
namespace ABPlant {
// Commissioning targets for this device's relative soil scale and lux sensor.
// They are NOT measured VWC, PAR, crop prescriptions or a validated irrigation model.
// Pot sizes are conservative planning hints for one plant / a small leafy clump.
static const uint16_t Leaf=0x7f & ~(1<<Fruiting), Fruit=0x7f;
const Preset Presets[]={
 {"pechay","Pechay","Brassica rapa subsp. chinensis","Leafy","Harvest young leaves; give afternoon shade in hot weather.",3,Leaf,40,75,4000,28000,18,30,45,85},
 {"kangkong","Kangkong","Ipomoea aquatica","Leafy","This profile is for drained pot culture, not flooded or hydroponic growing.",5,Leaf,50,85,6000,40000,22,34,50,90},
 {"mustasa","Mustasa","Brassica juncea","Leafy","Baby-leaf harvest suits small containers; heat can encourage bolting.",3,Leaf,40,75,5000,30000,18,30,45,85},
 {"alugbati","Alugbati","Basella alba","Leafy","Provide a small trellis; prune the vine and harvest leaves regularly.",8,Leaf,40,80,6000,40000,22,34,50,85},
 {"saluyot","Saluyot","Corchorus olitorius","Leafy","Pinch tall growth; a small pot is a temporary nursery container.",8,Leaf,40,75,8000,45000,22,35,45,85},
 {"kulitis","Kulitis","Amaranthus spp.","Leafy","Use edible locally identified varieties; harvest as baby greens in small pots.",5,Leaf,35,75,8000,45000,22,35,40,85},
 {"lettuce","Lettuce","Lactuca sativa","Leafy","Choose heat-tolerant loose-leaf varieties; midday heat can cause bolting.",3,Leaf,40,75,4000,25000,16,28,45,80},
 {"malunggay","Malunggay","Moringa oleifera","Nursery","Nursery / pruned leaf harvest only here. A mature tree needs substantially more root space.",30,Leaf,30,65,8000,50000,22,35,35,80},
 {"kamatis","Kamatis (compact)","Solanum lycopersicum","Fruiting","Choose a dwarf/bush cultivar, stake it and repot before heavy fruiting.",15,Fruit,40,75,10000,50000,18,32,40,80},
 {"talong","Talong (compact)","Solanum melongena","Fruiting","Choose a compact cultivar; support heavy fruit and keep drainage open.",15,Fruit,40,75,10000,50000,22,34,45,85},
 {"siling-labuyo","Siling labuyo","Capsicum frutescens","Fruiting","One plant per pot; avoid persistently saturated roots.",10,Fruit,35,70,10000,45000,20,33,40,80},
 {"siling-haba","Siling haba","Capsicum annuum","Fruiting","Support fruiting stems and increase root space as the canopy grows.",10,Fruit,35,75,10000,45000,20,33,40,80},
 {"bell-pepper","Bell pepper","Capsicum annuum","Fruiting","A compact cultivar is best; hot afternoons may affect fruit set.",10,Fruit,40,75,8000,40000,18,30,40,80},
 {"okra","Okra (dwarf)","Abelmoschus esculentus","Fruiting","Choose a dwarf cultivar and a deep pot; mature standard plants need more space.",15,Fruit,35,75,10000,50000,22,35,40,85},
 {"sitaw","Sitaw (bush)","Vigna unguiculata","Fruiting","Choose bush/small cultivars; climbing yardlong beans need a trellis and more root room.",12,Fruit,35,75,8000,45000,20,34,40,85},
 {"pipino","Pipino (compact)","Cucumis sativus","Fruiting","Use a compact variety and trellis; avoid crowding leaves around the sensor.",15,Fruit,45,80,8000,45000,20,32,45,85},
 {"ampalaya","Ampalaya","Momordica charantia","Vine","A small pot is nursery-only. Use a large final container and a strong trellis.",25,Fruit,40,80,10000,50000,22,34,45,85},
 {"labanos","Labanos","Raphanus sativus","Roots","Use a short-root cultivar and a pot deeper than its expected root length.",5,Leaf,40,75,6000,35000,16,30,40,80},
 {"carrot","Carrot (short-root)","Daucus carota subsp. sativus","Roots","Choose short/round cultivars and loose deep medium; avoid transplanting taproots.",8,Leaf,40,75,6000,35000,16,29,40,80},
 {"kamote","Kamote (leaf harvest)","Ipomoea batatas","Roots","Small-pot mode is for leaves / starts; tuber production needs a larger container.",20,Leaf,35,70,8000,45000,22,34,40,85},
 {"luya","Luya / ginger","Zingiber officinale","Roots","Use a wide drained pot and bright filtered light; rhizomes need room to spread.",10,Leaf,45,80,3000,25000,20,32,50,85},
 {"luyang-dilaw","Luyang dilaw / turmeric","Curcuma longa","Roots","Use a wide pot; natural end-of-season yellowing is not automatically disease.",10,Leaf,45,80,4000,30000,20,32,50,85},
 {"basil","Basil","Ocimum basilicum","Herbs","Pinch growing tips; flowering changes appearance and leaf growth.",5,Leaf,40,75,6000,35000,20,32,40,80},
 {"mint","Mint","Mentha spp.","Herbs","Keep in its own pot; give afternoon shade in hot exposed locations.",3,Leaf,45,80,3000,25000,18,30,45,85},
 {"oregano","Philippine oregano","Coleus amboinicus","Herbs","Fleshy leaves need free drainage; let the medium partially dry between watering.",3,Leaf,25,60,4000,30000,20,34,35,80},
 {"wansoy","Wansoy / coriander","Coriandrum sativum","Herbs","Sow directly; hot weather can cause early bolting.",3,Leaf,40,75,4000,28000,16,29,40,80},
 {"kulantro","Kulantro","Eryngium foetidum","Herbs","Bright shade suits this herb; keep it distinct from wansoy/coriander.",3,Leaf,45,80,2000,20000,20,32,50,85},
 {"kinchay","Kinchay","Apium graveolens","Herbs","Harvest leaves and keep moisture even; avoid harsh midday heat.",5,Leaf,45,80,3000,25000,16,29,50,85},
 {"tanglad","Tanglad","Cymbopogon citratus","Herbs","Divide crowded clumps; a mature clump quickly outgrows a small pot.",15,Leaf,35,75,8000,45000,22,35,40,85},
 {"pandan","Pandan","Pandanus amaryllifolius","Herbs","Use filtered light and repot as the clump expands.",10,Leaf,45,80,3000,25000,22,33,50,85},
 {"spring-onion","Spring onion","Allium fistulosum","Herbs","Grow a small clump in a drained pot; harvest outer leaves.",3,Leaf,35,70,6000,35000,18,32,40,80},
 {"chives","Chives","Allium schoenoprasum","Herbs","Allow airflow through the clump and avoid standing water.",3,Leaf,35,70,5000,30000,16,30,40,80},
 {"bawang","Bawang / garlic greens","Allium sativum","Herbs","Small-pot mode targets greens; bulb production also depends on cultivar and season.",3,Leaf,30,65,6000,35000,16,30,35,80},
 {"calamansi","Calamansi (young/dwarf)","Citrus x microcarpa","Nursery","Young/dwarf nursery profile. A productive mature tree needs a larger final container.",25,Fruit,30,70,8000,50000,20,34,40,85}
};
const uint8_t Count=sizeof(Presets)/sizeof(Presets[0]);
const char *stageName(uint8_t n){static const char *s[]={"seed","sprout","seedling","vegetative","flowering","fruiting","mature"};return n<StageCount?s[n]:"unknown";}
const char *mediumName(uint8_t n){static const char *s[]={"potting-mix","coco-blend","soil-compost"};return n<MediumCount?s[n]:"unknown";}
const char *eventName(uint8_t n){static const char *s[]={"established","just-sown","just-transplanted","cutting"};return n<EventCount?s[n]:"unknown";}
const char *exposureName(uint8_t n){static const char *s[]={"morning-sun","full-sun","bright-shade"};return n<ExposureCount?s[n]:"unknown";}
int find(const char *id){for(uint8_t n=0;n<Count;n++)if(!strcmp(Presets[n].id,id))return n;return -1;}
bool juvenile(uint8_t s){return s<=Seedling;}
bool Profile::valid()const{return version==Version && preset<Count && stage<StageCount && (Presets[preset].stages&(1<<stage)) && medium<MediumCount && event<EventCount && exposure<ExposureCount && ageKnown<=1 && customLimits<=1 && isfinite(potL) && potL>=1 && potL<=20 && ageSeconds<=36500UL*86400UL && anchorEpoch>=0 && anchorEpoch<4102444800LL && limits.valid() && !(event==Sown && stage!=Seed) && !(event==Cutting && stage==Seed);}
bool needsRepot(const Profile &p){return !juvenile(p.stage) && p.potL<Presets[p.preset].minPotL;}
AB::Thresholds suggest(const Profile &p,const AB::Thresholds &hardware){
  AB::Thresholds out=hardware;const Preset &r=Presets[p.preset];
  out.values[0]=r.moistureLow;out.values[1]=r.moistureHigh;
  out.values[3]=r.lightLow;out.values[4]=r.lightHigh;
  out.values[5]=r.humidityLow;out.values[6]=r.humidityHigh;
  out.values[7]=r.tempLow;out.values[8]=r.tempHigh;
  if(juvenile(p.stage)){out.values[0]=fminf(60,r.moistureLow+5);out.values[4]=fminf(22000,r.lightHigh);}
  if(p.event==Transplanted || p.event==Cutting)out.values[4]=fminf(20000,out.values[4]);
  if(p.exposure==BrightShade){out.values[3]=fminf(3000,out.values[3]);out.values[4]=fminf(22000,out.values[4]);}
  // Pot volume and medium inform guidance; neither changes uncalibrated sensor percentages.
  // Hardware reservoir, heat-index advisory, chip fan thresholds are retained.
  return out;
}
double ageSeconds(const Profile &p,uint32_t epoch,uint64_t tick,uint64_t saved){
  if(!p.ageKnown)return NAN;
  uint64_t elapsed=p.anchorEpoch>0 && epoch>=p.anchorEpoch ? uint64_t(epoch-p.anchorEpoch) : tick>=saved?(tick-saved)/1000:0;
  return double(p.ageSeconds)+elapsed;
}
}
