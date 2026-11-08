#include "AutobotanyPresentation.h"

namespace ABPresentation {
const Asset Assets[] = {
{"/assets/v3.6.0/garden.css","text/css; charset=utf-8",GardenCSS,sizeof(GardenCSS),true,true},
{"/assets/v3.6.0/garden.js","application/javascript; charset=utf-8",GardenJS,sizeof(GardenJS),true,true},
{"/","text/html; charset=utf-8",Entrance,sizeof(Entrance),true,false},
{"/index.html","text/html; charset=utf-8",Entrance,sizeof(Entrance),true,false},
{"/credits","text/html; charset=utf-8",Credits,sizeof(Credits),true,false},
{"/credits.html","text/html; charset=utf-8",Credits,sizeof(Credits),true,false},
{"/assets/v3.1.0/story.css","text/css; charset=utf-8",StoryCSS,sizeof(StoryCSS),true,true},
{"/assets/v3.1.0/motion.js","application/javascript; charset=utf-8",MotionJS,sizeof(MotionJS),true,true},
{"/assets/v3.6.0/autobotany.png","image/png",Logo,sizeof(Logo),false,true},
{"/assets/v3.1.0/colegio.jpg","image/jpeg",School,sizeof(School),false,true},
{"/assets/v3.1.0/archdiocese.png","image/png",Archdiocese,sizeof(Archdiocese),false,true},
{"/assets/vendor/gsap-3.15.0/gsap.min.js","application/javascript; charset=utf-8",GSAP,sizeof(GSAP),true,true}
};
const size_t Count=sizeof(Assets)/sizeof(Assets[0]);
}
