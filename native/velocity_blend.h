/* Immutable adjacency metadata is built before a bank is published.
 * Explicit SFZ fades take priority; automatic fades only join compatible,
 * contiguous, non-overlapping velocity zones. Stereo sides stay paired. */
#ifndef PR_VELOCITY_BLEND
#define PR_VELOCITY_BLEND
#include <math.h>
static void pr_blend_prepare(tsf*f){
 for(int p=0;p<f->presetNum;p++){
  struct tsf_preset*s=&f->presets[p];
  for(int i=0;i<s->regionNum;i++){
   struct tsf_region*a=&s->regions[i];a->prBlendLo=a->prBlendHi=0;
   if(a->group||a->prTrigger||a->prSeq>1||a->prXfinHi||a->prXfoutHi)continue;
   int overlap=0;
   for(int j=0;j<s->regionNum;j++)if(i!=j){
    struct tsf_region*b=&s->regions[j];
    if(b->prTrigger||b->prSeq>1||b->prXfinHi||b->prXfoutHi||a->lokey!=b->lokey||a->hikey!=b->hikey||a->pitch_keycenter!=b->pitch_keycenter||a->pitch_keytrack!=b->pitch_keytrack||a->transpose!=b->transpose||a->tune!=b->tune||a->pan!=b->pan||a->group!=b->group)continue;
    if(a->lovel<=b->hivel&&b->lovel<=a->hivel){overlap=1;continue;}
    int half=(a->hivel-a->lovel+1)/2,bhalf=(b->hivel-b->lovel+1)/2;if(bhalf<half)half=bhalf;if(half>8)half=8;
    if(b->hivel+1==a->lovel)a->prBlendLo=half;
    if(a->hivel+1==b->lovel)a->prBlendHi=half;
   }
   if(overlap)a->prBlendLo=a->prBlendHi=0;
  }
 }
}
static float pr_region_gain(tsf*f,struct tsf_region*r,int key,int velocity){
 if(key<r->lokey||key>r->hikey)return 0;
 int lo=0,hi=0;if(!r->prTrigger){lo=f->prBlend<r->prBlendLo?f->prBlend:r->prBlendLo;hi=f->prBlend<r->prBlendHi?f->prBlend:r->prBlendHi;}
 if(velocity<r->lovel-lo||velocity>r->hivel+hi)return 0;
 float gain=1;
 if(lo&&velocity<r->lovel+lo)gain*=fmaxf(0,fminf(1,(velocity-(r->lovel-.5f-lo))/(2*lo)));
 if(hi&&velocity>r->hivel-hi)gain*=fmaxf(0,fminf(1,(r->hivel+.5f+hi-velocity)/(2*hi)));
 if(f->prBlendCurve!=1)gain=sqrtf(gain);
 float x=1;if(r->prXfinHi>r->prXfinLo)x*=fmaxf(0,fminf(1,(velocity-r->prXfinLo)/(r->prXfinHi-r->prXfinLo)));
 if(r->prXfoutHi>r->prXfoutLo)x*=fmaxf(0,fminf(1,(r->prXfoutHi-velocity)/(r->prXfoutHi-r->prXfoutLo)));
 return gain*(r->prCurve==1?x:sqrtf(x));
}
#endif
