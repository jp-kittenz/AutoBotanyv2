"""Encode the supplied logos as small RGB565 flash assets (requires Pillow)."""
from pathlib import Path
from PIL import Image,ImageOps
import hashlib,json
root=Path(__file__).resolve().parent
sketch=root/'AutobotanyS3V350'
if not sketch.is_dir():
    sketch=root/'AutobotanyS3V340'
if not sketch.is_dir():
    sketch=root/'AutobotanyS3V330'
if not sketch.is_dir():sketch=root/'ProjectAutobotany'
art=root/'TFTArtwork'
items=[('Logo','autobotany-original.png',(128,460,1334,1094),(118,62)),('School','csfj-transparent.png',None,(52,52))]
header='#pragma once\n#include <Arduino.h>\nnamespace ABTFT {\n'
source='#include "AutobotanyTFTAssets.h"\n\nnamespace ABTFT {\n'
manifest=[]
for name,filename,crop,size in items:
 p=art/filename
 im=Image.open(p).convert('RGBA')
 if crop:im=im.crop(crop)
 im=ImageOps.contain(im,size,Image.Resampling.LANCZOS)
 background=(8,12,24) if name=='School' else (255,255,255) # RGB565 0x0863.
 canvas=Image.new('RGBA',size,background+(255,));canvas.alpha_composite(im,((size[0]-im.width)//2,(size[1]-im.height)//2));canvas=canvas.convert('RGB')
 words=[((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b in canvas.getdata()]
 header+=f'extern const uint16_t {name}[{len(words)}] PROGMEM;\n'
 source+=f'const uint16_t {name}[] PROGMEM = {{\n'
 source+=''.join(','.join(f'0x{x:04x}' for x in words[i:i+16])+',\n' for i in range(0,len(words),16))
 source+='};\n'
 manifest.append(dict(symbol=name,source=filename,sourceSha256=hashlib.sha256(p.read_bytes()).hexdigest(),crop=crop,width=size[0],height=size[1],bytes=len(words)*2))
(sketch/'AutobotanyTFTAssets.h').write_text(header+'}\n')
(sketch/'AutobotanyTFTAssets.cpp').write_text(source+'}\n')
(art/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Encoded two TFT logos:',sum(i['bytes'] for i in manifest),'flash bytes; no runtime image decoder.')
