"""Rebuild only website data files; preserve sketch logic and user configuration."""
from pathlib import Path
import gzip
import hashlib
import json

root=Path(__file__).resolve().parent
site=root/'Website'
sketch=root/'AutobotanyS3V350'
if not sketch.is_dir():
    sketch=root/'AutobotanyS3V340'
if not sketch.is_dir():
    sketch=root/'AutobotanyS3V330'
if not sketch.is_dir():
    sketch=root/'ProjectAutobotany'
if not sketch.is_dir():
    raise SystemExit('Keep this script beside Website and the complete sketch folder.')

files=[
    ('GardenCSS','assets/v3.6.0/garden.css','text/css; charset=utf-8',['/assets/v3.6.0/garden.css']),
    ('GardenJS','assets/v3.6.0/garden.js','application/javascript; charset=utf-8',['/assets/v3.6.0/garden.js']),
    ('Entrance','index.html','text/html; charset=utf-8',['/','/index.html']),
    ('Credits','credits.html','text/html; charset=utf-8',['/credits','/credits.html']),
    ('StoryCSS','assets/v3.1.0/story.css','text/css; charset=utf-8',['/assets/v3.1.0/story.css']),
    ('MotionJS','assets/v3.1.0/motion.js','application/javascript; charset=utf-8',['/assets/v3.1.0/motion.js']),
    ('Logo','assets/v3.6.0/autobotany.png','image/png',['/assets/v3.6.0/autobotany.png']),
    ('School','assets/v3.1.0/colegio.jpg','image/jpeg',['/assets/v3.1.0/colegio.jpg']),
    ('Archdiocese','assets/v3.1.0/archdiocese.png','image/png',['/assets/v3.1.0/archdiocese.png']),
]
declarations=[];entries=[];manifest=[]
for symbol,filename,mime,routes in files:
    raw=(site/filename).read_bytes()
    compressed=not mime.startswith('image/')
    data=gzip.compress(raw,compresslevel=9,mtime=0) if compressed else raw
    declarations.append(f'extern const uint8_t {symbol}[{len(data)}] PROGMEM;')
    text='#include "AutobotanyPresentation.h"\n\nnamespace ABPresentation {\n'
    text+=f'const uint8_t {symbol}[] PROGMEM = {{\n'
    text+=''.join(','.join(f'0x{byte:02x}' for byte in data[i:i+24])+',\n' for i in range(0,len(data),24))
    text+='};\n}\n'
    (sketch/f'Asset{symbol}.cpp').write_text(text)
    for route in routes:
        entries.append('{"%s","%s",%s,sizeof(%s),%s,%s}'%(route,mime,symbol,symbol,str(compressed).lower(),str(route.startswith('/assets/')).lower()))
    manifest.append(dict(symbol=symbol,source=filename,mime=mime,routes=routes,gzip=compressed,bytes=len(data),sha256=hashlib.sha256(raw).hexdigest()))

header='#pragma once\n#include <Arduino.h>\n\nnamespace ABPresentation {\n'
header+='struct Asset { const char *path; const char *mime; const uint8_t *data; size_t size; bool gzip; bool cached; };\n'
header+='\n'.join(declarations)+f'\nextern const Asset Assets[{len(entries)}];\nextern const size_t Count;\n}}\n'
(sketch/'AutobotanyPresentation.h').write_text(header)
index='#include "AutobotanyPresentation.h"\n\nnamespace ABPresentation {\nconst Asset Assets[] = {\n'+',\n'.join(entries)+'\n};\nconst size_t Count=sizeof(Assets)/sizeof(Assets[0]);\n}\n'
(sketch/'AutobotanyPresentation.cpp').write_text(index)

for stem,filename,symbol,delimiter in [
    ('AutobotanyUI','dashboard.html','AUTOBOTANY_HTML','AUTOBOTANY'),
    ('AutobotanyBootUI','startup.html','AUTOBOTANY_BOOT_HTML','ABBOOT'),
]:
    raw=(site/filename).read_bytes();body=raw.decode('utf-8')
    if ')'+delimiter+'"' in body:
        raise SystemExit(f'{filename} contains a reserved C++ raw-string terminator.')
    (sketch/f'{stem}.h').write_text(f'#pragma once\n#include <Arduino.h>\nextern const char {symbol}[{len(raw)+1}] PROGMEM;\n')
    (sketch/f'{stem}.cpp').write_text(f'#include "{stem}.h"\nconst char {symbol}[] PROGMEM = R"{delimiter}({body}){delimiter}";\n')

for page in ('credits','dashboard'):
    (site/page).mkdir(exist_ok=True)
    (site/page/'index.html').write_bytes((site/(page+'.html')).read_bytes())
(root/'website-manifest.json').write_text(json.dumps(dict(flashAssetBytes=sum(x['bytes'] for x in manifest),files=manifest),indent=2)+'\n')
print(f'Built {len(files)} separate website asset sources. Compile the complete sketch folder.')
