#!/usr/bin/env python3
"""Prepare real Salamander action recordings offline; no FLAC decode in render.
Needs FFmpeg and NumPy. Sources and Git hashes are in mechanics-source/manifest.json.
The recorded hammer-return and key-release action are the same capture. They are
played once on note-off. Pedal down/up have two alternating recorded takes each.
"""
from pathlib import Path
import subprocess,struct,json,hashlib
import numpy as np
ROOT=Path(__file__).resolve().parent.parent
SOURCE=ROOT/'build-support/mechanics-source'
names=[f'rel{i}.flac' for i in range(1,89)]+[f'pedal{k}{n}.flac' for k in ('D','U') for n in (1,2)]
offset=0;entries=[];data=[];details=[]
for name in names:
 raw=subprocess.check_output(['ffmpeg','-v','error','-i',str(SOURCE/'Samples'/name),'-ac','2','-ar','48000','-f','f32le','-'])
 a=np.frombuffer(raw,dtype='<f4').reshape(-1,2).copy();assert np.isfinite(a).all()
 # Trim quiet lead-in, keep 1ms before the first significant recorded motion.
 envelope=np.max(np.abs(a),axis=1);hot=np.flatnonzero(envelope>float(envelope.max())*.04)
 start=max(0,int(hot[0])-48);end=min(len(a),int(hot[-1])+480,start+(48000 if name.startswith('pedal') else 24000))
 a=a[start:end];a-=a.mean(axis=0)
 a[:48]*=np.linspace(0,1,48,dtype=np.float32)[:,None]
 fade=min(960,len(a)//4);a[-fade:]*=np.linspace(1,0,fade,dtype=np.float32)[:,None]
 entries.append((offset,len(a)));offset+=len(a);data.append(a.astype('<f4').tobytes())
 details.append({'file':name,'source_frames':len(raw)//8,'trim_begin':start,'frames':len(a),'peak':float(np.max(np.abs(a)))})
 out=ROOT/'assets/mechanics-v17.prm'
with out.open('wb') as f:
 f.write(struct.pack('<4sIIII',b'PRM1',1,48000,len(entries),offset))
 for at,n in entries:f.write(struct.pack('<II',at,n))
 for b in data:f.write(b)
report={'recordings':len(names),'rate':48000,'channels':2,'format':'float32','frames':offset,'bytes':out.stat().st_size,'sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'adaptation':'Quiet lead-in trimmed; DC removed; 1ms onset and <=20ms end fades; pedal clips bounded to 1s. No pitch shift, denoiser or synthetic layer.','samples':details}
(ROOT/'build-support/mechanics-preparation.json').write_text(json.dumps(report,indent=2))
print('Prepared real action recordings:',len(names),'samples;',out.stat().st_size,'bytes')
