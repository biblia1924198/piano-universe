#include <assert.h>
#include <stdio.h>
#include "../native/engine.h"
int main(int argc,char**argv){assert(argc==2);tsf*base=load_font(argv[1]);assert(base);FX*f=calloc(1,sizeof(*f));config_fx(f,.4f,1,.7f,0,120,4,.6f);assert(scene_init(f));f->protect=1;tsf*banks[4];float gain[4]={.5,.5,.5,.5},out[512];
 for(int ch=0;ch<4;ch++){banks[ch]=copy_font(base,256);tsf_channel_set_presetindex(banks[ch],0,0);pr_space_config(&f->parts[ch]->space,1,ch%3,80,35,75,85);f->parts[ch]->resonance=.15f;}
 scene_event(f,banks,(PrMidiEvent){0xB0,64,127,15});for(int note=48;note<84;note++)scene_event(f,banks,(PrMidiEvent){0x90,note,110,15});double energy=0;
 for(int n=0;n<120;n++){render_mixer_float(banks,f,gain,out,256);for(int i=0;i<512;i++){assert(isfinite(out[i])&&fabsf(out[i])<=.98001f);energy+=out[i]*out[i];}}assert(energy>1&&f->parts[0]->space.active>.9f);
 PrChannelFx*old=f->parts[0];scene_retire(f,banks[0],old,0,.5f);banks[0]=copy_font(base,256);tsf_channel_set_presetindex(banks[0],0,4);f->parts[0]=part_new(44100);assert(f->tails[0].part==old&&old->space.enabled&&old->space.texture==35&&!f->parts[0]->space.enabled);
 for(int note=48;note<84;note++)scene_event(f,banks,(PrMidiEvent){0x80,note,0,15});scene_event(f,banks,(PrMidiEvent){0xB0,64,0,15});for(int n=0;n<300;n++){render_mixer_float(banks,f,gain,out,256);for(int i=0;i<512;i++)assert(isfinite(out[i])&&fabsf(out[i])<=.98001f);}
 part_rate(old,48000);assert(old->space.enabled&&old->space.texture==35&&old->space.amount==80&&old->space.decay==85&&old->space.rate==48000);
 scene_all_silent(f);for(int ch=0;ch<4;ch++){for(int i=0;i<128;i++)assert(f->parts[ch]->space.pending[i]==0);assert(f->parts[ch]->space.env==0);tsf_close(banks[ch]);free(f->parts[ch]);}for(int i=0;i<PR_TAILS;i++){if(f->tails[i].bank)tsf_close(f->tails[i].bank);free(f->tails[i].part);}free(f);tsf_close(base);puts("PASS: four active hybrid/shimmer/resonance channels, loud pedal chords, linked protection, old/new scene private effect tails, stream rate settings and panic memory reset.");
}
