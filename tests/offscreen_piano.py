#!/usr/bin/env python3
"""Compile actual Android GLSL and render actual asset using surfaceless Mesa ES2.
Optional host QA: Python/Pillow + EGL. Not an Android/S23 GPU performance test.
Run from project: python tests/offscreen_piano.py STATE_TXT OUTPUT_DIRECTORY
STATE_TXT is exported by PianoVisualTest.
"""
import os,ctypes as c,struct,re,json,sys,math
from pathlib import Path
os.environ.setdefault('EGL_PLATFORM','surfaceless')
egl=c.CDLL('libEGL.so.1');P=c.c_void_p;I=c.c_int;F=c.c_float;U=c.c_uint
def ep(name,ret,args):
 f=getattr(egl,name);f.restype=ret;f.argtypes=args;return f
get=ep('eglGetDisplay',P,[P]);init=ep('eglInitialize',U,[P,c.POINTER(I),c.POINTER(I)]);choose=ep('eglChooseConfig',U,[P,c.POINTER(I),c.POINTER(P),I,c.POINTER(I)]);bind=ep('eglBindAPI',U,[U]);surface=ep('eglCreatePbufferSurface',P,[P,P,c.POINTER(I)]);context=ep('eglCreateContext',P,[P,P,P,c.POINTER(I)]);current=ep('eglMakeCurrent',U,[P,P,P,P]);proc=ep('eglGetProcAddress',P,[c.c_char_p])
d=get(None);major=I();minor=I();assert init(d,c.byref(major),c.byref(minor));assert bind(0x30a0)
config=P();count=I();attribs=(I*15)(0x3033,1,0x3040,4,0x3024,8,0x3023,8,0x3022,8,0x3021,8,0x3025,16,0x3038);assert choose(d,attribs,c.byref(config),1,c.byref(count)) and count.value
w,h=900,540;s=surface(d,config,(I*5)(0x3057,w,0x3056,h,0x3038));ctx=context(d,config,None,(I*3)(0x3098,2,0x3038));assert s and ctx and current(d,s,s,ctx)
def gl(name,ret,*args):
 address=proc(name.encode());assert address,name;return c.CFUNCTYPE(ret,*args)(address)
create=gl('glCreateShader',U,U);source=gl('glShaderSource',None,U,I,c.POINTER(c.c_char_p),c.POINTER(I));compile_=gl('glCompileShader',None,U);shaderiv=gl('glGetShaderiv',None,U,U,c.POINTER(I));shaderlog=gl('glGetShaderInfoLog',None,U,I,c.POINTER(I),c.c_char_p)
program=gl('glCreateProgram',U)();attach=gl('glAttachShader',None,U,U);link=gl('glLinkProgram',None,U);programiv=gl('glGetProgramiv',None,U,U,c.POINTER(I));use=gl('glUseProgram',None,U)
java=Path('src/com/luci/pianoroom/Piano3DView.java').read_text()
for label,kind in [('VERTEX',0x8b31),('FRAGMENT',0x8b30)]:
 shader=create(kind);code=json.loads(re.search(r'String '+label+r'=(".*?");',java).group(1)).encode();source(shader,1,(c.c_char_p*1)(code),None);compile_(shader);ok=I();shaderiv(shader,0x8b81,c.byref(ok));log=c.create_string_buffer(4096);shaderlog(shader,4096,None,log);assert ok.value,(label,log.value);attach(program,shader)
link(program);ok=I();programiv(program,0x8b82,c.byref(ok));assert ok.value;use(program)
getAttr=gl('glGetAttribLocation',I,U,c.c_char_p);getUniform=gl('glGetUniformLocation',I,U,c.c_char_p);vertex=gl('glVertexAttribPointer',None,U,I,U,U,I,P);enableAttr=gl('glEnableVertexAttribArray',None,U);uniform=gl('glUniform1f',None,I,F);uniform3=gl('glUniform3f',None,I,F,F,F);matrix=gl('glUniformMatrix4fv',None,I,I,U,c.POINTER(F));enable=gl('glEnable',None,U);viewport=gl('glViewport',None,I,I,I,I);clearColor=gl('glClearColor',None,F,F,F,F);clear=gl('glClear',None,U);draw=gl('glDrawArrays',None,U,I,I);read=gl('glReadPixels',None,I,I,I,I,U,U,P);error=gl('glGetError',U);string=gl('glGetString',c.c_char_p,U)
asset=Path('assets/piano3d.pr3d').read_bytes();magic,version,triangles=struct.unpack('<4sII',asset[:12]);assert magic==b'PR3D' and version==1;data=c.create_string_buffer(asset[12:]);base=c.addressof(data)
for name,n,offset in [('aPosition',3,0),('aNormal',3,12),('aColor',3,24),('aPart',1,36)]:
 location=getAttr(program,name.encode());assert location>=0;vertex(location,n,0x1406,0,40,P(base+offset));enableAttr(location)
viewport(0,0,w,h);enable(0x0b71);clearColor(.956,.952,.923,1);uniform3(getUniform(program,b'uEye'),-.8+math.sin(1.1)*math.cos(.48)*6.8,-.05+math.sin(.48)*6.8,math.cos(1.1)*math.cos(.48)*6.8);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
from PIL import Image
for line in Path(sys.argv[1]).read_text().splitlines():
 values=list(map(float,line.split()));lid,inside,angle,scale=values[:4];mat=values[4:];assert len(mat)==16;clear(0x4000|0x0100);matrix(getUniform(program,b'uMatrix'),1,0,(F*16)(*mat));
 for name,value in [('uAngle',angle),('uScale',scale),('uSelected',2 if inside else 1),('uInterior',inside)]:uniform(getUniform(program,name.encode()),value)
 draw(4,0,triangles*3);pixels=c.create_string_buffer(w*h*4);read(0,0,w,h,0x1908,0x1401,pixels);assert error()==0;Image.frombytes('RGBA',(w,h),pixels.raw).transpose(Image.Transpose.FLIP_TOP_BOTTOM).save(out/f'piano-lid{int(lid)}-interior{int(inside)}.png')
print('PASS: actual ES2 vertex/fragment compiled and linked; six actual mesh renders, no GL error; renderer',string(0x1f01).decode())
