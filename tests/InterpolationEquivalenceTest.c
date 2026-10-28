#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../native/engine.h"
#ifdef NDEBUG
#error Assertions required
#endif
static unsigned reference_index(const tsf*f,const struct tsf_voice*v,long long index,unsigned pos){
 unsigned count=f->sampleCount,lo=v->region->offset,hi=v->region->end;
 if(lo>=count)lo=count?count-1:0;if(hi>count)hi=count;if(hi<=lo)hi=lo+1;
 unsigned start=v->loopStart,end=v->loopEnd;
 if(start<end&&end<count&&pos>=start&&pos<=end){
  long long length=(long long)end-start+1;
  if(index>end)index=start+(index-start)%length;
  else if(index<start&&v->qualityLooped)index=end-((start-1-index)%length);
 }
 if(index<lo)index=lo;if(index>=hi)index=hi-1;return (unsigned)index;
}
static float reference_interpolate(const tsf*f,const struct tsf_voice*v,unsigned pos,unsigned next,float t){
 if(!f->sampleCount||pos>=f->sampleCount)return 0;
 if(next>=v->region->end)next=v->region->end-1;
 if(next>=f->sampleCount)next=f->sampleCount-1;
 if(!v->sampleQuality)return f->fontSamples[pos]*(1-t)+f->fontSamples[next]*t;
 float y0=f->fontSamples[reference_index(f,v,(long long)pos-1,pos)],y1=f->fontSamples[pos];
 float y2=f->fontSamples[reference_index(f,v,(long long)pos+1,pos)],y3=f->fontSamples[reference_index(f,v,(long long)pos+2,pos)];
 float a=.5f*(y3-y0)+1.5f*(y1-y2),b=y0-2.5f*y1+2*y2-.5f*y3,c=.5f*(y2-y0);
 return ((a*t+b)*t+c)*t+y1;
}

int main(void){float data[96];for(int i=0;i<96;i++)data[i]=sinf(i*.713f);tsf f={0};struct tsf_region r={0};struct tsf_voice v={0};f.sampleCount=96;f.fontSamples=data;v.region=&r;long checks=0;
for(int quality=0;quality<2;quality++)for(int lo=0;lo<12;lo++)for(int hi=lo+1;hi<96;hi+=7)for(int ls=lo;ls<hi;ls+=5)for(int le=ls;le<hi;le+=4)for(int looped=0;looped<2;looped++){
r.offset=lo;r.end=hi;v.sampleQuality=quality;v.loopStart=ls;v.loopEnd=le;v.qualityLooped=looped;
for(unsigned pos=lo;pos<(unsigned)hi;pos++)for(int frac=0;frac<9;frac++){unsigned next=pos>=(unsigned)le&&ls<le?ls:pos+1;float t=frac*.125f;float a=reference_interpolate(&f,&v,pos,next,t),b=pr_interpolate(&f,&v,pos,next,t);assert(memcmp(&a,&b,sizeof(float))==0);checks++;}}
printf("PASS: %ld interpolation comparisons bit-identical, including loops/bounds/linear/cubic.\n",checks);return 0;}
