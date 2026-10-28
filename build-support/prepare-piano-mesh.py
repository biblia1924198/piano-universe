"""Offline adaptation of Bruno Oliveira's CC-BY3.0 piano to a small native mesh.
No Blender, web loader, textures or runtime JSON needed. Standard library only.
"""
import json,struct,math
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
src=ROOT/'art/piano3d/Piano-Bruno-Oliveira.glb'
b=src.read_bytes();assert b[:4]==b'glTF';size=struct.unpack_from('<I',b,12)[0];g=json.loads(b[20:20+size]);raw=b[28+size:]
def acc(i):
 a=g['accessors'][i];v=g['bufferViews'][a['bufferView']];dims={'VEC3':3,'SCALAR':1}[a['type']];code={5126:'f',5123:'H',5125:'I'}[a['componentType']];values=struct.unpack_from('<'+code*(a['count']*dims),raw,v.get('byteOffset',0)+a.get('byteOffset',0));return [values[j:j+dims] for j in range(0,len(values),dims)]
faces=[]
def face(vertices,color,tag):
 a,b,c=vertices;u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)];n=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]];length=math.sqrt(sum(x*x for x in n));n=[x/max(length,1e-12) for x in n];faces.append((vertices,n,color,tag))
colors=[(.095,.115,.11),(1,.98,.92),(.015,.022,.03),(.75,.57,.25),(.85,.67,.24),(.06,.08,.1),(.58,.62,.65)]
for p in g['meshes'][0]['primitives']:
 v=acc(p['attributes']['POSITION']);indices=[x[0] for x in acc(p['indices'])];m=p['material'];tag=4 if m in (1,2) else 3 if m==3 else 0
 if m in (2,3):continue
 for t in range(len(indices)//3):
  if m==1 and max(v[n][1] for n in indices[t*3:t*3+3])<.1:continue
  kind=1 if m==0 and 152<=t<188 else 6 if m==0 and 188<=t<200 else tag
  face([v[n] for n in indices[t*3:t*3+3]],colors[m],kind)
# Inset from the actual upstream inner rim, including the tapered bass curve.
# All interior geometry shares this footprint; changing body size scales it as a unit.
outline=[(.23,-1.18),(-.43,-1.08),(-.78,-.60),(-2.14,-.105),(-2.39,.575),(-2.39,1.10),(.23,1.10)]
def left_edge(z):
 intersections=[]
 for a,b in zip(outline,outline[1:]+outline[:1]):
  if min(a[1],b[1])<=z<=max(a[1],b[1]) and a[1]!=b[1]:
   intersections.append(a[0]+(b[0]-a[0])*(z-a[1])/(b[1]-a[1]))
 assert len(intersections)>=2,z
 return min(intersections)
for i in range(1,len(outline)-1):face([(outline[j][0],.045,outline[j][1]) for j in (0,i,i+1)],(.54,.32,.16),2)
def box(x0,y0,z0,x1,y1,z1,color,tag):
 v=[(x0,y0,z0),(x1,y0,z0),(x1,y1,z0),(x0,y1,z0),(x0,y0,z1),(x1,y0,z1),(x1,y1,z1),(x0,y1,z1)]
 for a,b,c,d in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(3,7,6,2),(0,4,7,3),(1,2,6,5)]:face([v[a],v[b],v[c]],color,tag);face([v[a],v[c],v[d]],color,tag)
# Simplified lyre/pedals, absent in the upstream decorative model.
box(.40,-1.06,-.22,.46,-.18,.22,(.05,.07,.09),5)
for z in (-.15,0,.15):box(.42,-1.14,z-.035,.90,-1.09,z+.035,(.85,.67,.24),5)

# Refined keyboard, 88 keys A0-C8, all selectable as the mechanics region.
white=[k for k in range(21,109) if k%12 not in (1,3,6,8,10)]
step=2.44/len(white);start=-1.22
for index,key in enumerate(white):
 z=start+index*step;shade=.955+(index%3)*.008
 box(.39,.028,z+.0015,.91,.075,z+step-.0015,(shade,shade,.91),4)
