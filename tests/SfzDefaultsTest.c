#include <assert.h>
#include <stdio.h>
#include "../native/engine.h"

static double energy(tsf *bank){
 float pcm[512];double sum=0;smart_note(bank,60,100);
 for(int n=0;n<24;n++){tsf_render_float(bank,pcm,256,0);for(int i=0;i<512;i++){assert(isfinite(pcm[i]));sum+=pcm[i]*pcm[i];}}
 return sum;
}
int main(int argc,char **argv){
 assert(argc==2);tsf *base=load_font(argv[1]);assert(base&&base->prBank&&base->prMapping);
 tsf *a=tsf_copy(base),*b=tsf_copy(base);assert(a&&b);assert(tsf_set_max_voices(a,64)&&tsf_set_max_voices(b,64));
 tsf_channel_set_presetindex(a,0,0);tsf_channel_set_presetindex(b,0,0);
 assert(a->channels->channels[0].midiVolume>>7==80);assert(a->channels->channels[0].midiExpression>>7==100);assert(a->channels->channels[0].midiPan>>7==32);
 assert(a->channels->channels[0].panOffset<-.24f&&a->channels->channels[0].gainDB<-15);
 tsf_channel_midi_control(b,0,7,127);tsf_channel_midi_control(b,0,11,127);
 double low=energy(a),high=energy(b);assert(low>0&&high>low*8);
 silence(a);tsf_channel_set_presetindex(a,0,0);assert(a->channels->channels[0].midiVolume>>7==80);
 FX *f=calloc(1,sizeof(FX));assert(f);config_fx(f,.25,1,.8,0,120,4,.6);assert(scene_init(f));tsf *banks[4]={a,0,0,0};
 scene_event(f,banks,(PrMidiEvent){0xB0,7,0,1});assert(a->channels->channels[0].midiVolume>>7==0);assert(b->channels->channels[0].midiVolume>>7==127);
 scene_event(f,banks,(PrMidiEvent){0xB0,7,100,1});assert(a->channels->channels[0].midiVolume>>7==100);
 for(int i=0;i<4;i++)free(f->parts[i]);free(f);tsf_close(a);tsf_close(b);tsf_close(base);
 puts("PASS: actual native PRB4, audible default volume/expression, pan, copy/reset persistence, live MIDI and channel isolation");
}
