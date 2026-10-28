#!/usr/bin/env python3
"""Check the entire interior against independently sourced GLB inner-rim vertices."""
import json, math, struct
from pathlib import Path

root=Path(__file__).resolve().parent.parent
b=(root/'art/piano3d/Piano-Bruno-Oliveira.glb').read_bytes()
n=struct.unpack_from('<I',b,12)[0];g=json.loads(b[20:20+n]);raw=b[28+n:]
primitive=next(p for p in g['meshes'][0]['primitives'] if p['material']==0)
a=g['accessors'][primitive['attributes']['POSITION']];v=g['bufferViews'][a['bufferView']]
vertices=list(struct.iter_unpack('<3f',raw[v['byteOffset']:v['byteOffset']+a['count']*12]))
# Ordered inner corners of this upstream model; resolve exact values from the GLB.
references=[(.27516,.010,-1.30839),(-.52068,.00385,-1.19056),(-.83199,.00743,-.69087),(-2.20016,.00729,-.19246),(-2.45788,.01039,.56127),(-2.45788,.01039,1.16127),(.28212,.01039,1.16127)]
rim=[]
for reference in references:
 p=min(vertices,key=lambda p:sum((x-y)**2 for x,y in zip(p,reference)))
 assert math.dist(p,reference)<.0001,reference
 rim.append((p[0],p[2]))

def inside(x,z,outline):
 hit=False
 for a,b in zip(outline,outline[1:]+outline[:1]):
  if (a[1]>z)!=(b[1]>z) and x<a[0]+(b[0]-a[0])*(z-a[1])/(b[1]-a[1]):hit=not hit
 return hit

packed=(root/'assets/piano3d.pr3d').read_bytes();triangles=struct.unpack_from('<I',packed,8)[0]
count=0
for t in range(triangles):
 values=struct.unpack_from('<30f',packed,12+t*120);tag=int(values[9])
 if tag not in (2,3):continue
 pts=[values[i:i+3] for i in (0,10,20)]
 # Include edge/interior points so a face cannot cross the concave rim.
 for i in range(7):
  for j in range(7-i):
   weights=(i/6,j/6,1-(i+j)/6)
   x=sum(p[0]*w for p,w in zip(pts,weights));z=sum(p[2]*w for p,w in zip(pts,weights))
   assert inside(x,z,rim),(t,tag,x,z)
   for scale in (.88,1,1.12):assert inside(x*scale,z*scale,[(a*scale,b*scale) for a,b in rim])
 count+=1
assert count>1600
print(f'PASS: {count} board/string/frame triangles contained within the upstream inner rim, including face edges and all body sizes')
