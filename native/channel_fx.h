/* Piano Room: causal channel DSP. No extra playback queue, allocations or locks. */
#ifndef PR_CHANNEL_FX
#define PR_CHANNEL_FX
#include "piano_resonance.h"
#include "piano_mechanics.h"
#include "piano_body.h"
#include "hybrid_space.h"
typedef struct {
 int sendOnly;float pan,chorus,delay,delayMs,feedback,reverb,resonance;int pedalMode;
 PrHybridSpace space;PrPianoBody body;PrMechanics mechanics;float profile[792];int profileSet,profileCount,harmonics;PrToneEq eq;float line[2][48000],chorusLine[2][2048];int di,chi;
 float comb[2][4][3000],damp[2][4],ap[2][2][800];int ci[2][4],ai[2][2];
 float phase;PrPianoResonance strings;int rate,pedal,ever;
 float gain,energy,panNow,chorusNow,delayNow,reverbNow,resonanceNow;unsigned idle;int fade;unsigned short held[128];int transpose;unsigned owner;
} PrChannelFx;
static PrChannelFx* part_new(int rate){PrChannelFx*p=calloc(1,sizeof(*p));if(p){p->delayMs=280;p->feedback=.3f;p->reverb=p->reverbNow=1;p->gain=-1;p->rate=rate;pr_rooms_prepare();pr_space_init(&p->space,rate);pr_body_init(&p->body,rate);pr_res_init(&p->strings,rate);if(p->profileSet){pr_res_profile(&p->strings,p->profile);if(p->harmonics)pr_res_harmonics(&p->strings,p->profile,p->profileCount);}pr_mech_clear(&p->mechanics);}return p;}
static void part_rate(PrChannelFx*p,int rate){if(!p||p->rate==rate)return;p->rate=rate;PrHybridSpace *s=&p->space;int en=s->enabled,room=s->room,amount=s->amount,texture=s->texture,duck=s->duck,decay=s->decay;pr_space_init(s,rate);pr_space_config(s,en,room,amount,texture,duck,decay);PrPianoBody oldBody=p->body;pr_body_init(&p->body,rate);pr_body_prepare(&p->body,oldBody.enabled,oldBody.body,oldBody.board,oldBody.size,oldBody.decay,oldBody.lid,rate);memcpy(p->body.co,p->body.targetCo,sizeof(p->body.co));memcpy(p->body.si,p->body.targetSi,sizeof(p->body.si));memcpy(p->body.loss,p->body.targetLoss,sizeof(p->body.loss));pr_eq_reset(&p->eq,rate);pr_res_init(&p->strings,rate);if(p->profileSet){pr_res_profile(&p->strings,p->profile);if(p->harmonics)pr_res_harmonics(&p->strings,p->profile,p->profileCount);}pr_mech_clear(&p->mechanics);}
static void part_reset(PrChannelFx*p){if(!p)return;/* Keep user controls; delay memory is reset only on explicit panic. */
 memset(p->line,0,sizeof(p->line));memset(p->chorusLine,0,sizeof(p->chorusLine));memset(p->comb,0,sizeof(p->comb));memset(p->damp,0,sizeof(p->damp));memset(p->ap,0,sizeof(p->ap));memset(p->ci,0,sizeof(p->ci));memset(p->ai,0,sizeof(p->ai));pr_space_clear(&p->space);pr_res_reset(&p->strings);pr_body_reset(&p->body);pr_mech_clear(&p->mechanics);memset(p->held,0,sizeof(p->held));p->di=p->chi=p->pedal=p->idle=p->fade=0;p->energy=0;p->ever=0;pr_eq_reset(&p->eq,p->rate);}
