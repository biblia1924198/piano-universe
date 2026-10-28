#include <assert.h>
#include <stdio.h>
#include "../native/engine.h"
#ifdef NDEBUG
#error Assertions must be enabled.
#endif
int main(int argc,char**argv){
 assert(argc==2);pr_dsp_denormals();tsf*base=load_font(argv[1]);assert(base);FX*f=calloc(1,sizeof(*f));assert(f);f->rate=48000;assert(scene_init(f));config_fx(f,.3,1,.65,0,120,4,.6);f->protect=1;tsf*banks[4]={0};float gains[4]={.28,.28,.28,0};int presets[3]={0,89,48};
 for(int i=0;i<3;i++){banks[i]=copy_font(base,256);assert(banks[i]);tsf_set_output(banks[i],TSF_STEREO_INTERLEAVED,48000,-10);int p=tsf_get_presetindex(banks[i],0,presets[i]);assert(p>=0);tsf_channel_set_presetindex(banks[i],0,p);banks[i]->sampleQuality=1;f->currentBanks[i]=banks[i];f->parts[i]->chorus=.15f;f->parts[i]->resonance=.15f;pr_space_config(&f->parts[i]->space,1,0,35,0,0,35);}
 float out[512],lastPeak=0;int chord[8]={48,55,60,64,67,72,76,79},maxVoices=0;unsigned notes=0;long long start=pr_boottime(),idleStart=0,idleElapsed=0;int released=0,reentered=0;
 scene_event(f,banks,(PrMidiEvent){0xB0,64,127,7});
 for(int frame=0;frame<248*48000;frame+=256){
  if(frame<60*48000 && frame%24064==0){for(int k=0;k<8;k++){scene_event(f,banks,(PrMidiEvent){0x90,chord[k],80+(notes%40),7});scene_event(f,banks,(PrMidiEvent){0x80,chord[k],0,7});notes++;}}
  if(!released&&frame>=60*48000){released=1;scene_event(f,banks,(PrMidiEvent){0xB0,64,0,7});idleStart=pr_boottime();}
  if(!reentered&&frame>=240*48000){idleElapsed=pr_boottime()-idleStart;int live=0;for(int c=0;c<3;c++)live+=tsf_active_voice_count(banks[c]);printf("after_180s_idle: voices=%d last_block_peak=%.9g offline_cpu_ms=%.3f\n",live,lastPeak,idleElapsed/1e6);assert(live==0&&lastPeak<.00001f);for(int k=0;k<8;k++)scene_event(f,banks,(PrMidiEvent){0x90,chord[k],100,7});reentered=1;}
  render_mixer_float(banks,f,gains,out,256);lastPeak=0;for(int i=0;i<512;i++){assert(isfinite(out[i])&&fabsf(out[i])<=.981f);lastPeak=fmaxf(lastPeak,fabsf(out[i]));}int live=0;for(int c=0;c<3;c++)live+=tsf_active_voice_count(banks[c]);if(live>maxVoices)maxVoices=live;
 }
 printf("long_session: audio_seconds=248 channels=3 pedal_seconds=60 idle_seconds=180 notes=%u max_voices=%d invalid=%ld soft=%ld hard=%ld last_reentry_peak=%.6f cpu_ms=%.3f\n",notes,maxVoices,f->invalidSamples,smartSoft,smartHard,lastPeak,(pr_boottime()-start)/1e6);assert(f->invalidSamples==0&&lastPeak>0);for(int i=0;i<4;i++){if(banks[i])tsf_close(banks[i]);free(f->parts[i]);}tsf_close(base);free(f);puts("PASS: offline sustain, silence, reentry; not Android lifecycle or physical phone validation.");return 0;
}
