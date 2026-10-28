/* Optional Android AAudio backend, using the public API via libaaudio.so.
   Public ABI signatures match NDK AAudio.h. Loading at runtime preserves API26
   and the existing AudioTrack path; no Oboe dependency or fake system library.
   The callback allocates nothing, performs no JNI, and never waits on a lock. */
#ifndef PR_AAUDIO
#define PR_AAUDIO
extern void* dlopen(const char*,int);
extern void* dlsym(void*,const char*);
#include "audio_api.h"
static int engineBusy;
static void pr_lock(void){while(__atomic_exchange_n(&engineBusy,1,__ATOMIC_ACQUIRE)){} }
static void pr_unlock(void){__atomic_store_n(&engineBusy,0,__ATOMIC_RELEASE);}
static int pr_try(void){return !__atomic_exchange_n(&engineBusy,1,__ATOMIC_ACQUIRE);}
static PrApi api,legacyApi;static void* aaLib;static int requestedBackend=1;
typedef struct {AAudioStream* stream,*input;int inputId,inputError,inputShort,inputReads,inputWarm,inputRecovery;float inputLast[2],inputPeak;tsf*a,*b,*c,*d;FX*fx;float ga,gb,gc,gd,last[2];int rate,error,drops,recovery,timeMax,benchMax,deadlineMisses;unsigned long long timeTotal,callbackCount,framesTotal,meterLimited,meterTotal,meterInvalid;unsigned meterPeak[2],meterGain,layerSnapshot[4];int voiceSnapshot[8],tailSnapshot[3],doneSnapshot,meterCountdown,inputPeakSnapshot,phaseMax[4],phaseBench[4];} PrStream;
static PrStream nativeStream;
#include "performance_session.h"
static int api_load(void){
 if(requestedBackend>=2)return pr_oboe_api(&api);
 if(aaLib){api=legacyApi;return 1;}aaLib=dlopen("libaaudio.so",2);if(!aaLib)return 0;
#define SYM(field,name) api.field=(void*)dlsym(aaLib,name);if(!api.field)goto unavailable
 SYM(create,"AAudio_createStreamBuilder");SYM(destroy,"AAudioStreamBuilder_delete");
 SYM(device,"AAudioStreamBuilder_setDeviceId");SYM(direction,"AAudioStreamBuilder_setDirection");
 SYM(sharing,"AAudioStreamBuilder_setSharingMode");SYM(performance,"AAudioStreamBuilder_setPerformanceMode");
 SYM(channels,"AAudioStreamBuilder_setChannelCount");SYM(format,"AAudioStreamBuilder_setFormat");
 SYM(data,"AAudioStreamBuilder_setDataCallback");SYM(error,"AAudioStreamBuilder_setErrorCallback");
 SYM(open,"AAudioStreamBuilder_openStream");SYM(close,"AAudioStream_close");SYM(start,"AAudioStream_requestStart");SYM(stop,"AAudioStream_requestStop");
 SYM(rate,"AAudioStream_getSampleRate");SYM(size,"AAudioStream_getBufferSizeInFrames");SYM(capacity,"AAudioStream_getBufferCapacityInFrames");SYM(burst,"AAudioStream_getFramesPerBurst");SYM(mode,"AAudioStream_getSharingMode");SYM(perf,"AAudioStream_getPerformanceMode");SYM(xruns,"AAudioStream_getXRunCount");SYM(id,"AAudioStream_getDeviceId");SYM(ch,"AAudioStream_getChannelCount");SYM(fmt,"AAudioStream_getFormat");SYM(buffer,"AAudioStream_setBufferSizeInFrames");
 api.sampleRate=(void*)dlsym(aaLib,"AAudioStreamBuilder_setSampleRate");api.readInput=(void*)dlsym(aaLib,"AAudioStream_read");api.inputPreset=(void*)dlsym(aaLib,"AAudioStreamBuilder_setInputPreset");
 api.usage=(void*)dlsym(aaLib,"AAudioStreamBuilder_setUsage");legacyApi=api;return 1;
unavailable:aaLib=0;return 0;
#undef SYM
}
static void pr_rates(tsf*a,tsf*b,FX*f,int rate){if(a)tsf_set_output(a,TSF_STEREO_INTERLEAVED,rate,-10);if(b)tsf_set_output(b,TSF_STEREO_INTERLEAVED,rate,-10);if(f){f->rate=rate;part_rate(f->yamaha,rate);for(int i=0;i<4;i++)part_rate(f->parts[i],rate);for(int i=0;i<PR_TAILS;i++)if(f->tails[i].bank)part_rate(f->tails[i].part,rate);if(f->eq.rate!=rate)pr_eq_reset(&f->eq,rate);}}
static void aa_input_block(PrStream*s,int n){FX*f=s->fx;if(!f)return;float*b=f->yamahaInput;memset(b,0,n*2*sizeof(float));if(!s->input||!f->yamahaEnabled)return;int got=api.readInput(s->input,b,n,0);if(got<0){__atomic_store_n(&s->inputError,got,__ATOMIC_RELEASE);f->yamahaEnabled=0;return;}if(got>n)got=n;__atomic_add_fetch(&s->inputReads,got,__ATOMIC_RELAXED);if(got<n&&s->inputWarm>4)__atomic_add_fetch(&s->inputShort,1,__ATOMIC_RELAXED);s->inputWarm++;for(int i=0;i<got;i++)for(int c=0;c<2;c++){float v=b[i*2+c];if(!isfinite(v))v=0;if(v>1)v=1;if(v< -1)v=-1;if(s->inputRecovery>i){float t=(65-s->inputRecovery+i)/64.f;v=s->inputLast[c]*(1-t)+v*t;}b[i*2+c]=v;if(fabsf(v)>s->inputPeak)s->inputPeak=fabsf(v);}s->inputRecovery-=got;if(s->inputRecovery<0)s->inputRecovery=0;if(got<n)s->inputRecovery=64;for(int c=0;c<2;c++){float last=got?b[(got-1)*2+c]:s->inputLast[c];for(int i=got;i<n;i++){int missing=i-got;float fade=missing<64?1-(missing+1)/64.f:0;b[i*2+c]=last*fade;}s->inputLast[c]=n?b[(n-1)*2+c]:0;}}
/* UI/diagnostics read only these atomic snapshots. They never acquire the
   render lock, traverse mutable voices or reset DSP state during playback. */
