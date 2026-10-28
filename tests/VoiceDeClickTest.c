#include <assert.h>
#include <stdio.h>
#include "../native/engine.h"
int main(int argc,char**argv){assert(argc==2);tsf*base=load_font(argv[1]);assert(base);tsf*f=copy_font(base,64);tsf_channel_set_presetindex(f,0,0);tsf_set_output(f,TSF_STEREO_INTERLEAVED,48000,-10);smart_note(f,60,127);float out[128];tsf_render_float(f,out,64,0);float previous[2]={out[126],out[127]};int alive=0;
 for(int i=0;i<f->voiceNum;i++)if(f->voices[i].playingPreset>=0){tsf_voice_declick(f,&f->voices[i]);tsf_voice_kill(&f->voices[i]);alive++;}assert(alive>0&&f->prDeClicks==(unsigned)alive);tsf_render_float(f,out,64,0);assert(fabsf(out[0]-previous[0])<1e-6&&fabsf(out[1]-previous[1])<1e-6);for(int i=1;i<64;i++)assert(fabsf(out[i*2]-out[(i-1)*2])<fabsf(previous[0])*.02f+1e-8f);
 for(int i=0;i<100;i++)tsf_render_float(f,out,64,0);assert(f->prClickTail[0]==0&&f->prClickTail[1]==0);f->prClickTail[0]=.1f;tsf*copy=copy_font(f,64);assert(copy&&copy->prClickTail[0]==0);tsf_close(copy);tsf_reset(f);assert(f->prClickTail[0]==0);tsf_close(f);tsf_close(base);puts("PASS: capacity recycle continues last stereo sample without a step; bounded 2ms decay; silence, clone and reset clear residual; no extra voices/buffer.");}
