#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../native/hybrid_space.h"
static double power(const float*x,int count,double hz,int rate){double co=2*cos(6.28318530718*hz/rate),a=0,b=0;for(int i=0;i<count;i++){double v=x[i]+co*a-b;b=a;a=v;}return a*a+b*b-co*a*b;}
int main(){pr_rooms_prepare();PrHybridSpace*s=calloc(1,sizeof(*s));assert(s);
 for(int room=0;room<3;room++){
  pr_space_init(s,48000);s->roomNow[0]=0;s->roomNow[room]=1;
  for(int i=0;i<7000;i++){float input=(i==17?1.f:0)+(i==143?-.3f:0)+(i==819? .25f:0),actual=pr_space_early(s,input),want=0;int t=i-17;if(t>=0&&t<3840)want+=pr_room_ir[room][t];t=i-143;if(t>=0&&t<3840)want-=.3f*pr_room_ir[room][t];t=i-819;if(t>=0&&t<3840)want+=.25f*pr_room_ir[room][t];assert(fabsf(actual-want)<.000002f);}
 }
 float*signal=malloc(48000*sizeof(float));pr_space_init(s,48000);for(int i=0;i<72000;i++){float out=pr_space_shimmer(s,.2f*sinf(6.28318530718f*440*i/48000));if(i>=24000)signal[i-24000]=out;}
 double shifted=0,original=0;for(int hz=855;hz<=905;hz++)shifted+=power(signal,48000,hz,48000);for(int hz=415;hz<=465;hz++)original+=power(signal,48000,hz,48000);printf("shimmer power octave/base %.1f\n",shifted/(original+1e-9));assert(shifted>original*20);free(signal);
 for(int rate=8000;rate<=96000;rate=rate==8000?44100:rate==44100?48000:rate==48000?96000:96001){
  pr_space_init(s,rate);float l,r,b;pr_space_tick(s,1,&l,&r,&b);assert(l==0&&r==0&&b==0);pr_space_config(s,1,2,100,100,100,100);double peak=0,end=0;
  for(int i=0;i<rate*10;i++){float x=i<rate?4*sinf(i*.019f):0;pr_space_tick(s,x,&l,&r,&b);assert(isfinite(l)&&isfinite(r)&&fabsf(l)<20&&fabsf(r)<20&&b>=0&&b<=1.00001f);if(i==rate)peak=s->duckGain;if(i>rate*9)end+=l*l+r*r;}
  assert(peak<.2&&s->duckGain>.95&&end<.1);pr_space_config(s,0,0,100,100,100,100);for(int i=0;i<rate;i++)pr_space_tick(s,0,&l,&r,&b);assert(s->active==0);pr_space_config(s,1,0,100,100,0,100);for(int i=0;i<rate/2;i++){pr_space_tick(s,0,&l,&r,&b);assert(l==0&&r==0);} // no stale echoes after re-enable
 }
 pr_space_init(s,48000);pr_space_config(s,1,0,65,35,55,45);clock_t begin=clock();float l,r,b;for(int i=0;i<48000;i++)pr_space_tick(s,.2f*sinf(i*.04f),&l,&r,&b);printf("one active channel DSP: %.3f CPU seconds per audio second (host, not S23)\n",(clock()-begin)/(double)CLOCKS_PER_SEC);
 free(s);puts("PASS: measured IR exact partition timing, overlapping impulses, octave shimmer, bounded hot input, duck/recovery, tail decay, bypass/re-enable at 8/44.1/48/96k.");
}
