#ifdef __ANDROID__
#include <sys/sysinfo.h>
#endif
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef PR_ENGINE
#define PR_ENGINE
typedef __SIZE_TYPE__ pr_size;
extern void *malloc(pr_size),*calloc(pr_size,pr_size),*realloc(void*,pr_size);
extern void free(void*);
extern void *memcpy(void*,const void*,pr_size),*memset(void*,int,pr_size);
extern double pow(double,double),log(double),tan(double),log10(double);
extern float powf(float,float),expf(float),sqrtf(float),sinf(float),fminf(float,float),fmaxf(float,float);
extern int open(const char*,int,...),close(int);
extern long read(int,void*,pr_size),lseek(int,long,int);
#define TSF_NO_STDIO
#define TSF_IMPLEMENTATION
#define TSF_PR_BANK
#define TSF_MALLOC malloc
#define TSF_REALLOC realloc
#define TSF_FREE free
#define TSF_MEMCPY memcpy
#define TSF_MEMSET memset
#define TSF_POW pow
#define TSF_POWF powf
#define TSF_EXPF expf
#define TSF_LOG log
#define TSF_TAN tan
#define TSF_LOG10 log10
#define TSF_SQRT sqrt
#define TSF_SQRTF sqrtf
#define TSF_SAMPLE_INTERPOLATE pr_interpolate
#include "tsf.h"
#include "parallel_sampler.h"
#include "packed_bank.h"
#include "tone_eq.h"
#include "record_tap.h"
#include "retro_tap.h"
#include "channel_fx.h"
#define RATE 44100
#define BLOCK 256
#define PR_MIDI_EVENTS 2048u
typedef struct{int type,a,b,mask;} PrMidiEvent;
#define PR_TAILS 32
typedef struct{tsf*bank;PrChannelFx*part;float gain,master,wet,feedback;unsigned serial;int done,channel,transpose;} PrTail;
typedef struct{unsigned owner[4];} PrHeld;
typedef struct{PrHeld keys[128][32];unsigned char keyHead[128],keyCount[128];PrChannelFx*yamaha;float yamahaInput[BLOCK*2],yamahaGain;int yamahaEnabled;unsigned char yamahaVelocity[128];unsigned long long yamahaFrame,yamahaOn[128];PrChannelFx*parts[4];tsf*currentBanks[4];PrTail tails[PR_TAILS];unsigned serial,tailFades;int transpose;float masterAnchor,tailScale;float comb[2][4][3000],damp[2][4],ap[2][2][800];int ci[2][4],ai[2][2];int profileAudio;long long sampleNanos,effectNanos;float layerPeak[4];float wet,feedback,master,clickVolume,phase;int tempo,beats,metro,beat,left;long clock,next;int protect,toneLeft,rate;float tonePhase,limiterGain,peakBefore,peakAfter;long limitedSamples,totalSamples,invalidSamples;PrMidiEvent events[PR_MIDI_EVENTS];unsigned eventRead,eventWrite;int queuedMidi,eventOverflow,eventPanic;PrToneEq eq;} FX;
static int fx_rate(FX*f){return f->rate>0?f->rate:RATE;}
static int rd(void*d,void*p,unsigned n){return (int)read(*(int*)d,p,n);}
static int sk(void*d,unsigned n){return lseek(*(int*)d,n,1)>=0;}
/* Read-ahead is bounded by currently free RAM; it never runs in the callback.
   Existing immutable sample sharing remains unchanged. Pages are reclaimable. */
