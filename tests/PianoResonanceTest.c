#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../native/piano_resonance.h"
static double exercise(int rate,int pedal,int heldKey,int transpose,int releasePedal){
 PrPianoResonance r;pr_res_init(&r,rate);unsigned short held[128]={0};if(heldKey>=0)held[heldKey]=1;pr_res_prepare(&r,held,transpose,pedal,1);
 double energy=0;float l,rr;
 for(int i=0;i<rate*2;i++){
  if(releasePedal&&i==rate){memset(held,0,sizeof(held));pr_res_prepare(&r,held,transpose,0,1);}
  float sample=i<rate?.6f*sinf((float)(6.283185307179586*440*i/rate)):0;
  pr_res_tick(&r,sample,1,&l,&rr);assert(isfinite(l)&&isfinite(rr)&&fabsf(l)<2&&fabsf(rr)<2);
  if(i>rate+rate/4)energy+=l*l+rr*rr;
 }return energy/rate;
}
int main(void){
 for(int rate=44100;rate<=96000;rate=rate==44100?48000:rate==48000?96000:96001){
  double closed=exercise(rate,0,-1,0,0),half=exercise(rate,90,-1,0,0),open=exercise(rate,127,-1,0,0),key=exercise(rate,0,69,0,0),trans=exercise(rate,0,57,12,0),damped=exercise(rate,127,-1,0,1);
  printf("rate=%d tail energy closed %.8g half %.8g open %.8g held %.8g damped %.8g\n",rate,closed,half,open,key,damped);
  assert(open>half*2&&half>closed*2&&key>closed*100&&damped<open*.02);assert(fabs(trans-key)<1e-10);
 }
 PrPianoResonance r;pr_res_init(&r,48000);unsigned short held[128]={0};float l,rr;
 pr_res_prepare(&r,held,0,127,1);for(int i=0;i<48000;i++)pr_res_tick(&r,.5f,1,&l,&rr);
 pr_res_tick(&r,1,0,&l,&rr);assert(l==0&&rr==0&&!r.active);for(int i=0;i<PR_RESONANCE_MODES;i++)assert(r.x[i]==0&&r.y[i]==0);
 unsigned rng=1;clock_t start=clock();double sum=0;
 // One minute of rapid controller changes, all held notes and hot input.
 for(int i=0;i<48000*60;i++){
  if(i%96==0){rng=rng*1664525u+1013904223u;held[(rng>>8)&127]^=1;pr_res_prepare(&r,held,0,(rng>>16)&127,1);}
  rng=rng*1664525u+1013904223u;float input=((int)(rng>>16)-32768)/4096.f;
  pr_res_tick(&r,input,1,&l,&rr);assert(isfinite(l)&&isfinite(rr)&&fabsf(l)<4&&fabsf(rr)<4);sum+=l*l+rr*rr;
 }
 printf("PASS: resonant selectivity, damping/half pedal, transpose, rates, zero-wet bypass, finite hot-input stress; %.3fs host CPU / 60s audio, energy %.5g (not phone latency).\n",(clock()-start)/(double)CLOCKS_PER_SEC,sum);
}
