#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "../native/engine.h"
#include "../native/piano_profile.h"
static void advance(tsf*b,int frames){float out[512];while(frames){int n=frames>256?256:frames;tsf_render_float(b,out,n,0);for(int i=0;i<n*2;i++)assert(isfinite(out[i]));frames-=n;}}
static tsf* synthetic(void){
 tsf*f=calloc(1,sizeof(*f));f->presetNum=1;f->presets=calloc(1,sizeof(struct tsf_preset));f->presets[0].regionNum=2;f->presets[0].regions=calloc(2,sizeof(struct tsf_region));f->sampleCount=144001;f->fontSamples=calloc(f->sampleCount,sizeof(float));
 for(unsigned i=0;i<f->sampleCount;i++)f->fontSamples[i]=.25f*sinf(6.283185307f*442*i/48000)*expf(-(float)i/(48000.f*2));
 for(int i=0;i<2;i++){struct tsf_region*r=&f->presets[0].regions[i];tsf_region_clear(r,0);r->lokey=21;r->hikey=108;r->lovel=i?65:1;r->hivel=i?127:64;r->pitch_keycenter=69;r->pitch_keytrack=100;r->sample_rate=48000;r->end=144000;memset(&r->ampenv,0,sizeof(r->ampenv));memset(&r->modenv,0,sizeof(r->modenv));r->modenv.sustain=1;r->ampenv.sustain=1;r->ampenv.release=.2f;r->initialFilterFc=13500;}
 tsf_set_output(f,TSF_STEREO_INTERLEAVED,48000,-10);tsf_channel_set_presetindex(f,0,0);tsf_set_max_voices(f,288);pr_blend_prepare(f);return f;
}
int main(int argc,char**argv){assert(argc==2);tsf*f=synthetic();struct tsf_region*a=&f->presets[0].regions[0],*b=&f->presets[0].regions[1];assert(a->prBlendHi==8&&b->prBlendLo==8);
 for(int width=1;width<=8;width++){f->prBlend=width;for(int v=1;v<=127;v++){float x=pr_region_gain(f,a,69,v),y=pr_region_gain(f,b,69,v);assert(fabsf(x*x+y*y-1)<1e-6);f->prBlendCurve=1;x=pr_region_gain(f,a,69,v);y=pr_region_gain(f,b,69,v);assert(fabsf(x+y-1)<1e-6);f->prBlendCurve=0;}}
 f->prBlend=4;smart_note(f,69,64);assert(tsf_active_voice_count(f)==2);advance(f,1024);silence(f);tsf_channel_set_presetindex(f,0,0);f->prBlend=0;smart_note(f,69,64);assert(tsf_active_voice_count(f)==1);
 // Existing overlapping zones and exclusive groups are never reinterpreted.
 b->lovel=64;pr_blend_prepare(f);assert(!a->prBlendHi&&!b->prBlendLo);b->lovel=65;a->group=1;pr_blend_prepare(f);assert(!a->prBlendHi&&!b->prBlendLo);a->group=0;pr_blend_prepare(f);
 float profile[264];pr_profile_estimate(f,0,profile);int i=69-21;assert(fabsf(profile[i]-442)<.3f&&profile[i+176]>.8f&&fabsf(profile[i+88]-2)<.08f);
 PrPianoResonance original,next;pr_res_init(&original,48000);pr_res_init(&next,48000);pr_res_profile(&next,profile);original.x[i*3]=.123f;pr_res_coefficients(&original,&next);assert(original.x[i*3]==.123f&&original.co[i*3]==next.co[i*3]);
 a->loop_mode=1;a->loop_start=100;a->loop_end=144000;b->loop_mode=1;b->loop_start=100;b->loop_end=144000;pr_profile_estimate(f,0,profile);assert(profile[i]>0&&profile[i+88]==0);tsf_close(f);
 // Real Java SFZ importer -> native PRB2: stereo fades and per-note rt_decay.
 tsf*bank=load_font(argv[1]);assert(bank&&bank->prBank&&bank->presets[0].regionNum==6);tsf_set_max_voices(bank,288);tsf_set_output(bank,TSF_STEREO_INTERLEAVED,48000,-10);tsf_channel_set_presetindex(bank,0,0);
 a=&bank->presets[0].regions[0];b=&bank->presets[0].regions[2];assert(fabsf(pr_region_gain(bank,a,60,60)-.75f)<1e-6&&fabsf(pr_region_gain(bank,b,60,60)-.5f)<1e-6);
 for(int n=0;n<4;n++){struct tsf_region*r=&bank->presets[0].regions[n];r->loop_mode=1;r->loop_start=r->offset+10;r->loop_end=r->end-2;}
 PrChannelFx*p=part_new(48000);bank->prContext=1;part_pedal(bank,p,127);smart_note(bank,60,50);advance(bank,48000);part_release(bank,p,60);smart_note(bank,60,90);advance(bank,12000);part_release(bank,p,60);assert(bank->prRelease[60]==2);advance(bank,24000);part_pedal(bank,p,0);assert(!bank->prRelease[60]);int first=0,second=0;
 for(int n=0;n<bank->voiceNum;n++){struct tsf_voice*v=&bank->voices[n];if(v->playingPreset<0||v->region->prTrigger!=1)continue;int velocity=v->ampenv.midiVelocity;float expected=-10+20*log10f(velocity/127.f)-6*(velocity==50?1.75f:.75f);assert(fabsf(v->noteGainDB-expected)<.02f);if(velocity==50)first++;if(velocity==90)second++;}assert(first==2&&second==2);silence(bank);assert(!bank->prRelease[60]);
 // A finished attack still owns its release; it must not release a newer repeat.
 tsf_channel_set_presetindex(bank,0,0);for(int n=0;n<4;n++)bank->presets[0].regions[n].loop_mode=0;smart_note(bank,60,50);advance(bank,4800);assert(!tsf_active_voice_count(bank)&&bank->prAttackCount[60]==1);smart_note(bank,60,90);part_release(bank,p,60);int alive=0;for(int n=0;n<bank->voiceNum;n++){struct tsf_voice*v=&bank->voices[n];if(v->playingPreset>=0&&!v->region->prTrigger&&v->ampenv.midiVelocity==90){assert(v->ampenv.segment<TSF_SEGMENT_RELEASE);alive++;}}assert(alive==2&&bank->prAttackCount[60]==1);part_release(bank,p,60);assert(!bank->prAttackCount[60]);silence(bank);assert(!bank->prAttackCount[60]);
 tsf_channel_set_presetindex(bank,0,0);for(int n=4;n<6;n++)bank->presets[0].regions[n].prRtDecay=0;smart_note(bank,60,50);advance(bank,4800);part_release(bank,p,60);int explicitZero=0;for(int n=0;n<bank->voiceNum;n++){struct tsf_voice*v=&bank->voices[n];if(v->playingPreset>=0&&v->region->prTrigger==1){assert(fabsf(v->noteGainDB-(-10+20*log10f(50/127.f)))<.02f);explicitZero++;}}assert(explicitZero==2);
 silence(bank);tsf_channel_set_presetindex(bank,0,0);for(int n=4;n<6;n++)bank->presets[0].regions[n].prRtPresent=0;smart_note(bank,60,50);advance(bank,4800);part_release(bank,p,60);for(int n=0;n<bank->voiceNum;n++){struct tsf_voice*v=&bank->voices[n];if(v->playingPreset>=0&&v->region->prTrigger==1)assert(v->noteGainDB< -10+20*log10f(50/127.f)-.1f);}
 free(p);tsf_close(bank);
 for(int rate=8000;rate<=96000;rate=rate==8000?44100:rate==44100?48000:rate==48000?96000:96001){PrMechanics m={0};float l,r;pr_mech_event(&m,0,60,127,0,rate);pr_mech_tick(&m,&l,&r);assert(l==0&&r==0);m.hammer=m.key=m.pedal=100;for(int n=0;n<10000;n++)pr_mech_event(&m,n%3,n%88+21,n%127+1,n/100.f,rate);double energy=0;for(int n=0;n<rate;n++){pr_mech_tick(&m,&l,&r);assert(isfinite(l)&&isfinite(r)&&fabsf(l)<.15f&&fabsf(r)<.15f);energy+=l*l+r*r;}assert(energy>0);pr_mech_tick(&m,&l,&r);assert(l==0&&r==0);pr_mech_clear(&m);}
 puts("PASS: both fade laws/widths, voice eligibility, overlap exclusion, sample tuning/decay estimates and loop fallback, state-preserving coefficient transfer, stereo PRB2 release FIFO/original velocity/rt_decay, bounded mechanics and bypass at multiple rates.");
}