static void pr_prepare_memory(tsf*f){
#ifdef __ANDROID__
 struct sysinfo info;if(!f||!f->fontSamples||sysinfo(&info))return;
 unsigned long long available=(unsigned long long)info.freeram*info.mem_unit;
 unsigned long long budget=available/8,cap=512ULL*1024*1024;if(budget>cap)budget=cap;
 unsigned long long bytes=(unsigned long long)f->sampleCount*sizeof(float);if(bytes>budget)bytes=budget;
 if(f->prMapping&&bytes){pr_size length=f->prMappingBytes;if(length>budget)length=(pr_size)budget;madvise(f->prMapping,length,MADV_WILLNEED);}
 volatile float touch=0;for(unsigned long long i=0;i<bytes/sizeof(float);i+=1024)touch=f->fontSamples[i];(void)touch;
 f->prPreparedBytes=bytes;
#endif
}
static tsf* load_font(const char*path){int fd=open(path,0);if(fd<0)return 0;char magic[4];tsf*f=0;if(bank_read(fd,magic,4)&&magic[0]=='P'&&magic[1]=='R'&&magic[2]=='B'&&(magic[3]=='1'||magic[3]=='2'||magic[3]=='3'||magic[3]=='4'||magic[3]=='5'||magic[3]=='6'||magic[3]=='7'))f=packed_bank_version(fd,magic[3]-'0');else{lseek(fd,0,0);struct tsf_stream s={&fd,rd,sk};f=tsf_load(&s);}close(fd);if(f){pr_prepare_memory(f);pr_blend_prepare(f);tsf_set_output(f,TSF_STEREO_INTERLEAVED,RATE,-10);f->refCount=malloc(sizeof(int));if(!f->refCount){tsf_close(f);return 0;}*f->refCount=1;}return f;}
static tsf* copy_font(tsf*base,int voices){tsf*f=tsf_copy(base);if(f&&!tsf_set_max_voices(f,voices+32)){tsf_close(f);return 0;}return f;}
static long smartSoft,smartHard,smartDropped;
static struct tsf_voice* quiet_group(tsf*f,int allowHeld){
 struct tsf_voice*best=0;int bestRank=9;float bestScore=1e30f;
 for(int i=0;i<f->voiceNum;i++){struct tsf_voice*v=&f->voices[i];if(v->playingPreset<0)continue;
  int rank=v->ampenv.segment>=TSF_SEGMENT_RELEASE?0:v->heldSustain?1:2;if(rank==2&&!allowHeld)continue;
  float level=v->ampenv.level*tsf_decibelsToGain(v->noteGainDB);unsigned age=f->voicePlayIndex-v->playIndex;float score=level/(1+age*.035f);
  if(rank<bestRank||(rank==bestRank&&score<bestScore)){best=v;bestRank=rank;bestScore=score;}
 }return best;
}
static int free_voices(tsf*f){int free=0;for(int i=0;i<f->voiceNum;i++)free+=f->voices[i].playingPreset<0;return free;}
static void retire_group(tsf*f,struct tsf_voice*v,int hard){unsigned group=v->playIndex;for(int i=0;i<f->voiceNum;i++){struct tsf_voice*x=&f->voices[i];if(x->playingPreset>=0&&x->playIndex==group){if(hard){tsf_voice_declick(f,x);tsf_voice_kill(x);}else tsf_voice_endquick(f,x);}}if(hard)smartHard++;else smartSoft++;}
static void smart_note(tsf*f,int key,int velocity){
 if(!f||!f->channels)return;int preset=f->channels->channels[0].presetIndex,need=0;
 if(preset<0||preset>=f->presetNum)return;
 for(int i=0;i<f->presets[preset].regionNum;i++){struct tsf_region*r=&f->presets[preset].regions[i];if(bank_region_match(f,r,key,velocity))need++;}
 if(need>f->voiceNum){smartDropped++;return;}
 int free=free_voices(f);
 // End a few pedal tails early, while reserve voices finish their short release.
 if(free<need+32){struct tsf_voice*best=0;float score=1e30f;for(int i=0;i<f->voiceNum;i++){struct tsf_voice*v=&f->voices[i];if(v->playingPreset<0||!v->heldSustain||v->ampenv.segment>=TSF_SEGMENT_RELEASE)continue;float q=v->ampenv.level*tsf_decibelsToGain(v->noteGainDB)/(1+(f->voicePlayIndex-v->playIndex)*.035f);if(q<score){score=q;best=v;}}if(best)retire_group(f,best,0);}
 while(free<need){struct tsf_voice*v=quiet_group(f,1);if(!v){smartDropped++;return;}retire_group(f,v,1);free=free_voices(f);}
 /* Repetition refinement only ducks previously released groups of this key.
    Held keys, author-defined release decay and FIFO ownership remain intact. */
 if(f->prRepeat)for(int i=0;i<f->voiceNum;i++){struct tsf_voice*v=&f->voices[i];if(v->playingPreset>=0&&v->playingKey==key&&(v->prKeyReleased||v->heldSustain)&&!v->region->prTrigger){float reduction=1-f->prRepeat*.006f;if(reduction<.4f)reduction=.4f;v->prRepeatTarget=fmaxf(.12f,v->prRepeatTarget*reduction);}}
 tsf_channel_note_on(f,0,key,velocity/127.0f);
}
static void config_fx(FX*f,float wet,int room,float master,int metro,int tempo,int beats,float vol){if(f->limiterGain<=0)f->limiterGain=1;f->wet=wet;f->feedback=room==0?.62f:room==1?.77f:room==2?.85f:.71f;f->master=master;f->tempo=tempo;f->beats=beats;f->clickVolume=vol;if(metro&&!f->metro){f->next=f->clock;f->beat=0;}f->metro=metro;}
static void silence(tsf*f){if(f){bank_clear_context(f);tsf_reset(f);for(int i=0;i<f->voiceNum;i++)tsf_voice_kill(&f->voices[i]);}}
static void clear_midi(FX*f){if(f){__atomic_store_n(&f->eventRead,__atomic_load_n(&f->eventWrite,__ATOMIC_ACQUIRE),__ATOMIC_RELEASE);__atomic_store_n(&f->eventPanic,0,__ATOMIC_RELEASE);}}
static void apply_event(tsf**banks,PrMidiEvent ev){for(int i=0;i<4;i++){tsf*f=banks[i];if(!f||!(ev.mask&(1<<i)))continue;if(ev.type==0x90&&ev.b)smart_note(f,ev.a,ev.b);else if(ev.type==0x90||ev.type==0x80)tsf_channel_note_off(f,0,ev.a);else if(ev.type==0xB0&&(ev.a==64||ev.a==120||ev.a==121||ev.a==123))tsf_channel_midi_control(f,0,ev.a,ev.b);else if(ev.type==0xB0&&f->prBank&&(ev.a==7||ev.a==10||ev.a==11||ev.a==39||ev.a==42||ev.a==43||(ev.a>=0&&ev.a<128&&(f->prVelCCMask[ev.a]||f->prCCMask[ev.a]))))tsf_channel_midi_control(f,0,ev.a,ev.b);else if(ev.type==0xE0)tsf_channel_set_pitchwheel(f,0,ev.a|(ev.b<<7));}}
#include "scene_engine.h"
/* Java serializes producers. The audio callback is the only consumer. */
static void enqueue_midi(FX*f,PrMidiEvent ev){unsigned w=__atomic_load_n(&f->eventWrite,__ATOMIC_RELAXED),r=__atomic_load_n(&f->eventRead,__ATOMIC_ACQUIRE);if(w-r>=PR_MIDI_EVENTS){__atomic_add_fetch(&f->eventOverflow,1,__ATOMIC_RELAXED);__atomic_store_n(&f->eventPanic,1,__ATOMIC_RELEASE);return;}f->events[w&(PR_MIDI_EVENTS-1)]=ev;__atomic_store_n(&f->eventWrite,w+1,__ATOMIC_RELEASE);}
static void drain_midi(FX*f,tsf**banks){if(__atomic_exchange_n(&f->eventPanic,0,__ATOMIC_ACQ_REL)){for(int i=0;i<4;i++){tsf*b=banks[i];if(!b)continue;for(int j=0;j<b->voiceNum;j++)tsf_voice_kill(&b->voices[j]);if(b->channels){tsf_channel_set_sustain(b,0,0);tsf_channel_set_pitchwheel(b,0,8192);}}scene_all_silent(f);clear_midi(f);return;}unsigned r=__atomic_load_n(&f->eventRead,__ATOMIC_RELAXED),w=__atomic_load_n(&f->eventWrite,__ATOMIC_ACQUIRE);unsigned n=w-r;if(n>64)n=64;for(unsigned i=0;i<n;i++)scene_event(f,banks,f->events[(r+i)&(PR_MIDI_EVENTS-1)]);__atomic_store_n(&f->eventRead,r+n,__ATOMIC_RELEASE);}
static void panic_fx(tsf*a,tsf*b,FX*f){silence(a);silence(b);scene_all_silent(f);clear_midi(f);memset(f->comb,0,sizeof(f->comb));memset(f->damp,0,sizeof(f->damp));memset(f->ap,0,sizeof(f->ap));memset(f->ci,0,sizeof(f->ci));memset(f->ai,0,sizeof(f->ai));f->limiterGain=1;f->toneLeft=0;f->clock=f->next=0;pr_eq_reset(&f->eq,fx_rate(f));}
/* Stereo-linked limiter, smoothed attack/release and a continuous soft knee.
   Block peak supplies lookahead without buffering another block. */