for key in range(21,109):
 if key%12 not in (1,3,6,8,10):continue
 index=sum(k<key for k in white);z=start+index*step
 box(.39,.075,z-step*.30,.70,.12,z+step*.30,(.018,.024,.022),4)
box(.375,.07,-1.224,.39,.088,1.224,(.36,.045,.045),4)
def cylinder(a,b,r,color,tag,sides=6):
 axis=[b[i]-a[i] for i in range(3)];length=math.sqrt(sum(x*x for x in axis));axis=[x/length for x in axis]
 ref=(0,1,0) if abs(axis[1])<.9 else (1,0,0)
 u=[axis[1]*ref[2]-axis[2]*ref[1],axis[2]*ref[0]-axis[0]*ref[2],axis[0]*ref[1]-axis[1]*ref[0]];q=math.sqrt(sum(x*x for x in u));u=[x/q for x in u]
 v=[axis[1]*u[2]-axis[2]*u[1],axis[2]*u[0]-axis[0]*u[2],axis[0]*u[1]-axis[1]*u[0]]
 rings=[]
 for centre in (a,b):rings.append([tuple(centre[j]+r*(u[j]*math.cos(2*math.pi*i/sides)+v[j]*math.sin(2*math.pi*i/sides)) for j in range(3)) for i in range(sides)])
 for i in range(sides):
  nxt=(i+1)%sides;face([rings[0][i],rings[0][nxt],rings[1][nxt]],color,tag);face([rings[0][i],rings[1][nxt],rings[1][i]],color,tag)
  face([a,rings[0][nxt],rings[0][i]],color,tag);face([b,rings[1][i],rings[1][nxt]],color,tag)
# Individual graduated strings; illustrative, not measured C5 geometry.
for i in range(88):
 z=-.97+i*1.96/87;end=max(-2.29+.92*(i/87)**1.5,left_edge(z*.86)+.07)
 cylinder((.20,.125,z),(end,.125,z*.86),.0026 if i<28 else .0018,(.78,.70,.43),3,4)
# Cast frame, braces, bridges and tuning pin clusters.
for i in range(len(outline)):
 a=outline[i];b=outline[(i+1)%len(outline)];cylinder((a[0],.098,a[1]),(b[0],.098,b[1]),.017,(.59,.43,.16),3,6)
for z in (-.72,-.10,.60):
 end=max(-2.16,left_edge(z*.80)+.09)
 cylinder((.18,.104,z),(end,.104,z*.80),.021,(.59,.43,.16),3,6)
 bridge=max(-1.43,left_edge(z-.015)+.09,left_edge(z+.015)+.09)
 box(bridge,.063,z-.015,bridge+.09,.10,z+.015,(.39,.22,.10),2)
for i in range(16):
 z=-.95+i*1.9/15;cylinder((.205,.13,z),(.205,.157,z),.014,(.50,.54,.52),3,6)
for z in (-.15,0,.15):cylinder((.43,-1.05,z),(.43,-.20,z),.013,(.69,.51,.18),5,8)
for z in (-.85,-.45,0,.45,.85):
 left=max(-2.05,left_edge(z-.012)+.06,left_edge(z+.012)+.06)
 box(left,.014,z-.012,-.10,.033,z+.012,(.46,.28,.12),2)

assert len(faces)<=4096
out=ROOT/'assets/piano3d.pr3d'
with out.open('wb') as f:
 f.write(b'PR3D'+struct.pack('<II',1,len(faces)))
 for v,n,color,tag in faces:
  for p in v:f.write(struct.pack('<10f',*p,*n,*color,float(tag)))
print(f'{len(faces)} triangles; {out.stat().st_size} bytes; no textures')