static void part_process(PrChannelFx*p,float*b,float*mix,int n,float target,float master,float wet,float feedback){
 const int lens[4]={1557,1617,1491,1422},al[2]={225,556};pr_eq_process(&p->eq,b,n);if(p->gain<0)p->gain=target;
 int delay=(int)(p->delayMs*p->rate/1000);if(delay<1)delay=1;if(delay>47999)delay=47999;int fading=p->fade>0;float peak=0,smooth=1.0f-expf(-1.0f/(p->rate*.005f));
 if(p->resonanceNow>.00001f||p->resonance>.00001f)pr_res_prepare(&p->strings,p->held,p->transpose,p->pedal,p->pedalMode);
 for(int i=0;i<n;i++){
  p->gain+=(target-p->gain)*smooth;p->panNow+=(p->pan-p->panNow)*smooth;p->chorusNow+=(p->chorus-p->chorusNow)*smooth;p->delayNow+=(p->delay-p->delayNow)*smooth;p->reverbNow+=(p->reverb-p->reverbNow)*smooth;p->resonanceNow+=(p->resonance-p->resonanceNow)*smooth;float fade=1;if(fading){fade=p->fade>0?p->fade/(float)(p->rate/10):0;if(fade>1)fade=1;if(p->fade)p->fade--;}
  float excitation=(b[i*2]+b[i*2+1])*.5f,resL=0,resR=0;if(b[i*2]>.000001f||b[i*2]<-.000001f||b[i*2+1]>.000001f||b[i*2+1]<-.000001f)p->ever=1;
  pr_res_tick(&p->strings,excitation,p->resonanceNow,&resL,&resR);float mechL,mechR;pr_mech_tick(&p->mechanics,&mechL,&mechR);resL+=mechL;resR+=mechR;
  float dryL=b[i*2]+resL,dryR=b[i*2+1]+resR;pr_body_tick(&p->body,&dryL,&dryR);
  p->phase+=6.2831853f*.35f/p->rate;if(p->phase>6.2831853f)p->phase-=6.2831853f;
  float roomL,roomR,roomBlend;pr_space_tick(&p->space,(dryL+dryR)*.5f*p->gain,&roomL,&roomR,&roomBlend);
  for(int c=0;c<2;c++){
   float x=(c?dryR:dryL)*p->gain;float pan=c==0?(p->panNow>0?1-p->panNow:1):(p->panNow<0?1+p->panNow:1);x*=pan;
   p->chorusLine[c][p->chi]=x;
   /* Fractional read avoids one-sample jumps of the modulated chorus tap. */
   float lag=(.012f+.004f*sinf(p->phase+c*1.5f))*p->rate;if(lag>2047)lag=2047;
   int whole=(int)lag,cp=(p->chi-whole+2048)&2047;float fraction=lag-whole;
   float ch=p->chorusLine[c][cp]*(1-fraction)+p->chorusLine[c][(cp-1)&2047]*fraction;
   x+=ch*p->chorusNow*.5f;int dp=(p->di-delay+48000)%48000;float echo=p->line[c][dp];p->line[c][p->di]=x+echo*p->feedback;
   float rv=0;for(int k=0;k<4;k++){int q=p->ci[c][k],len=lens[k]+c*23;float y=p->comb[c][k][q];p->damp[c][k]=y*.65f+p->damp[c][k]*.35f;p->comb[c][k][q]=x*.22f+p->damp[c][k]*feedback;p->ci[c][k]=(q+1)%len;rv+=y*.25f;}
   for(int k=0;k<2;k++){int q=p->ai[c][k],len=al[k]+c*17;float y=p->ap[c][k][q],v=rv;rv=y-v;p->ap[c][k][q]=v+y*.5f;p->ai[c][k]=(q+1)%len;}
   rv=rv*(1-roomBlend)+(c?roomR:roomL)*roomBlend;
   float audible=p->sendOnly?((c?resR:resL)*p->gain*pan+ch*p->chorusNow*.5f):x;
   float y=(audible+echo*p->delayNow+rv*wet*p->reverbNow*1.3f)*master*fade;mix[i*2+c]+=y;float a=y<0?-y:y;if(a>peak)peak=a;
  }p->di=(p->di+1)%48000;p->chi=(p->chi+1)&2047;
 }p->energy=peak;
}
/* Partial damping preserves amplitude and never revives voice-stealing fades.
   Release age is counted by the renderer, independently of envelope length. */
