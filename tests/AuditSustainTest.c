#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#ifdef NDEBUG
#error This audit requires assertions; compile with -UNDEBUG.
#endif
#include "../native/engine.h"
#include "../native/aaudio_backend.h"

static tsf* fixture(int regions,int packed){
 tsf*f=calloc(1,sizeof(*f));f->presetNum=1;f->presets=calloc(1,sizeof(struct tsf_preset));f->presets[0].regionNum=regions;f->presets[0].regions=calloc(regions,sizeof(struct tsf_region));
 f->sampleCount=4801;f->fontSamples=calloc(f->sampleCount,sizeof(float));for(int i=0;i<4801;i++)f->fontSamples[i]=.2f*sinf(6.283185307f*440*i/48000);
 f->prBank=packed; if(packed)f->prSeqCounters=calloc(regions,sizeof(unsigned));
 for(int i=0;i<regions;i++){struct tsf_region*r=&f->presets[0].regions[i];tsf_region_clear(r,0);r->lokey=0;r->hikey=127;r->lovel=1;r->hivel=127;r->sample_rate=48000;r->end=4800;r->loop_start=0;r->loop_end=4799;r->loop_mode=TSF_LOOPMODE_CONTINUOUS;r->pitch_keycenter=69;r->pitch_keytrack=0;r->initialFilterFc=13500;memset(&r->ampenv,0,sizeof(r->ampenv));memset(&r->modenv,0,sizeof(r->modenv));r->ampenv.sustain=1;r->ampenv.release=.2f;r->modenv.sustain=1;}
 tsf_set_output(f,TSF_STEREO_INTERLEAVED,48000,-10);tsf_set_max_voices(f,288);tsf_channel_set_presetindex(f,0,0);return f;
}
static void render(tsf*f,int frames){float out[512];while(frames){int n=frames>256?256:frames;tsf_render_float(f,out,n,0);frames-=n;}}
static void silent_voices(void){
 tsf*f=fixture(1,0);f->presets[0].regions[0].ampenv.sustain=0;f->presets[0].regions[0].ampenv.decay=.02f;tsf_channel_set_sustain(f,0,1);
 for(int i=0;i<200;i++){smart_note(f,21+i%88,100);tsf_channel_note_off(f,0,21+i%88);}render(f,48000*3);int silent=0;for(int i=0;i<f->voiceNum;i++)silent+=f->voices[i].playingPreset>=0&&f->voices[i].ampenv.level==0;
 printf("silent_sustain: active=%d zero_envelope=%d after=3s\n",tsf_active_voice_count(f),silent);
#ifdef PR_AUDIT_FIXED
 assert(silent==0&&tsf_active_voice_count(f)==0);
#endif
 long long begin=pr_boottime();render(f,48000*10);printf("silent_voices_10s_render_ms=%.3f\n",(pr_boottime()-begin)/1e6);tsf_close(f);
}
static void held_recycle(void){
 tsf*f=fixture(2,0);tsf_set_max_voices(f,288);f->prPedalFade=8;PrChannelFx*p=part_new(48000);p->pedalMode=1;part_pedal(f,p,127);
 for(int i=0;i<180;i++){smart_note(f,21+i%88,100);part_release(f,p,21+i%88);render(f,64);}
 int quick=0;for(int i=0;i<f->voiceNum;i++)quick+=f->voices[i].playingPreset>=0&&!f->voices[i].prPedalRelease&&f->voices[i].ampenv.segment==TSF_SEGMENT_RELEASE;
 printf("recycle: active=%d quick_fades=%d soft=%ld hard=%ld\n",tsf_active_voice_count(f),quick,smartSoft,smartHard);free(p);tsf_close(f);
}
static void pending_release(void){
 tsf*f=fixture(2,1);f->presets[0].regions[1].prTrigger=1;f->presets[0].regions[1].loop_mode=TSF_LOOPMODE_NONE;PrChannelFx*p=part_new(48000);part_pedal(f,p,127);
 for(int i=0;i<2816;i++){int key=21+i%88;smart_note(f,key,100);part_release(f,p,key);render(f,16);}
 unsigned pending=0;for(int k=0;k<128;k++)pending+=f->prRelease[k];unsigned before=f->voicePlayIndex;long long begin=pr_boottime();part_pedal(f,p,0);double ms=(pr_boottime()-begin)/1e6;
 printf("release_burst: deferred=%u triggered_in_single_event=%u pedal_event_ms=%.3f callback_budget_192frames_ms=4.000\n",pending,f->voicePlayIndex-before,ms);free(p);tsf_close(f);
}
static void contention(void){
 FX*f=calloc(1,sizeof(*f));f->rate=48000;nativeStream.fx=f;nativeStream.rate=48000;nativeStream.last[0]=nativeStream.last[1]=.2f;float out[384];pr_lock();aa_data(0,&nativeStream,out,192);pr_unlock();int zeros=0;for(int i=0;i<192;i++)zeros+=out[i*2]==0;printf("contention: callback_frames=192 zero_frames=%d drops=%d\n",zeros,nativeStream.drops);free(f);nativeStream.fx=0;
}
static void stolen_release(void){
 tsf*f=fixture(1,1);f->prPedalFade=8;f->prSfz5=1;f->prExtData=calloc(1,sizeof(struct pr_sfz_ext));struct tsf_region*r=&f->presets[0].regions[0];r->prExt=f->prExtData;r->prExt->sustain=64;r->prExt->randomHi=1;r->prExt->side=-1;r->prSfz5=1;r->prSfzFlags=4;r->prVelTrack=100;r->ampenv.release=8;
 tsf_channel_midi_control(f,0,64,127);smart_note(f,60,100);render(f,256);tsf_channel_note_off(f,0,60);struct tsf_voice*v=0;for(int i=0;i<f->voiceNum;i++)if(f->voices[i].playingPreset>=0){v=&f->voices[i];break;}assert(v&&v->heldSustain);tsf_voice_endquick(f,v);int before=v->ampenv.samplesUntilNextSegment;pr_sfz_controller(f,0,64,0);printf("stolen_release: quick_frames_before=%d release_frames_after_pedal_up=%d prPedalRelease=%d\n",before,v->ampenv.samplesUntilNextSegment,v->prPedalRelease);
#ifdef PR_AUDIT_FIXED
 assert(v->ampenv.samplesUntilNextSegment==before&&!v->prPedalRelease&&!v->heldSustain);render(f,4800);assert(tsf_active_voice_count(f)==0);
#endif
 tsf_close(f);
}
static void chorus_reference(void){
 PrChannelFx*p=part_new(48000);p->gain=1;p->chorus=p->chorusNow=1;double maxError=0,sq=0;int count=0;
 for(int i=0;i<24000;i++){float x=.2f*sinf((float)(6.283185307179586*2000*i/48000)),in[2]={x,x},out[2]={0};part_process(p,in,out,1,1,1,0,.6f);if(i>1000){float lag=(.012f+.004f*sinf(p->phase))*48000;double ref=.1*sin(6.283185307179586*2000*(i-lag)/48000);double err=(out[0]-x)-ref;if(fabs(err)>maxError)maxError=fabs(err);sq+=err*err;count++;}}
 printf("chorus_2kHz_reference: max_error=%.8f rms_error=%.8f\n",maxError,sqrt(sq/count));
#ifdef PR_AUDIT_FIXED
 assert(maxError<.0012);
#endif
 free(p);
}
static void quiet_attack_guard(void){tsf*f=fixture(1,0);struct tsf_region*r=f->presets[0].regions;r->ampenv.delay=.01f;r->ampenv.attack=.01f;smart_note(f,60,100);render(f,240);assert(tsf_active_voice_count(f)==1);render(f,2400);assert(tsf_active_voice_count(f)==1);float out[512];tsf_render_float(f,out,256,0);float peak=0;for(int i=0;i<512;i++)peak=fmaxf(peak,fabsf(out[i]));assert(peak>.001f);puts("delayed_attack_guard=PASS");tsf_close(f);}
int main(void){pr_dsp_denormals();silent_voices();held_recycle();pending_release();contention();stolen_release();chorus_reference();quiet_attack_guard();return 0;}
