#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../native/engine.h"
int main(int argc,char**argv){assert(argc==3);tsf*bank=load_font(argv[1]);assert(bank);FILE*out=fopen(argv[2],"wb");assert(out);long long begin=pr_boottime();int presets[]={0,48,89};
for(int p=0;p<3;p++){tsf*f=copy_font(bank,512);assert(f);f->sampleQuality=1;tsf_set_output(f,TSF_STEREO_INTERLEAVED,48000,-10);tsf_channel_set_presetindex(f,0,presets[p]);float block[192];
for(int tick=0;tick<2000;tick++){
 if(tick%64==0){tsf_channel_midi_control(f,0,64,127);for(int n=0;n<6;n++)smart_note(f,36+(tick/64+n*7)%48,30+(tick+n*17)%98);}
 if(tick%64==32)for(int n=0;n<6;n++)tsf_channel_note_off(f,0,36+((tick-32)/64+n*7)%48);
 if(tick%193==0)tsf_channel_set_pitchwheel(f,0,(tick*43)%16384);
 if(tick%127==0)tsf_channel_set_volume(f,0,.3f+(tick%90)/100.f);
 if(tick%191==0)tsf_channel_midi_control(f,0,11,30+(tick%98));
 if(tick%311==0)tsf_channel_midi_control(f,0,64,0);
 tsf_render_float(f,block,96,0);assert(fwrite(block,sizeof(float),192,out)==192);
 }tsf_close(f);}
printf("waveform_render_ms=%.3f\n",(pr_boottime()-begin)/1e6);fclose(out);tsf_close(bank);return 0;}
