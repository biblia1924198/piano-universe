#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "../native/engine.h"
int main(int argc,char**argv){assert(argc==2);tsf*base=load_font(argv[1]);assert(base);FX*f=calloc(1,sizeof(*f));config_fx(f,.25,1,.8,0,120,4,.6);assert(scene_init(f));f->tailScale=1;f->protect=1;
 tsf*a=copy_font(base,256),*b=copy_font(base,256);assert(a&&b);tsf_channel_set_presetindex(a,0,0);tsf_channel_set_presetindex(b,0,4);tsf*banks[4]={a,0,0,0};float gains[4]={.8,0,0,0},out[512];
 scene_event(f,banks,(PrMidiEvent){0x90,60,100,1});scene_event(f,banks,(PrMidiEvent){0xB0,64,127,15});for(int n=0;n<20;n++)render_mixer_float(banks,f,gains,out,256);assert(tsf_active_voice_count(a)>0);assert(f->parts[0]->held[60]==1);
 PrChannelFx*next=part_new(44100);next->owner=++f->serial;scene_retire(f,a,f->parts[0],0,.8);f->parts[0]=next;next->transpose=12;banks[0]=b;part_pedal(b,next,127);scene_event(f,banks,(PrMidiEvent){0x90,60,110,1});assert(next->held[60]==1);int found=0;for(int i=0;i<b->voiceNum;i++)if(b->voices[i].playingPreset>=0){assert(b->voices[i].playingKey==72);found++;}assert(found);
 scene_event(f,banks,(PrMidiEvent){0x80,60,0,15});assert(f->tails[0].part->held[60]==0);assert(next->held[60]==1);assert(tsf_active_voice_count(a)>0);scene_event(f,banks,(PrMidiEvent){0x80,60,0,15});assert(!next->held[60]);
 scene_event(f,banks,(PrMidiEvent){0xB0,64,0,15});for(int i=0;i<a->voiceNum;i++)if(a->voices[i].playingPreset>=0)assert(a->voices[i].ampenv.segment>=TSF_SEGMENT_RELEASE);
 double energy=0;for(int n=0;n<3000;n++){render_mixer_float(banks,f,gains,out,256);for(int i=0;i<512;i++){assert(isfinite(out[i]));assert(fabsf(out[i])<=.98001f);energy+=out[i]*out[i];}}assert(energy>0&&f->tails[0].done);
 // Partial pedal keeps a release finite, and does not synthesize MIDI resolution.
 next->pedalMode=1;part_pedal(b,next,60);smart_note(b,64,100);part_release(b,next,64);int release=0;for(int i=0;i<b->voiceNum;i++)if(b->voices[i].playingPreset>=0&&b->voices[i].playingKey==64){assert(b->voices[i].ampenv.segment==TSF_SEGMENT_RELEASE);release++;}assert(release);
 // All DSP enabled and heavy input stays bounded through master protection.
 next->chorus=.5;next->delay=.4;next->feedback=.6;next->resonance=.2;next->pan=.3;part_pedal(b,next,127);smart_note(b,64,127);for(int n=0;n<1000;n++){render_mixer_float(banks,f,gains,out,256);for(int i=0;i<512;i++)assert(isfinite(out[i])&&fabsf(out[i])<=.98001f);}
 PrChannelFx*fade=part_new(44100);float dry[512],faded[512]={0};for(int i=0;i<512;i++)dry[i]=.2f;fade->fade=100;part_process(fade,dry,faded,256,1,1,0,.77);assert(faded[0]>0);for(int i=200;i<512;i++)assert(faded[i]==0);free(fade);
 FX*other=calloc(1,sizeof(*other));config_fx(other,0,1,.8,0,120,4,.6);assert(scene_init(other));tsf*x=copy_font(base,256),*y=copy_font(base,256);tsf_channel_set_presetindex(x,0,0);tsf_channel_set_presetindex(y,0,0);tsf*both[4]={x,y,0,0};scene_event(other,both,(PrMidiEvent){0x90,60,90,1});scene_event(other,both,(PrMidiEvent){0x90,60,110,2});scene_event(other,both,(PrMidiEvent){0x80,60,0,15});assert(!other->parts[0]->held[60]&&other->parts[1]->held[60]==1);scene_event(other,both,(PrMidiEvent){0x80,60,0,15});assert(!other->parts[1]->held[60]);for(int c=0;c<4;c++)free(other->parts[c]);free(other);tsf_close(x);tsf_close(y);
 panic_fx(b,0,f);assert(tsf_active_voice_count(b)==0);for(int i=0;i<4;i++)free(f->parts[i]);for(int i=0;i<PR_TAILS;i++){if(f->tails[i].bank)tsf_close(f->tails[i].bank);free(f->tails[i].part);}tsf_close(b);tsf_close(base);free(f);puts("Scene continuity: FIFO note-offs, old transpose, pedal/tails and finite channel DSP passed");
}