static void output_audio_float(FX*f,const float*mix,float*out,int frames){
 float peak=0;for(int i=0;i<frames*2;i++){float v=mix[i];if(v==v&&v<1e8f&&v>-1e8f){float a=v<0?-v:v;if(a>peak)peak=a;}}
 if(peak>f->peakBefore)f->peakBefore=peak;
 float want=f->protect&&peak>.94f?.94f/peak:1;
 for(int i=0;i<frames;i++){
  f->limiterGain+=(want-f->limiterGain)*(want<f->limiterGain?.04f:10.0f/fx_rate(f));
  float stereoPeak=0;for(int c=0;c<2;c++){float v=mix[i*2+c];if(v==v&&v<1e8f&&v>-1e8f){float a=v<0?-v:v;if(a>stereoPeak)stereoPeak=a;}}
  float frameGain=f->protect?f->limiterGain:1,linkedPeak=stereoPeak*frameGain;
  if(f->protect&&linkedPeak>.88f){float over=linkedPeak-.88f;frameGain*=(.88f+.06f*over/(.06f+over))/linkedPeak;}
  for(int c=0;c<2;c++){float v=mix[i*2+c];if(v!=v||v>=1e8f||v<=-1e8f){v=0;f->invalidSamples++;}float y=v*(f->protect?f->limiterGain:1);
   y=v*frameGain;if(f->protect&&frameGain<.999f&&v!=0)f->limitedSamples++;
   if(y>.98f)y=.98f;if(y<-.98f)y=-.98f;
   float a=y<0?-y:y;if(a>f->peakAfter)f->peakAfter=a;
   out[i*2+c]=y;f->totalSamples++;
  }
 }
}
static void pcm16(const float*src,short*out,int frames){for(int i=0;i<frames*2;i++)out[i]=(short)(src[i]*32767);}
static void output_audio(FX*f,const float*mix,short*out,int frames){float converted[BLOCK*2];output_audio_float(f,mix,converted,frames);pcm16(converted,out,frames);rec_push(out,frames,fx_rate(f));retro_push(out,frames,fx_rate(f));}
static void render_mixer_float(tsf**banks,FX*f,const float*gains,float*out,int frames){
 pr_dsp_denormals();
 float aa[BLOCK*2]={0},bb[BLOCK*2]={0},mix[BLOCK*2]={0};
 if(f->toneLeft){for(int i=0;i<frames;i++){if(f->toneLeft>0){int elapsed=fx_rate(f)*2-f->toneLeft;float fade=elapsed<220?elapsed/220.0f:f->toneLeft<220?f->toneLeft/220.0f:1;float y=sinf(f->tonePhase)*.12f*fade*f->master;f->tonePhase+=6.2831853f*440/fx_rate(f);if(f->tonePhase>6.2831853f)f->tonePhase-=6.2831853f;mix[i*2]=mix[i*2+1]=y;f->toneLeft--;}}output_audio_float(f,mix,out,frames);return;}
 if(f->parts[0]){scene_render(f,banks,gains,mix,frames);if(f->yamahaEnabled&&f->yamaha){long long stamp=f->profileAudio?pr_boottime():0;part_process(f->yamaha,f->yamahaInput,mix,frames,f->yamahaGain,f->master,f->wet,f->feedback);if(f->profileAudio)f->effectNanos+=pr_boottime()-stamp;f->yamahaFrame+=frames;}for(int i=0;i<frames;i++){float click=0;if(f->metro&&f->clock>=f->next){f->left=700;f->phase=0;f->beat=(f->beat+1)%f->beats;f->next=f->clock+(long)(fx_rate(f)*60.0/f->tempo);}if(f->left){f->phase+=6.2831853f*(f->beat==1?1600:1000)/fx_rate(f);click=sinf(f->phase)*f->left/700.0f*f->clickVolume*.22f*f->master;f->left--;}f->clock++;mix[i*2]+=click;mix[i*2+1]+=click;}pr_eq_process(&f->eq,mix,frames);output_audio_float(f,mix,out,frames);return;}
 for(int l=0;l<4;l++)if(banks[l]){tsf_render_float(banks[l],bb,frames,0);if(gains[l]>0)for(int i=0;i<frames*2;i++){float y=bb[i]*gains[l];aa[i]+=y;float peak=y<0?-y:y;peak*=f->master;if(peak>f->layerPeak[l])f->layerPeak[l]=peak;}}
 const int lens[4]={1557,1617,1491,1422},al[2]={225,556};for(int i=0;i<frames;i++){
 float click=0;if(f->metro&&f->clock>=f->next){f->left=700;f->phase=0;f->beat=(f->beat+1)%f->beats;f->next=f->clock+(long)(fx_rate(f)*60.0/f->tempo);}if(f->left){float freq=(f->beat==1||f->beats==1)?1600:1000;f->phase+=6.2831853f*freq/fx_rate(f);click=sinf(f->phase)*(f->left/700.0f)*f->clickVolume*.22f;f->left--;}f->clock++;
 for(int c=0;c<2;c++){float x=aa[i*2+c],rv=0;for(int k=0;k<4;k++){int p=f->ci[c][k],len=lens[k]+c*23;float y=f->comb[c][k][p];f->damp[c][k]=y*.65f+f->damp[c][k]*.35f;f->comb[c][k][p]=x*.22f+f->damp[c][k]*f->feedback;f->ci[c][k]=(p+1)%len;rv+=y*.25f;}for(int k=0;k<2;k++){int p=f->ai[c][k],len=al[k]+c*17;float y=f->ap[c][k][p],v=rv;rv=y-v;f->ap[c][k][p]=v+y*.5f;f->ai[c][k]=(p+1)%len;}mix[i*2+c]=(x+rv*f->wet*1.3f+click)*f->master;}
 }
 pr_eq_process(&f->eq,mix,frames);output_audio_float(f,mix,out,frames);
}
static void render_mixer(tsf**banks,FX*f,const float*gains,short*out,int frames){float converted[BLOCK*2];render_mixer_float(banks,f,gains,converted,frames);pcm16(converted,out,frames);rec_push(out,frames,fx_rate(f));retro_push(out,frames,fx_rate(f));}
static void render_audio(tsf*a,tsf*b,FX*f,float ga,float gb,short*out,int frames){tsf*banks[4]={a,b,0,0};float gains[4]={ga,gb,0,0};render_mixer(banks,f,gains,out,frames);}
#endif
