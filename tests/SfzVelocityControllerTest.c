#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "../native/engine.h"

static struct tsf_voice*newest(tsf*b,int channel){struct tsf_voice*last=0;for(int i=0;i<b->voiceNum;i++){struct tsf_voice*v=&b->voices[i];if(v->playingPreset>=0&&v->playingChannel==channel&&(!last||v->playIndex>last->playIndex))last=v;}assert(last);return last;}
static float gain(tsf*b,int channel){return powf(10,(newest(b,channel)->noteGainDB-b->globalGainDB)/20);}
static double render(tsf*b){float pcm[128];double sum=0;for(int n=0;n<100;n++){tsf_render_float(b,pcm,64,0);for(int i=0;i<128;i++){assert(isfinite(pcm[i]));sum+=pcm[i]*pcm[i];}}return sum;}
static void close_fx(FX*f){for(int i=0;i<4;i++)free(f->parts[i]);for(int i=0;i<PR_TAILS;i++){tsf_close(f->tails[i].bank);free(f->tails[i].part);}free(f);}
int main(int argc,char**argv){
 assert(argc==3);tsf*base=load_font(argv[1]);assert(base&&base->prVelCCData&&base->prMapping&&base->prVelCCMask[99]&&!base->prVelCCMask[98]);
 tsf*a=copy_font(base,128),*b=copy_font(base,128);assert(a&&b);tsf_close(base); // Shared metadata/mmap outlive the loading handle.
 tsf_channel_set_presetindex(a,0,0);tsf_channel_set_presetindex(a,1,0);tsf_channel_set_presetindex(b,0,0);
 float velocity=64/127.f,squared=velocity*velocity;
 tsf_channel_note_on(a,0,60,velocity);assert(fabsf(gain(a,0)-(1-(63/127.f)*(1-squared)))<1e-5);
 struct tsf_voice*old=newest(a,0);float oldGain=old->noteGainDB,oldLevel=old->ampenv.level;
 tsf*banks[4]={a,b,0,0};apply_event(banks,(PrMidiEvent){0xB0,99,0,1});assert(a->channels->channels[0].prCC[99]==0&&b->channels->channels[0].prCC[99]==63);assert(old->noteGainDB==oldGain&&old->ampenv.level==oldLevel);
 tsf_channel_note_on(a,0,60,velocity);assert(fabsf(gain(a,0)-1)<1e-5);
 tsf_channel_note_on(a,1,60,velocity);assert(fabsf(gain(a,1)-(1-(63/127.f)*(1-squared)))<1e-5);
 apply_event(banks,(PrMidiEvent){0xB0,99,127,1});tsf_channel_note_on(a,0,60,velocity);assert(fabsf(gain(a,0)-squared)<1e-5);assert(old->noteGainDB==oldGain);
 tsf_channel_midi_control(a,0,121,0);assert(a->channels->channels[0].prCC[99]==63);assert(a->channels->channels[1].prCC[99]==63);
 // Audible PCM energy, not just inspecting stored coefficients.
 silence(a);silence(b);tsf_channel_set_presetindex(a,0,0);tsf_channel_set_presetindex(b,0,0);tsf_channel_midi_control(a,0,99,0);tsf_channel_midi_control(b,0,99,127);smart_note(a,60,64);smart_note(b,60,64);
 double loud=render(a),soft=render(b);assert(soft>0&&fabs(soft/loud-squared*squared)<1e-5);
 FX*f=calloc(1,sizeof(*f));assert(f);config_fx(f,0,1,.8,0,120,4,.6);assert(scene_init(f));
 scene_event(f,banks,(PrMidiEvent){0xB0,99,42,15});assert(a->channels->channels[0].prCC[99]==42&&b->channels->channels[0].prCC[99]==42);
 scene_retire(f,a,f->parts[0],0,.8);f->parts[0]=part_new(44100);assert(f->parts[0]);f->parts[0]->owner=++f->serial;tsf*next=copy_font(b,128);assert(next);tsf_channel_set_presetindex(next,0,0);banks[0]=next;
 scene_event(f,banks,(PrMidiEvent){0xB0,99,77,15});assert(next->channels->channels[0].prCC[99]==77&&a->channels->channels[0].prCC[99]==77);assert(isfinite((float)render(a)));
 for(int i=0;i<1000;i++){scene_event(f,banks,(PrMidiEvent){0xB0,99,i%128,15});scene_event(f,banks,(PrMidiEvent){0x90,60,1+i%127,15});if(i%4==0)scene_event(f,banks,(PrMidiEvent){0x80,60,0,15});float out[128],gains[4]={.6,.6,0,0};render_mixer_float(banks,f,gains,out,64);for(int k=0;k<128;k++)assert(isfinite(out[k]));}
 tsf_close(next);tsf_close(b);close_fx(f); // a is owned by a scene tail.
 tsf*x=load_font(argv[2]);assert(x&&tsf_set_max_voices(x,32));tsf_channel_set_presetindex(x,0,0);tsf_channel_midi_control(x,0,99,127);tsf_channel_note_on(x,0,60,velocity);assert(fabsf(gain(x,0)-.5f*(1-squared))<1e-5);
 tsf_channel_midi_control(x,0,0,127);tsf_channel_midi_control(x,0,127,127);tsf_channel_note_on(x,0,60,velocity);assert(fabsf(gain(x,0)-(1-.75f*(1-squared)))<1e-5);
 tsf_channel_midi_control(x,0,99,0);tsf_channel_note_on(x,0,60,velocity);assert(fabsf(gain(x,0)-squared)<1e-5); // Sum above 100 is safely bounded.
 assert(render(x)>0);tsf_close(x);
 puts("PASS: native CC99 dynamic response at attack, exact rendered PCM gain, stereo/channel/mixer isolation, held voices unchanged, defaults/reset, scene tails, negative/multiple CCs, bounded extremes and 1000-note stress; shared mmap lifetime.");
}
