/* Recorded key/hammer-return and pedal action; see asset credit.
 * Real captures are loaded before playback; legacy synthesis only for hosts
 * without the packaged bank. Both paths never consume musical polyphony. */
#ifndef PR_PIANO_MECHANICS
#define PR_PIANO_MECHANICS
#include <math.h>
#include <string.h>
#include "mechanical_samples.h"
typedef struct {float level,decay,co,si,x,y,noise,pan;unsigned rng;int left;} PrMechanicalVoice;
typedef struct {PrMechanicalVoice v[8];int hammer,key,pedal;unsigned serial;PrRecordedVoice recorded[PR_MECH_VOICES];unsigned downTake,upTake;int recordedActive;} PrMechanics;
static void pr_mech_clear(PrMechanics*m){memset(m->v,0,sizeof(m->v));memset(m->recorded,0,sizeof(m->recorded));m->recordedActive=0;}
static void pr_mech_event(PrMechanics*m,int kind,int key,int velocity,float age,int rate){
 PrMechanicalBank*bank=pr_recorded_bank();
 if(bank){
  /* rel1..88 capture the SAME key/hammer-return action: play once on key-up,
     including when sustain is held. They are not hammer-strike recordings. */
  if(kind==0)return;int amount=kind==1?(m->hammer>m->key?m->hammer:m->key):m->pedal;
  if(amount<=0||rate<8000)return;int sample;if(kind==1){if(key<21||key>108)return;sample=key-21;}else sample=velocity>=0?88+(m->downTake++&1):90+(m->upTake++&1);
  int slot=0;float quietest=1e9f;for(int i=0;i<PR_MECH_VOICES;i++){PrRecordedVoice*v=&m->recorded[i];if(!v->sample&&!v->old){slot=i;break;}float remaining=v->sample?fmaxf(0,v->sample->frames-v->position)/v->sample->frames:0,score=v->gain*remaining;if(score<quietest){quietest=score;slot=i;}}
  float strength=kind==1?.55f+.45f*powf(fmaxf(0,fminf(127,velocity))/127.f,.8f):1;
  float gain=amount/100.f*(kind==1?.045f:.25f)*strength;
  pr_mech_record(&m->recorded[slot],&bank->sample[sample],rate,gain);m->recordedActive=1;return;
 }
 if(velocity<0)velocity=-velocity;
 int amount=kind==0?m->hammer:kind==1?m->key:m->pedal;if(amount<=0||rate<8000)return;
 int slot=0;for(int i=0;i<8;i++){if(!m->v[i].left){slot=i;break;}if(m->v[i].level<m->v[slot].level)slot=i;}
 PrMechanicalVoice*v=&m->v[slot];float strength=kind==0?powf(velocity/127.f,1.7f):kind==1?.45f+.35f*expf(-age*2):.35f+velocity/254.f;
 float seconds=kind==0?.026f:kind==1?.035f:.065f,hz=kind==2?105:kind==1?430:650+key*3;
 v->level=amount*.00016f*strength;v->decay=expf(-7.f/(rate*seconds));v->left=(int)(seconds*rate);v->x=1;v->y=v->noise=0;v->co=cosf(6.2831853f*hz/rate);v->si=sinf(6.2831853f*hz/rate);v->pan=fmaxf(-.3f,fminf(.3f,(key-64)/150.f));v->rng=(++m->serial)*2654435761u+0x93a871u+(unsigned)key*731u+(unsigned)velocity*7919u;
}
static void pr_mech_tick(PrMechanics*m,float*l,float*r){
 *l=*r=0;if(pr_recorded_bank()){if(!m->recordedActive)return;int active=0;for(int i=0;i<PR_MECH_VOICES;i++)active|=pr_mech_record_tick(&m->recorded[i],l,r);m->recordedActive=active;return;}for(int i=0;i<8;i++){PrMechanicalVoice*v=&m->v[i];if(!v->left)continue;v->rng^=v->rng<<13;v->rng^=v->rng>>17;v->rng^=v->rng<<5;float n=(v->rng&65535)/32768.f-1;v->noise+=.3f*(n-v->noise);float x=v->co*v->x-v->si*v->y;v->y=v->si*v->x+v->co*v->y;v->x=x;float a=v->level*(.7f*x+.3f*v->noise);*l+=a*(1-v->pan);*r+=a*(1+v->pan);v->level*=v->decay;v->left--;}
}
#endif
