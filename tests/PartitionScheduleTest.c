#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../native/hybrid_space.h"
#include "../native/piano_resonance.h"
static float reference_early(PrHybridSpace*s,float input){
 float early=s->pending[s->pos];s->dry[s->pos]=input;if(pr_head_any)for(int k=0;k<128;k++){float h=0;for(int j=0;j<3;j++)h+=pr_rooms[j][s->ri].head[k]*s->roomNow[j];early+=s->dry[(s->pos-k+128)&127]*h;}
 if(++s->pos==128){
  s->pos=0;s->hist=(s->hist+1)&63;PrComplex*in=s->history[s->hist];memset(in,0,256*sizeof(*in));for(int k=0;k<128;k++)in[k].r=s->dry[k];pr_fft(in,0);memset(s->scratch,0,sizeof(s->scratch));
  for(int j=0;j<3;j++){if(s->roomNow[j]<.00001f)continue;PrRoomPlan*p=&pr_rooms[j][s->ri];for(int part=1;part<p->count;part++){PrComplex*x=s->history[(s->hist-(part-1)+64)&63],*h=p->h[part];float a=s->roomNow[j];for(int k=0;k<256;k++){s->scratch[k].r+=a*(x[k].r*h[k].r-x[k].i*h[k].i);s->scratch[k].i+=a*(x[k].r*h[k].i+x[k].i*h[k].r);}}}
  pr_fft(s->scratch,1);for(int k=0;k<128;k++){s->pending[k]=s->scratch[k].r+s->overlap[k];s->overlap[k]=s->scratch[k+128].r;}
 }
 return early;
}
static void reference_resonance(PrPianoResonance*r,float input,float amount,float*outL,float*outR){
 *outL=*outR=0;
 if(amount<=.00001f){if(r->active)pr_res_reset(r);return;}r->active=1;
 /* Bound pathological sources locally; the output limiter remains downstream. */
 if(!isfinite(input))input=0;input=fmaxf(-8,fminf(8,input));
 float drive=input-r->dcX+r->dcPole*r->dcY;r->dcX=input;r->dcY=drive;
 float l=0,rr=0;
 for(int k=0;k<PR_RESONANCE_MODES;k++){
  float loss=r->loss[k]+(r->targetLoss[k]-r->loss[k])*r->smooth;r->loss[k]=loss;float radius=1-loss;
  float coupling=r->coupling[k]+(r->couplingTarget[k]-r->coupling[k])*r->smooth;r->coupling[k]=coupling;
  float x=radius*(r->co[k]*r->x[k]-r->si[k]*r->y[k])+drive*coupling;
  float y=radius*(r->si[k]*r->x[k]+r->co[k]*r->y[k]);
  /* Flush sub-audible state to avoid denormal CPU spikes during long tails. */
  r->x[k]=fabsf(x)<1e-20f?0:x;r->y[k]=fabsf(y)<1e-20f?0:y;
  l+=x*r->left[k];rr+=x*(1-r->left[k]);
 }*outL=l*amount;*outR=rr*amount;
}
int main(void){pr_rooms_prepare();pr_dsp_denormals();PrHybridSpace*a=calloc(1,sizeof(*a)),*b=calloc(1,sizeof(*b));assert(a&&b);float maximum=0;double error=0,power=0;
 for(int rate=44100;rate<=96000;rate=rate==44100?48000:rate==48000?96000:96001){
  pr_space_init(a,rate);pr_space_init(b,rate);int target=0;
  for(int i=0;i<90000;i++){if(i==17||i==255||i==7891||i==31358||i==61057)target=(target+1)%3;
   for(int j=0;j<3;j++){a->roomTarget[j]=j==target;a->roomNow[j]+=(a->roomTarget[j]-a->roomNow[j])/(rate*.02f);b->roomNow[j]=a->roomNow[j];}
   float input=.35f*sinf(i*.081f)+.13f*sinf(i*.019f);if(i%297==0)input+=.7f;float x=pr_space_early(a,input),y=reference_early(b,input),d=fabsf(x-y);if(d>maximum)maximum=d;error+=d*d;power+=y*y;assert(d<.000004f);
  }
  pr_space_init(a,rate);a->roomNow[0]=a->roomTarget[0]=0;a->roomNow[2]=a->roomTarget[2]=1;
  for(int i=0;i<8192;i++)pr_space_early(a,i==17?1:0);assert(a->tailJobsMax<=1&&a->boundaryJobsMax<=1); // never dozens of partitions at a boundary
 }
 printf("distributed IR max difference %.9g / relative RMS %.9g\n",maximum,sqrt(error/(power+1e-30)));assert(sqrt(error/(power+1e-30))<.000003);
 float resMaximum=0;for(int rate=44100;rate<=96000;rate=rate==44100?48000:rate==48000?96000:96001){PrPianoResonance r,t;pr_res_init(&r,rate);t=r;unsigned short held[128]={0};
  for(int i=0;i<120000;i++){if(i%256==0){held[48+(i/256)%40]^=1;int pedal=(i/15000)%3==0?0:(i/15000)%3==1?55:127;pr_res_prepare(&r,held,0,pedal,1);pr_res_prepare(&t,held,0,pedal,1);}float input=.4f*sinf(i*.057f),x,y,u,v;pr_res_tick(&r,input,.55f,&x,&y);reference_resonance(&t,input,.55f,&u,&v);float d=fmaxf(fabsf(x-u),fabsf(y-v));if(d>resMaximum)resMaximum=d;assert(d<.000003f);}
 }
 printf("SIMD resonance max difference %.9g\n",resMaximum);free(a);free(b);puts("PASS: reference equivalence, room changes inside blocks, 44.1/48/96k, causal IR timing, bounded scheduled partition work and all 264 SIMD modes preserved.");return 0;
}
