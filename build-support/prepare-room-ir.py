#!/usr/bin/env python3
"""Reproduce the embedded measured early IR coefficients from attributed WAVs.
Requires numpy/scipy on the build computer; these are not Android dependencies.
"""
from pathlib import Path
import hashlib,json,warnings
import numpy as np
from scipy.io import wavfile
root=Path(__file__).resolve().parent.parent
source=root/'build-support/room-ir-source'
rooms=[('Room6_Speak1_Mic1.wav','Estúdio íntimo'),('Room9_Speak1_Mic1.wav','Sala de concerto'),('Room12_Speak1_Mic1.wav','Capela')]
metadata=[];arrays=[]
for filename,name in rooms:
 path=source/filename
 with warnings.catch_warnings():
  warnings.simplefilter('ignore');rate,x=wavfile.read(path)
 if rate!=48000 or x.ndim!=1:raise ValueError('Expected original mono 48kHz IR')
 x=x.astype(np.float64);peak=int(np.argmax(abs(x)));y=np.zeros(3840)
 src=x[peak:peak+3840-384].copy();src-=np.mean(src[-256:]);src[:96]=0;src[96:144]*=np.linspace(0,1,48)
 y[384:384+len(src)]=src;y[-256:]*=np.linspace(1,0,256);y*=.28/max(np.linalg.norm(y),1e-9);arrays.append(y)
 metadata.append(dict(name=name,file=filename,source_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),rate=rate,direct_peak_frame=peak,early_frames=3840,predelay_frames=384,removed_direct_ms=2,normalization='L2=0.28',format='measured mono early IR + algorithmic stereo late tail'))
header='/* Derived under CC BY 4.0: University of Rochester room impulse response dataset.\n * Credits and exact transformation: licenses/ROOM-IR-CC-BY-4.0.txt, build-support/room-ir-source/manifest.json. */\n#define PR_ROOM_IR_SAMPLES 3840\nstatic const float pr_room_ir[3][PR_ROOM_IR_SAMPLES]={\n'
for y in arrays:header+=' {\n'+''.join('  '+','.join(f'{v:.9e}f' for v in y[i:i+8])+',\n' for i in range(0,len(y),8))+' },\n'
(root/'native/room_ir.h').write_text(header+'};\n');(source/'manifest.json').write_text(json.dumps(metadata,ensure_ascii=False,indent=2))
print('Generated three attributed 80ms early IRs; original WAVs unchanged.')
