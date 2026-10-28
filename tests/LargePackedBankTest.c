#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include "../native/engine.h"
int main(int argc,char**argv){assert(argc==2||argc==3);int version=argc==3?4:3;long long header=version==4?324LL:196LL;char*path=argv[1];unsigned count=805306368u;/* exactly 3 GiB float payload, sparse fixture */
 unsigned offset=600000000u;int v[18]={60,60,1,127,48000,(int)offset,(int)offset+4096,(int)offset,(int)offset+4096,0,60,100,0,0,1,1,0,0};float a[6]={0,0,0,0,1,.1f},x[5]={0};int curve=0;unsigned one=1;char name[64]={0};memcpy(name,"Large piano",11);int fd=open(path,O_CREAT|O_TRUNC|O_RDWR,0600);assert(fd>=0);assert(write(fd,version==4?"PRB4":"PRB3",4)==4);assert(write(fd,&one,4)==(long)4);assert(write(fd,&count,4)==(long)4);assert(write(fd,name,64)==(long)64);if(version==4){unsigned char cc[128]={0};cc[7]=128;assert(write(fd,cc,128)==128);}assert(write(fd,v,sizeof(v))==(long)sizeof(v));assert(write(fd,a,sizeof(a))==(long)sizeof(a));assert(write(fd,x,sizeof(x))==(long)sizeof(x));assert(write(fd,&curve,4)==(long)4);assert(ftruncate(fd,header+(long long)count*4)==0);float pcm[4097];for(int i=0;i<4097;i++)pcm[i]=.5f*sinf(i*.0863938f);assert(pwrite(fd,pcm,sizeof(pcm),header+(long long)offset*4)==sizeof(pcm));close(fd);
 tsf*b=load_font(path);assert(b&&b->prMapping&&b->sampleCount==count&&b->prMappingBytes>2147483648ULL);tsf*c=copy_font(b,64);assert(c&&c->fontSamples==b->fontSamples);tsf_close(b);tsf_channel_set_presetindex(c,0,0);smart_note(c,60,127);float out[512];tsf_render_float(c,out,256,0);float peak=0;for(int i=0;i<512;i++){assert(isfinite(out[i]));if(fabsf(out[i])>peak)peak=fabsf(out[i]);}assert(peak>.01f);tsf_close(c);
 fd=open(path,O_RDWR);assert(fd>=0);float nan=NAN;assert(pwrite(fd,&nan,4,header+((long long)count-1)*4)==4);close(fd);assert(!load_font(path));unlink(path);printf("PASS: real 3-GiB PRB%d sparse file, sample offsets beyond signed 2-GiB, audible render, shared mapped ownership, last-page corruption rejected.\n",version);}
