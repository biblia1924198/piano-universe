#!/usr/bin/env python3
"""Lossless FLAC storage for SF2's 16-bit sample pool; preserve every SF2 byte.
The app restores SF2 on its initial-load worker and verifies SHA-256. Requires
ffmpeg only at build time. Musical sample rate/metadata are never changed.
"""
from pathlib import Path
import hashlib,json,struct,subprocess,tempfile,os
ROOT=Path(__file__).resolve().parent.parent
report=[]
for font in sorted(ROOT.joinpath('assets').glob('*.sf2')):
    raw=font.read_bytes();assert raw[:4]==b'RIFF' and raw[8:12]==b'sfbk'
    at=12;found=None
    while at+8<=len(raw):
        kind,n=struct.unpack_from('<4sI',raw,at)
        if kind==b'LIST' and raw[at+8:at+12]==b'sdta':
            pos=at+12;end=at+8+n
            while pos+8<=end:
                k,size=struct.unpack_from('<4sI',raw,pos)
                if k==b'smpl':found=(pos+8,size);break
                pos+=8+size+(size&1)
        if found:break
        at+=8+n+(n&1)
    assert found and found[1]%2==0
    offset,size=found;tail=len(raw)-offset-size;digest=hashlib.sha256(raw).digest()
    dest=font.with_name(font.name+'.prsf')
    with tempfile.TemporaryDirectory(prefix='pr-bank-') as folder:
        pcm=Path(folder)/'samples.pcm';flac=Path(folder)/'samples.flac';pcm.write_bytes(raw[offset:offset+size])
        subprocess.run([os.environ.get('PR_FFMPEG','ffmpeg'),'-v','error','-y','-f','s16le','-ar','48000','-ac','1','-i',str(pcm),'-c:a','flac','-compression_level','8',str(flac)],check=True)
        packed=flac.read_bytes();temp=dest.with_suffix('.partial')
        with temp.open('wb') as out:
            out.write(struct.pack('>4sI5Q32s',b'PRSF',1,len(raw),offset,size,len(packed),tail,digest));out.write(raw[:offset]);out.write(packed);out.write(raw[offset+size:]);out.flush();os.fsync(out.fileno())
        temp.replace(dest)
    item={'file':font.name,'originalBytes':len(raw),'originalSha256':digest.hex(),'packedBytes':dest.stat().st_size,'packedSha256':hashlib.sha256(dest.read_bytes()).hexdigest()};report.append(item);print(json.dumps(item),flush=True)
ROOT.joinpath('build-support/bundled-banks-lossless.json').write_text(json.dumps(report,indent=2)+'\n')
