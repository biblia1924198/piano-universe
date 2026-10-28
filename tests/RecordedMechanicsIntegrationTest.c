#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "../native/engine.h"
static int active(PrMechanics*m){int n=0;for(int i=0;i<PR_MECH_VOICES;i++)n+=m->recorded[i].sample!=0;return n;}
int main(int argc,char**argv){
 assert(argc==3&&pr_mech_load(argv[2]));tsf*base=load_font(argv[1]);assert(base);FX*f=calloc(1,sizeof(*f));assert(f&&scene_init(f));config_fx(f,.2f,1,.5f,0,120,4,.6f);f->protect=1;f->yamaha=part_new(44100);f->yamahaEnabled=1;f->yamahaGain=.2f;
 tsf*banks[4]={0};float gains[4]={.3,.3,.3,0},out[128];
 for(int c=0;c<3;c++){banks[c]=copy_font(base,256);assert(banks[c]);tsf_channel_set_presetindex(banks[c],0,0);PrChannelFx*p=f->parts[c];p->mechanics.hammer=p->mechanics.key=50;p->mechanics.pedal=30;p->chorus=.12f;p->delay=.1f;p->resonance=.1f;}
 f->yamaha->mechanics=f->parts[0]->mechanics;
 scene_event(f,banks,(PrMidiEvent){0xB0,64,127,7});for(int c=0;c<3;c++)assert(active(&f->parts[c]->mechanics)==1);assert(active(&f->yamaha->mechanics)==1);
 scene_event(f,banks,(PrMidiEvent){0xB0,64,127,7});for(int c=0;c<3;c++)assert(active(&f->parts[c]->mechanics)==1);assert(active(&f->yamaha->mechanics)==1);
 for(int c=0;c<3;c++)pr_mech_clear(&f->parts[c]->mechanics);pr_mech_clear(&f->yamaha->mechanics);
 scene_event(f,banks,(PrMidiEvent){0x90,60,110,7});for(int i=0;i<5;i++)render_mixer_float(banks,f,gains,out,64);for(int c=0;c<3;c++)assert(!active(&f->parts[c]->mechanics));
 scene_event(f,banks,(PrMidiEvent){0x80,60,0,7});for(int c=0;c<3;c++){assert(active(&f->parts[c]->mechanics)==1&&f->parts[c]->pedal==127);assert(tsf_active_voice_count(banks[c])>0);}assert(active(&f->yamaha->mechanics)==1);
 scene_event(f,banks,(PrMidiEvent){0x80,60,0,7});for(int c=0;c<3;c++)assert(active(&f->parts[c]->mechanics)==1);assert(active(&f->yamaha->mechanics)==1);
 // Stress musical polyphony and independent mechanical pools under sustained fast notes.
 double energy=0;for(int block=0;block<1600;block++){
  if(block%3==0){for(int n=0;n<4;n++){int key=21+(block+n*7)%88;scene_event(f,banks,(PrMidiEvent){0x90,key,20+(block+n)%108,7});scene_event(f,banks,(PrMidiEvent){0x80,key,0,7});}}
  if(block%151==0){scene_event(f,banks,(PrMidiEvent){0xB0,64,0,7});scene_event(f,banks,(PrMidiEvent){0xB0,64,127,7});}
  render_mixer_float(banks,f,gains,out,64);for(int n=0;n<128;n++){assert(isfinite(out[n])&&fabsf(out[n])<=.98001f);energy+=out[n]*out[n];}for(int c=0;c<3;c++)assert(active(&f->parts[c]->mechanics)<=32);
 }assert(energy>1);
 scene_all_silent(f);for(int c=0;c<3;c++)assert(!active(&f->parts[c]->mechanics));assert(!active(&f->yamaha->mechanics));
 for(int c=0;c<4;c++){if(banks[c])tsf_close(banks[c]);free(f->parts[c]);}free(f->yamaha);free(f);tsf_close(base);PrMechanicalBank*b=pr_recorded_bank();free(b->data);free(b);pr_mechanical_bank=0;
 puts("PASS: actual sampler + Yamaha sidechain; key-up during sustain exactly once; repeated pedal CC silent; three layered banks/effects/rapid velocity variations with bounded recorded voices; master finite and panic clears mechanics.");
}
