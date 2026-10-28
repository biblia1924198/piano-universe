#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "../native/engine.h"
static struct tsf_voice* find(tsf*b,int key){for(int i=0;i<b->voiceNum;i++)if(b->voices[i].playingPreset>=0&&b->voices[i].playingKey==key)return &b->voices[i];return 0;}
int main(int argc,char**argv){assert(argc==2);tsf*base=load_font(argv[1]),*b=copy_font(base,256);assert(base&&b);tsf_channel_set_presetindex(b,0,0);PrChannelFx*p=part_new(44100);p->pedalMode=1;float out[512];
 part_pedal(b,p,80);smart_note(b,60,100);for(int n=0;n<20;n++)tsf_render_float(b,out,256,0);part_release(b,p,60);struct tsf_voice*v=find(b,60);assert(v&&v->ampenv.segment==TSF_SEGMENT_RELEASE&&v->prPartialRelease);int length=v->ampenv.samplesUntilNextSegment;
 for(int n=0;n<3;n++)tsf_render_float(b,out,256,0);assert(v->prReleaseFrames==768);float level=v->ampenv.level;part_pedal(b,p,127);assert(v->heldSustain&&v->ampenv.segment==TSF_SEGMENT_SUSTAIN&&v->ampenv.level==level);
 part_pedal(b,p,70);assert(v->ampenv.segment==TSF_SEGMENT_RELEASE);int partial=v->ampenv.samplesUntilNextSegment;part_pedal(b,p,0);assert(v->ampenv.samplesUntilNextSegment<partial&&v->ampenv.level==level);assert(length>0);
 // A recycled voice must not be recaptured by a pedal press.
 silence(b);tsf_channel_set_presetindex(b,0,0);smart_note(b,62,100);v=find(b,62);assert(v);tsf_voice_endquick(b,v);part_pedal(b,p,127);assert(v->ampenv.segment==TSF_SEGMENT_RELEASE&&!v->prPedalRelease);
 // Old releases are not recaptured; repeated note-offs affect only their group.
 silence(b);tsf_channel_set_presetindex(b,0,0);part_pedal(b,p,80);smart_note(b,64,100);part_release(b,p,64);v=find(b,64);assert(v);unsigned group=v->playIndex;
 for(int n=0;n<20;n++)tsf_render_float(b,out,256,0);assert(v->prReleaseFrames>44100*.09);int old=v->ampenv.samplesUntilNextSegment;smart_note(b,64,110);part_release(b,p,64);assert(v->playIndex==group&&v->ampenv.samplesUntilNextSegment==old);part_pedal(b,p,127);assert(v->ampenv.segment==TSF_SEGMENT_RELEASE);
 // Off is exactly transparent in channel DSP. Enabling adds sound, not a queue.
 PrChannelFx*a=part_new(44100),*z=part_new(44100);float input[512],copy[512],mixA[512],mixZ[512];for(int i=0;i<512;i++)input[i]=.2f*sinf(i*.1f);memcpy(copy,input,sizeof(copy));memset(mixA,0,sizeof(mixA));memset(mixZ,0,sizeof(mixZ));a->resonance=0;z->resonance=0;part_process(a,input,mixA,256,1,1,0,.7);part_process(z,copy,mixZ,256,1,1,0,.7);assert(!memcmp(mixA,mixZ,sizeof(mixA))&&!a->strings.active);for(int i=0;i<512;i++)assert(mixA[i]==copy[i]);
 a->resonance=.3f;a->pedal=127;for(int n=0;n<200;n++){memcpy(input,copy,sizeof(input));memset(mixA,0,sizeof(mixA));part_process(a,input,mixA,256,1,1,0,.7);for(int i=0;i<512;i++)assert(isfinite(mixA[i]));}assert(a->strings.active);part_reset(a);for(int i=0;i<PR_RESONANCE_MODES;i++)assert(a->strings.x[i]==0&&a->strings.y[i]==0);
 free(a);free(z);free(p);tsf_close(b);tsf_close(base);puts("PASS: half-pedal/release age, immediate recapture without envelope jump, full damping, no revival of stolen/old voices, repeated-note isolation, bypass/reset.");
}
