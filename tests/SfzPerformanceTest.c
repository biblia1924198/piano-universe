#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "../native/engine.h"
static struct tsf_voice*first(tsf*b){for(int i=0;i<b->voiceNum;i++)if(b->voices[i].playingPreset>=0)return &b->voices[i];assert(0);return 0;}
static double render(tsf*b,float seconds){float pcm[128];double energy=0;int frames=(int)(seconds*b->outSampleRate);while(frames){int n=frames>64?64:frames;tsf_render_float(b,pcm,n,0);for(int i=0;i<n*2;i++){assert(isfinite(pcm[i]));energy+=pcm[i]*pcm[i];}frames-=n;}return energy;}
static int held(tsf*b){int n=0;for(int i=0;i<b->voiceNum;i++)if(b->voices[i].playingPreset>=0&&b->voices[i].ampenv.segment<TSF_SEGMENT_RELEASE)n++;return n;}
static void start(tsf*b,int velocity){smart_note(b,60,velocity);assert(held(b)>=2);}
int main(int argc,char**argv){
 assert(argc==2);tsf*b=load_font(argv[1]);assert(b&&b->prSfz5&&b->prMapping);tsf_set_max_voices(b,256);tsf_set_output(b,TSF_STEREO_INTERLEAVED,48000,-10);tsf_channel_set_presetindex(b,0,0);PrChannelFx*p=part_new(48000);assert(p);p->pedalMode=1;b->prContext=1;
 start(b,100);struct tsf_voice*v=first(b);assert(fabsf(v->prCCGain-100/127.f)<1e-6);assert(fabsf(v->noteGainDB-(-10+20*log10f(powf(100/127.f,1.3f))))<.001);assert(fabs(v->pitchInputTimecents-(6000.0+v->region->tune+v->region->prFineTune))<.00001);
 assert(render(b,.6f)>0);part_pedal(b,p,127);part_release(b,p,60);v=first(b);assert(!v->heldSustain&&v->ampenv.segment==TSF_SEGMENT_RELEASE);assert(fabsf(v->ampenv.parameters.release-1.6f)<.00001);assert(render(b,.2f)>0);float level=v->ampenv.level;part_pedal(b,p,0);assert(v->ampenv.level==level);assert(v->ampenv.samplesUntilNextSegment<48000*.251f);render(b,.3f);assert(tsf_active_voice_count(b)==0);
 silence(b);tsf_channel_set_presetindex(b,0,0);start(b,100);render(b,.5f);part_release(b,p,60);assert(fabsf(first(b)->ampenv.parameters.release-.45f)<.00001);render(b,.5f);assert(tsf_active_voice_count(b)==0);
 silence(b);tsf_channel_set_presetindex(b,0,0);start(b,100);assert(render(b,9)>0);assert(held(b)==2);tsf_channel_midi_control(b,0,11,0);render(b,.1f);double muted=render(b,.1f);assert(muted<1e-8);tsf_channel_midi_control(b,0,11,127);assert(render(b,.1f)>muted+1e-5);assert(held(b)==2);
 silence(b);tsf_channel_set_presetindex(b,0,0);start(b,100);render(b,.01f);start(b,100);render(b,.01f);start(b,100);assert(held(b)==4);int normal=0;for(int i=0;i<b->voiceNum;i++){v=&b->voices[i];if(v->playingPreset>=0&&v->prSfzOff){normal++;assert(v->ampenv.segment==TSF_SEGMENT_RELEASE&&fabsf(v->ampenv.parameters.release-.45f)<1e-6);}}assert(normal==2);start(b,40);assert(held(b)==6); /* Softer strike does not self-mask loud strikes. */
 part_pedal(b,p,127);for(int i=0;i<b->voiceNum;i++){v=&b->voices[i];if(v->playingPreset>=0&&v->prSfzOff)assert(v->ampenv.segment==TSF_SEGMENT_RELEASE);}for(int i=0;i<64;i++){start(b,127);render(b,.01f);}assert(held(b)==4);render(b,1.7f);assert(held(b)==4);
 tsf_channel_midi_control(b,0,121,0);assert(b->channels->channels[0].prCC[64]==0&&b->channels->channels[0].prCC[11]==127);
 silence(b);tsf_channel_set_presetindex(b,0,0);b->sampleQuality=1;start(b,100);assert(render(b,9)>0&&held(b)==2);
 silence(b);tsf_channel_set_presetindex(b,0,0);tsf_channel_set_presetindex(b,1,0);tsf_channel_midi_control(b,0,11,0);tsf_channel_note_on(b,0,60,100/127.f);tsf_channel_note_on(b,1,60,100/127.f);for(int i=0;i<b->voiceNum;i++){v=&b->voices[i];if(v->playingPreset>=0)assert(v->prCCTarget==(v->playingChannel==0?0:100/127.f));}for(int j=0;j<3;j++)for(int channel=0;channel<2;channel++)tsf_channel_note_on(b,channel,60,100/127.f);assert(held(b)==8);
 silence(b);tsf_channel_set_presetindex(b,0,0);int saved=b->presets[0].regionNum,key=b->presets[0].regions[0].lokey;int savedHi=b->presets[0].regions[0].hivel;b->presets[0].regions[0].hivel=127;b->presets[0].regionNum=1;for(int i=0;i<3;i++)tsf_channel_note_on(b,0,key,100/127.f);assert(held(b)==2);tsf_channel_note_on(b,0,key,40/127.f);assert(held(b)==3);tsf_channel_note_on(b,0,key,127/127.f);assert(held(b)==2);b->presets[0].regionNum=saved;b->presets[0].regions[0].hivel=savedHi;
 /* A PRB5 bank with only tuning/velocity extensions still uses optional piano
    half-pedal/context processing unless it authors its own pedal envelope. */
 for(int i=0;i<b->presets[0].regionNum;i++){b->presets[0].regions[i].prSfzFlags=4;b->presets[0].regions[i].prReleaseCC64=0;}
 silence(b);tsf_channel_set_presetindex(b,0,0);p->pedal=0;part_pedal(b,p,64);start(b,100);render(b,.5f);part_release(b,p,60);v=first(b);assert(v->prPartialRelease&&v->ampenv.segment==TSF_SEGMENT_RELEASE&&v->ampenv.samplesUntilNextSegment>48000*.5f);render(b,.01f);part_pedal(b,p,127);assert(held(b)==2);
 free(p);tsf_close(b);puts("PASS: actual PRB5 native PCM, exact CC/velocity gain and tuning, stereo loops beyond sample end, authored 1.6s/.45s pedal release and live shortening without level reset; expression mute/restore; repeated stereo note events, self-mask and no pedal resurrection; sustained stress finite; mono event limits and channel isolation; high-quality loop boundary.");
}