static unsigned pr_float_bits(float value){unsigned bits;memcpy(&bits,&value,4);return bits;}
static float pr_bits_float(unsigned bits){float value;memcpy(&value,&bits,4);return value;}
static void pr_peak(unsigned*dest,float value){unsigned bits=pr_float_bits(value),old=__atomic_load_n(dest,__ATOMIC_RELAXED);for(int attempt=0;bits>old&&attempt<2;attempt++)if(__atomic_compare_exchange_n(dest,&old,bits,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED))break; }
static void aa_publish(PrStream*s,int frames){
 s->meterCountdown-=frames;if(s->meterCountdown>0)return;s->meterCountdown=s->rate/125;FX*f=s->fx;if(!f)return;
 pr_peak(&s->meterPeak[0],f->peakBefore);pr_peak(&s->meterPeak[1],f->peakAfter);__atomic_store_n(&s->meterGain,pr_float_bits(f->limiterGain),__ATOMIC_RELAXED);
 __atomic_add_fetch(&s->meterLimited,(unsigned long long)f->limitedSamples,__ATOMIC_RELAXED);__atomic_add_fetch(&s->meterTotal,(unsigned long long)f->totalSamples,__ATOMIC_RELAXED);__atomic_add_fetch(&s->meterInvalid,(unsigned long long)f->invalidSamples,__ATOMIC_RELAXED);f->peakBefore=f->peakAfter=0;f->limitedSamples=f->totalSamples=f->invalidSamples=0;
 tsf*banks[4]={s->a,s->b,s->c,s->d};int declicks=0,live=0,voices=0,done=0;
 for(int i=0;i<4;i++){__atomic_store_n(&s->voiceSnapshot[i],banks[i]?tsf_active_voice_count(banks[i]):0,__ATOMIC_RELAXED);if(banks[i])declicks+=banks[i]->prDeClicks;pr_peak(&s->layerSnapshot[i],f->layerPeak[i]);f->layerPeak[i]=0;}
 __atomic_store_n(&s->voiceSnapshot[4],smartSoft,__ATOMIC_RELAXED);__atomic_store_n(&s->voiceSnapshot[5],smartHard,__ATOMIC_RELAXED);__atomic_store_n(&s->voiceSnapshot[6],smartDropped,__ATOMIC_RELAXED);__atomic_store_n(&s->voiceSnapshot[7],declicks,__ATOMIC_RELAXED);
 for(int i=0;i<PR_TAILS;i++)if(f->tails[i].bank){if(f->tails[i].done)done++;else{live++;voices+=tsf_active_voice_count(f->tails[i].bank);}}
 __atomic_store_n(&s->tailSnapshot[0],live,__ATOMIC_RELAXED);__atomic_store_n(&s->tailSnapshot[1],voices,__ATOMIC_RELAXED);__atomic_store_n(&s->tailSnapshot[2],f->tailFades,__ATOMIC_RELAXED);__atomic_store_n(&s->doneSnapshot,done,__ATOMIC_RELAXED);__atomic_store_n(&s->inputPeakSnapshot,(int)(s->inputPeak*1000),__ATOMIC_RELAXED);
}
static void aa_phase(PrStream*s,int phase,long long elapsed){if(elapsed<0)elapsed=0;if(elapsed>1000000000)elapsed=1000000000;if(elapsed>__atomic_load_n(&s->phaseMax[phase],__ATOMIC_RELAXED))__atomic_store_n(&s->phaseMax[phase],(int)elapsed,__ATOMIC_RELAXED);if(elapsed>__atomic_load_n(&s->phaseBench[phase],__ATOMIC_RELAXED))__atomic_store_n(&s->phaseBench[phase],(int)elapsed,__ATOMIC_RELAXED);}
static int aa_data(AAudioStream*stream,void*user,void*out,int frames){
 pr_dsp_denormals();PrStream*s=user;float*dest=out;long long cpuBegin=pr_performance_begin();long long begin=pr_boottime();
 int owns=pr_try();short pcm[BLOCK*2];int offset=0;
 if(!owns){__atomic_add_fetch(&s->drops,1,__ATOMIC_RELAXED);s->recovery=64;for(int i=0;i<frames;i++){float fade=i<64?1-(i+1)/64.0f:0;for(int c=0;c<2;c++)dest[i*2+c]=s->last[c]*fade;}}
 else{FX*f=s->fx;int profile=(__atomic_load_n(&s->callbackCount,__ATOMIC_RELAXED)&15)==0;f->profileAudio=profile;f->sampleNanos=f->effectNanos=0;long long midiTime=0,inputTime=0;while(offset<frames){int n=frames-offset;if(n>BLOCK)n=BLOCK;tsf*banks[4]={s->a,s->b,s->c,s->d};float gains[4]={s->ga,s->gb,s->gc,s->gd};long long stamp=profile?pr_boottime():0;drain_midi(s->fx,banks);if(profile){long long now=pr_boottime();midiTime+=now-stamp;stamp=now;}aa_input_block(s,n);if(profile)inputTime+=pr_boottime()-stamp;render_mixer_float(banks,s->fx,gains,dest+offset*2,n);for(int i=0;i<n&&s->recovery>0;i++,s->recovery--){float t=(65-s->recovery)/64.0f;for(int c=0;c<2;c++)dest[(offset+i)*2+c]=s->last[c]*(1-t)+dest[(offset+i)*2+c]*t;}offset+=n;}if(profile){aa_phase(s,0,f->sampleNanos);aa_phase(s,1,f->effectNanos);aa_phase(s,2,midiTime);aa_phase(s,3,inputTime);}f->profileAudio=0;aa_publish(s,frames);pr_unlock();}
 if(frames>0){s->last[0]=dest[(frames-1)*2];s->last[1]=dest[(frames-1)*2+1];}
 if(__atomic_load_n(&recActive,__ATOMIC_ACQUIRE)||__atomic_load_n(&retroEnabled,__ATOMIC_ACQUIRE))for(offset=0;offset<frames;){int n=frames-offset;if(n>BLOCK)n=BLOCK;pcm16(dest+offset*2,pcm,n);rec_push(pcm,n,s->rate);retro_push(pcm,n,s->rate);offset+=n;}
 long long elapsed=pr_boottime()-begin;if(elapsed<0)elapsed=0;if(elapsed>1000000000LL)elapsed=1000000000LL;
 __atomic_add_fetch(&s->timeTotal,(unsigned long long)elapsed,__ATOMIC_RELAXED);__atomic_add_fetch(&s->callbackCount,1,__ATOMIC_RELAXED);__atomic_add_fetch(&s->framesTotal,(unsigned long long)frames,__ATOMIC_RELAXED);
 if(elapsed>__atomic_load_n(&s->timeMax,__ATOMIC_RELAXED))__atomic_store_n(&s->timeMax,(int)elapsed,__ATOMIC_RELAXED);
 if(elapsed>__atomic_load_n(&s->benchMax,__ATOMIC_RELAXED))__atomic_store_n(&s->benchMax,(int)elapsed,__ATOMIC_RELAXED);
 if(s->rate>0&&elapsed>(long long)frames*1000000000LL/s->rate)__atomic_add_fetch(&s->deadlineMisses,1,__ATOMIC_RELAXED);
 pr_performance_end(elapsed,frames,s->rate,cpuBegin);
 return 0;
}
static void aa_error(AAudioStream*stream,void*user,int error){PrStream*s=user;__atomic_store_n(&s->error,error,__ATOMIC_RELEASE);}
static int aa_buffer(int ms){PrStream*s=&nativeStream;if(!s->stream)return -1;int burst=api.burst(s->stream);if(burst<1)return -1;if(ms<2)ms=2;if(ms>120)ms=120;int frames=(s->rate*ms+999)/1000,min=burst*(ms<8?1:2);if(frames<min)frames=min;frames=(frames+burst-1)/burst*burst;return api.buffer(s->stream,frames);}
static void aa_input_stop(void){PrStream*s=&nativeStream;pr_lock();AAudioStream*input=s->input;s->input=0;if(s->fx){s->fx->yamahaEnabled=0;part_reset(s->fx->yamaha);memset(s->fx->yamahaInput,0,sizeof(s->fx->yamahaInput));}pr_unlock();if(input){api.stop(input);api.close(input);}}
static void aa_input_error(AAudioStream*stream,void*user,int error){PrStream*s=user;__atomic_store_n(&s->inputError,error,__ATOMIC_RELEASE);}
static int aa_input_start(int id){PrStream*s=&nativeStream;aa_input_stop();if(!s->stream||!s->fx||id<=0||!api.readInput||!api.sampleRate)return -1;AAudioStreamBuilder*b=0;AAudioStream*input=0;int result=api.create(&b);if(result<0||!b)return -2;api.device(b,id);api.direction(b,1);api.sharing(b,1);api.performance(b,12);api.channels(b,2);api.format(b,2);api.sampleRate(b,s->rate);if(api.inputPreset)api.inputPreset(b,9);api.error(b,aa_input_error,s);result=api.open(b,&input);api.destroy(b);if(result<0||!input)return result<0?result:-3;
 if(api.id(input)!=id||api.ch(input)!=2||api.fmt(input)!=2||api.rate(input)!=s->rate){api.close(input);return -4;}result=api.start(input);if(result<0){api.close(input);return result;}pr_lock();s->input=input;s->inputId=id;s->inputError=s->inputShort=s->inputReads=s->inputWarm=0;s->inputRecovery=64;s->inputPeak=s->inputLast[0]=s->inputLast[1]=0;part_reset(s->fx->yamaha);s->fx->yamahaEnabled=1;pr_unlock();return 0;
}
static void aa_stop(void){aa_input_stop();PrStream*s=&nativeStream;AAudioStream*stream=s->stream;if(stream){api.stop(stream);api.close(stream);s->stream=0;}pr_parallel_stop();pr_performance_close();pr_lock();if(s->fx){__atomic_store_n(&s->fx->queuedMidi,0,__ATOMIC_RELEASE);clear_midi(s->fx);}s->a=s->b=s->c=s->d=0;s->fx=0;pr_unlock();}
static int aa_start(tsf*a,tsf*b,FX*f,float ga,float gb,int id,int ms){
 aa_stop();if(!api_load())return -1;PrStream*s=&nativeStream;
 AAudioStreamBuilder*builder=0;int result=api.create(&builder);if(result<0||!builder)return result<0?result:-2;
 api.direction(builder,0);api.device(builder,id<0?0:id);api.sharing(builder,0);api.performance(builder,12);api.channels(builder,2);api.format(builder,2);if(api.usage)api.usage(builder,14);
 // Sample rate and callback frame count are deliberately left unspecified.
 api.data(builder,aa_data,s);api.error(builder,aa_error,s);
 result=api.open(builder,&s->stream);if(result<0){s->stream=0;api.sharing(builder,1);result=api.open(builder,&s->stream);}api.destroy(builder);
 if(result<0||!s->stream){s->stream=0;return result<0?result:-3;}
 int rate=api.rate(s->stream);if(rate<8000||rate>192000||api.ch(s->stream)!=2||api.fmt(s->stream)!=2){aa_stop();return -4;}
 pr_lock();s->a=a;s->b=b;s->fx=f;s->ga=ga;s->gb=gb;s->rate=rate;s->last[0]=s->last[1]=0;s->recovery=0;clear_midi(f);__atomic_store_n(&f->queuedMidi,1,__ATOMIC_RELEASE);__atomic_store_n(&s->error,0,__ATOMIC_RELEASE);__atomic_store_n(&s->drops,0,__ATOMIC_RELEASE);pr_rates(a,b,f,rate);pr_unlock();
 s->timeTotal=s->callbackCount=s->framesTotal=0;s->timeMax=s->deadlineMisses=0;memset(s->phaseMax,0,sizeof(s->phaseMax));memset(s->phaseBench,0,sizeof(s->phaseBench));memset(s->meterPeak,0,sizeof(s->meterPeak));memset(s->layerSnapshot,0,sizeof(s->layerSnapshot));s->meterLimited=s->meterTotal=s->meterInvalid=0;s->meterGain=pr_float_bits(1);s->meterCountdown=0;aa_publish(s,0);
 if(aa_buffer(ms)<0){aa_stop();return -5;}pr_parallel_start(requestedBackend==3);pr_performance_start();result=api.start(s->stream);if(result<0){aa_stop();return result;}return 0;
}
static void aa_layers(tsf*a,tsf*b,FX*f,float ga,float gb){pr_lock();PrStream*s=&nativeStream;if(s->stream&&s->fx==f){s->a=a;s->b=b;s->ga=ga;s->gb=gb;pr_rates(a,b,f,s->rate);}pr_unlock();}
static void aa_extra(tsf*c,tsf*d,float gc,float gd){pr_lock();PrStream*s=&nativeStream;s->c=c;s->d=d;s->gc=gc;s->gd=gd;if(s->stream){if(c)tsf_set_output(c,TSF_STEREO_INTERLEAVED,s->rate,-10);if(d)tsf_set_output(d,TSF_STEREO_INTERLEAVED,s->rate,-10);}pr_unlock();}
#endif