static void part_partial(tsf*b,struct tsf_voice*v,int value){
 if(value<=32&&v->prFromPedal&&b->prPedalFade>0)return;
 int base=tsf_voice_envelope_release_samples(&v->ampenv,b->outSampleRate);
 float position=value/112.f;
 int duration=base+(int)(b->outSampleRate*1.5f*position*position);
 int remaining=duration-(int)v->prReleaseFrames,min=(int)(b->outSampleRate*.005f);
 if(remaining<min)remaining=min;
 v->ampenv.samplesUntilNextSegment=remaining;v->ampenv.slope=expf(-9.226f/remaining);v->ampenv.segmentIsExponential=1;v->prPartialRelease=value>0;
}
static void part_release(tsf*b,PrChannelFx*p,int key){
 unsigned group=0;int found=0;float velocity=0,age=0,energy=0;int matchedVoices=0,sustainAllowed=0;int fromHistory=bank_take_attack(b,key,&group,&velocity,&age);if(fromHistory){found=1;energy=1;b->prOffValid=1;b->prOffGroup=group;}
 if(!fromHistory)for(int i=0;i<b->voiceNum;i++){struct tsf_voice*v=&b->voices[i];if(v->playingPreset>=0&&v->playingChannel==0&&v->playingKey==key&&!v->heldSustain&&!v->prKeyReleased&&v->ampenv.segment<TSF_SEGMENT_RELEASE&&(!b->prBank||!v->region->prTrigger)&&(!found||v->playIndex<group)){group=v->playIndex;found=1;}}
 if(found)for(int i=0;i<b->voiceNum;i++){struct tsf_voice*v=&b->voices[i];if(v->playingPreset>=0&&v->playIndex==group){v->prKeyReleased=1;if(!(v->region->prSfz5&&(v->region->prSfzFlags&1)))sustainAllowed=1;if(!matchedVoices++){energy=0;}if(!fromHistory){velocity=v->ampenv.midiVelocity;age=v->prAgeSeconds;}if(v->ampenv.level>energy)energy=v->ampenv.level;if(b->prContext&&!pr_sfz_authored_pedal(v->region))v->ampenv.parameters.release*= (.8f+.2f*expf(-age/2))*(.9f+.1f*velocity/127.f);}}
 if(p&&found)pr_mech_event(&p->mechanics,1,key,(int)velocity,age,p->rate);
 if(b->prBank&&found){bank_trigger_context(b,key,2,velocity,age,1);if(b->channels&&((b->prExtData&&b->channels->channels[0].prCC[64]>32)||b->channels->channels[0].sustain&&(!b->prSfz5||sustainAllowed)))bank_defer(b,key,velocity,age,energy);else bank_trigger_context(b,key,1,velocity,age,energy);}
 tsf_channel_note_off(b,0,key);b->prOffValid=0;if(!found||!p||!p->pedalMode||p->pedal<=0||p->pedal>=112)return;
 for(int i=0;i<b->voiceNum;i++){struct tsf_voice*v=&b->voices[i];if(v->playingPreset>=0&&v->playIndex==group&&!pr_sfz_authored_pedal(v->region)&&v->prPedalRelease&&v->ampenv.segment==TSF_SEGMENT_RELEASE)part_partial(b,v,p->pedal);}
}
static void part_pedal(tsf*b,PrChannelFx*p,int value){
 int before=p?p->pedal:0,beforeFull=b->channels&&b->channels->channels[0].sustain;if(p){p->pedal=value;if((before>=64)!=(value>=64))pr_mech_event(&p->mechanics,2,64,(value-before),0,p->rate);}
 pr_sfz_controller(b,0,64,value);
 int full=value>=(p&&p->pedalMode?112:64);tsf_channel_set_sustain(b,0,full);
 if(p&&p->pedalMode&&!full)for(int i=0;i<b->voiceNum;i++){
  struct tsf_voice*v=&b->voices[i];if(v->playingPreset>=0&&!pr_sfz_authored_pedal(v->region)&&v->prPedalRelease&&v->ampenv.segment==TSF_SEGMENT_RELEASE&&(v->prPartialRelease||beforeFull&&v->heldSustain&&value>0))part_partial(b,v,value);
 }
 if(b->prBank&&(b->prExtData?value<=32:!full))for(int key=0;key<128;key++)if(b->prRelease[key]){bank_release_pending(b,key);}
 if(p&&p->pedalMode&&value>=112&&before<112)for(int i=0;i<b->voiceNum;i++){
  struct tsf_voice*v=&b->voices[i];if(v->playingPreset<0||!v->prPedalRelease||v->ampenv.segment!=TSF_SEGMENT_RELEASE||v->ampenv.level<.001f||v->prReleaseFrames>=b->outSampleRate*.09f)continue;
  if(b->prBank&&(v->region->prTrigger||v->region->prOneShot||pr_sfz_authored_pedal(v->region)))continue;
  v->heldSustain=1;v->ampenv.segment=TSF_SEGMENT_SUSTAIN;v->ampenv.samplesUntilNextSegment=0x7fffffff;v->ampenv.slope=0;
  // Sustain the current modulation level; never reset the sample cursor or attack.
  v->modenv.segment=TSF_SEGMENT_SUSTAIN;v->modenv.samplesUntilNextSegment=0x7fffffff;v->modenv.slope=0;
 }
}
#endif
