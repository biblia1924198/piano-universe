#include <jni.h>
#include "engine.h"
#include "aaudio_backend.h"
#define J(n) Java_com_luci_pianoroom_Synth_##n
JNIEXPORT jlong JNICALL J(load)(JNIEnv*e,jclass c,jstring s){const char*p=(*e)->GetStringUTFChars(e,s,0);tsf*f=load_font(p);(*e)->ReleaseStringUTFChars(e,s,p);return (jlong)f;}
JNIEXPORT jlong JNICALL J(copy)(JNIEnv*e,jclass c,jlong h,jint voices){pr_lock();tsf*f=tsf_copy((tsf*)h);pr_unlock();if(f&&!tsf_set_max_voices(f,voices+32)){tsf_close(f);f=0;}return (jlong)f;}
JNIEXPORT void JNICALL J(freeBank)(JNIEnv*e,jclass c,jlong h){if(h)tsf_close((tsf*)h);}
JNIEXPORT jobjectArray JNICALL J(names)(JNIEnv*e,jclass c,jlong h){tsf*f=(tsf*)h;int n=tsf_get_presetcount(f);jclass s=(*e)->FindClass(e,"java/lang/String");jobjectArray a=(*e)->NewObjectArray(e,n,s,0);for(int i=0;i<n;i++){jstring t=(*e)->NewStringUTF(e,tsf_get_presetname(f,i));(*e)->SetObjectArrayElement(e,a,i,t);(*e)->DeleteLocalRef(e,t);}return a;}
JNIEXPORT void JNICALL J(preset)(JNIEnv*e,jclass c,jlong h,jint p){pr_lock();if(h){silence((tsf*)h);tsf_channel_set_presetindex((tsf*)h,0,p);}pr_unlock();}
JNIEXPORT void JNICALL J(voices)(JNIEnv*e,jclass c,jlong h,jint n){pr_lock();if(h){tsf*f=(tsf*)h;silence(f);if(tsf_set_max_voices(f,n+32))f->voiceNum=f->maxVoiceNum=n+32;}pr_unlock();}
JNIEXPORT void JNICALL J(event)(JNIEnv*e,jclass c,jlong h,jint s,jint a,jint b){tsf*f=(tsf*)h;if(!f)return;pr_lock();if(s==0x90&&b)smart_note(f,a,b);else if(s==0x90||s==0x80)tsf_channel_note_off(f,0,a);else if(s==0xB0&&(a==64||a==120||a==121||a==123))tsf_channel_midi_control(f,0,a,b);else if(s==0xB0&&f->prBank&&(a==7||a==10||a==11||a==39||a==42||a==43||(a>=0&&a<128&&(f->prVelCCMask[a]||f->prCCMask[a]))))tsf_channel_midi_control(f,0,a,b);else if(s==0xE0)tsf_channel_set_pitchwheel(f,0,a|(b<<7));pr_unlock();}
JNIEXPORT void JNICALL J(midiEvent)(JNIEnv*e,jclass c,jlong h,jlong a,jlong b,jlong cc,jlong d,jint type,jint key,jint value,jint mask){FX*f=(FX*)h;if(!f)return;PrMidiEvent ev={type,key,value,mask};if(__atomic_load_n(&f->queuedMidi,__ATOMIC_ACQUIRE)){enqueue_midi(f,ev);return;}tsf*banks[4]={(tsf*)a,(tsf*)b,(tsf*)cc,(tsf*)d};pr_lock();scene_event(f,banks,ev);pr_unlock();}
JNIEXPORT void JNICALL J(midiFlush)(JNIEnv*e,jclass c,jlong h,jlong a,jlong b,jlong cc,jlong d){FX*f=(FX*)h;tsf*banks[4]={(tsf*)a,(tsf*)b,(tsf*)cc,(tsf*)d};pr_lock();if(f)while(__atomic_load_n(&f->eventRead,__ATOMIC_ACQUIRE)!=__atomic_load_n(&f->eventWrite,__ATOMIC_ACQUIRE)||__atomic_load_n(&f->eventPanic,__ATOMIC_ACQUIRE))drain_midi(f,banks);pr_unlock();}
JNIEXPORT jintArray JNICALL J(midiQueueStats)(JNIEnv*e,jclass c,jlong h){FX*f=(FX*)h;jint v[2]={0};if(f){v[0]=__atomic_load_n(&f->eventOverflow,__ATOMIC_RELAXED);v[1]=__atomic_load_n(&f->eventWrite,__ATOMIC_ACQUIRE)-__atomic_load_n(&f->eventRead,__ATOMIC_ACQUIRE);}jintArray r=(*e)->NewIntArray(e,2);if(r)(*e)->SetIntArrayRegion(e,r,0,2,v);return r;}
JNIEXPORT jlong JNICALL J(newFx)(JNIEnv*e,jclass c){FX*f=calloc(1,sizeof(FX));if(f){config_fx(f,.25f,1,.8f,0,120,4,.6f);f->protect=1;f->masterAnchor=f->master;f->tailScale=1;f->yamaha=part_new(RATE);f->yamahaGain=.5f;if(!scene_init(f)||!f->yamaha){free(f->yamaha);for(int i=0;i<4;i++)free(f->parts[i]);free(f);f=0;}}return (jlong)f;}
JNIEXPORT void JNICALL J(freeFx)(JNIEnv*e,jclass c,jlong h){FX*f=(FX*)h;if(f){free(f->yamaha);for(int i=0;i<4;i++)free(f->parts[i]);for(int i=0;i<PR_TAILS;i++){if(f->tails[i].bank)tsf_close(f->tails[i].bank);free(f->tails[i].part);}free(f);}}
JNIEXPORT void JNICALL J(config)(JNIEnv*e,jclass c,jlong h,jfloat w,jint r,jfloat m,jboolean mt,jint t,jint bt,jfloat v){pr_lock();if(h)config_fx((FX*)h,w,r,m,mt,t,bt,v);pr_unlock();}
JNIEXPORT void JNICALL J(panic)(JNIEnv*e,jclass c,jlong a,jlong b,jlong f){pr_lock();if(f){panic_fx((tsf*)a,(tsf*)b,(FX*)f);if(nativeStream.stream&&nativeStream.fx==(FX*)f){nativeStream.meterCountdown=0;aa_publish(&nativeStream,0);}}pr_unlock();}
JNIEXPORT void JNICALL J(render)(JNIEnv*e,jclass c,jlong a,jlong b,jlong f,jfloat ga,jfloat gb,jshortArray pcm,jint n){if(!f||n<1||n>BLOCK)return;jshort*p=(*e)->GetShortArrayElements(e,pcm,0);if(p){render_audio((tsf*)a,(tsf*)b,(FX*)f,ga,gb,p,n);(*e)->ReleaseShortArrayElements(e,pcm,p,0);}}

JNIEXPORT void JNICALL J(renderDirect)(JNIEnv*e,jclass c,jlong a,jlong b,jlong cc,jlong d,jlong f,jfloat ga,jfloat gb,jfloat gc,jfloat gd,jobject pcm,jint n){if(!f||n<1||n>BLOCK)return;pr_lock();short*p=(*e)->GetDirectBufferAddress(e,pcm);jlong size=(*e)->GetDirectBufferCapacity(e,pcm);if(p&&size>=n*4){tsf*banks[4]={(tsf*)a,(tsf*)b,(tsf*)cc,(tsf*)d};float gains[4]={ga,gb,gc,gd};render_mixer(banks,(FX*)f,gains,p,n);}pr_unlock();}
JNIEXPORT void JNICALL J(protection)(JNIEnv*e,jclass c,jlong h,jboolean on){pr_lock();if(h){((FX*)h)->protect=on;((FX*)h)->limiterGain=1;}pr_unlock();}
JNIEXPORT void JNICALL J(tone)(JNIEnv*e,jclass c,jlong h){pr_lock();if(h){((FX*)h)->toneLeft=fx_rate((FX*)h)*2;((FX*)h)->tonePhase=0;}pr_unlock();}
JNIEXPORT jfloatArray JNICALL J(levels)(JNIEnv*e,jclass c,jlong h){jfloat v[5]={0,0,0,1,0};FX*f=(FX*)h;PrStream*s=&nativeStream;if(f&&s->stream&&s->fx==f){v[0]=pr_bits_float(__atomic_exchange_n(&s->meterPeak[0],0,__ATOMIC_RELAXED));v[1]=pr_bits_float(__atomic_exchange_n(&s->meterPeak[1],0,__ATOMIC_RELAXED));unsigned long long limited=__atomic_exchange_n(&s->meterLimited,0,__ATOMIC_RELAXED),total=__atomic_exchange_n(&s->meterTotal,0,__ATOMIC_RELAXED);v[2]=total?fminf(100,limited*100.f/total):0;v[3]=pr_bits_float(__atomic_load_n(&s->meterGain,__ATOMIC_RELAXED));v[4]=__atomic_exchange_n(&s->meterInvalid,0,__ATOMIC_RELAXED);}else{pr_lock();if(f){v[0]=f->peakBefore;v[1]=f->peakAfter;v[2]=f->totalSamples?f->limitedSamples*100.0f/f->totalSamples:0;v[3]=f->limiterGain;v[4]=f->invalidSamples;f->peakBefore=f->peakAfter=0;f->limitedSamples=f->totalSamples=f->invalidSamples=0;}pr_unlock();}jfloatArray out=(*e)->NewFloatArray(e,5);if(out)(*e)->SetFloatArrayRegion(e,out,0,5,v);return out;}

JNIEXPORT jint JNICALL J(nativeOpen)(JNIEnv*e,jclass c,jlong a,jlong b,jlong f,jfloat ga,jfloat gb,jint id,jint ms){return aa_start((tsf*)a,(tsf*)b,(FX*)f,ga,gb,id,ms);}
JNIEXPORT void JNICALL J(nativeStop)(JNIEnv*e,jclass c){aa_stop();}
JNIEXPORT jint JNICALL J(nativeBuffer)(JNIEnv*e,jclass c,jint ms){return aa_buffer(ms);}
JNIEXPORT void JNICALL J(nativeLayers)(JNIEnv*e,jclass c,jlong a,jlong b,jlong f,jfloat ga,jfloat gb){aa_layers((tsf*)a,(tsf*)b,(FX*)f,ga,gb);}
JNIEXPORT void JNICALL J(nativeRate)(JNIEnv*e,jclass c,jlong a,jlong b,jlong f,jint rate){pr_lock();pr_rates((tsf*)a,(tsf*)b,(FX*)f,rate);pr_unlock();}
JNIEXPORT jintArray JNICALL J(nativeStats)(JNIEnv*e,jclass c){PrStream*s=&nativeStream;jint v[18]={0};if(s->stream){v[0]=s->rate;v[1]=api.size(s->stream);v[2]=api.capacity(s->stream);v[3]=api.burst(s->stream);v[4]=api.mode(s->stream);v[5]=api.perf(s->stream);v[6]=api.xruns(s->stream);v[7]=api.id(s->stream);v[8]=__atomic_load_n(&s->error,__ATOMIC_ACQUIRE);v[9]=__atomic_load_n(&s->drops,__ATOMIC_RELAXED);v[10]=__atomic_load_n(&s->timeMax,__ATOMIC_RELAXED);unsigned long long count=__atomic_load_n(&s->callbackCount,__ATOMIC_RELAXED),total=__atomic_load_n(&s->timeTotal,__ATOMIC_RELAXED),frames=__atomic_load_n(&s->framesTotal,__ATOMIC_RELAXED);v[11]=count?total/count:0;v[12]=__atomic_load_n(&s->deadlineMisses,__ATOMIC_RELAXED);v[13]=frames?(jint)((double)total*s->rate/frames/1000000.0):0;}for(int i=0;i<4;i++)v[14+i]=__atomic_load_n(&s->phaseMax[i],__ATOMIC_RELAXED);jintArray out=(*e)->NewIntArray(e,18);if(out)(*e)->SetIntArrayRegion(e,out,0,18,v);return out;}

JNIEXPORT void JNICALL J(nativeExtra)(JNIEnv*e,jclass c,jlong a,jlong b,jfloat ga,jfloat gb){aa_extra((tsf*)a,(tsf*)b,ga,gb);}
JNIEXPORT void JNICALL J(recordStart)(JNIEnv*e,jclass c,jint rate){pr_lock();rec_start(rate);pr_unlock();}
JNIEXPORT void JNICALL J(recordStop)(JNIEnv*e,jclass c){pr_lock();rec_stop();pr_unlock();}
JNIEXPORT jint JNICALL J(recordRead)(JNIEnv*e,jclass c,jobject buffer,jint frames){short*p=(*e)->GetDirectBufferAddress(e,buffer);long size=(*e)->GetDirectBufferCapacity(e,buffer);return p&&size>=frames*4?rec_pull(p,frames):0;}
JNIEXPORT jlong JNICALL J(recordFirstNanos)(JNIEnv*e,jclass c){return __atomic_load_n(&recFirst,__ATOMIC_ACQUIRE);}
JNIEXPORT jint JNICALL J(recordFailure)(JNIEnv*e,jclass c){return __atomic_load_n(&recFailure,__ATOMIC_ACQUIRE);}
JNIEXPORT jintArray JNICALL J(voiceStats)(JNIEnv*e,jclass c,jlong a,jlong b,jlong cc,jlong d){tsf*banks[4]={(tsf*)a,(tsf*)b,(tsf*)cc,(tsf*)d};jint v[8]={0};PrStream*s=&nativeStream;if(s->stream&&s->a==banks[0]&&s->b==banks[1]&&s->c==banks[2]&&s->d==banks[3]){for(int i=0;i<8;i++)v[i]=__atomic_load_n(&s->voiceSnapshot[i],__ATOMIC_RELAXED);}else{pr_lock();for(int i=0;i<4;i++)if(banks[i]){v[i]=tsf_active_voice_count(banks[i]);v[7]+=banks[i]->prDeClicks;}v[4]=smartSoft;v[5]=smartHard;v[6]=smartDropped;pr_unlock();}jintArray r=(*e)->NewIntArray(e,8);if(r)(*e)->SetIntArrayRegion(e,r,0,8,v);return r;}

JNIEXPORT jintArray JNICALL J(presetInfo)(JNIEnv*e,jclass c,jlong h){tsf*f=(tsf*)h;int n=f?f->presetNum:0;jintArray r=(*e)->NewIntArray(e,n*2);for(int i=0;i<n;i++){jint v[2]={f->presets[i].bank,f->presets[i].preset};(*e)->SetIntArrayRegion(e,r,i*2,2,v);}return r;}

JNIEXPORT jfloatArray JNICALL J(mixerLevels)(JNIEnv*e,jclass c,jlong h){FX*f=(FX*)h;jfloat v[4]={0};PrStream*s=&nativeStream;if(f&&s->stream&&s->fx==f){for(int i=0;i<4;i++)v[i]=pr_bits_float(__atomic_exchange_n(&s->layerSnapshot[i],0,__ATOMIC_RELAXED));}else{pr_lock();if(f)for(int i=0;i<4;i++){v[i]=f->layerPeak[i];f->layerPeak[i]=0;}pr_unlock();}jfloatArray r=(*e)->NewFloatArray(e,4);if(r)(*e)->SetFloatArrayRegion(e,r,0,4,v);return r;}

JNIEXPORT void JNICALL J(sampleQuality)(JNIEnv*e,jclass c,jlong a,jlong b,jlong cc,jlong d,jint quality){pr_lock();tsf*banks[4]={(tsf*)a,(tsf*)b,(tsf*)cc,(tsf*)d};for(int i=0;i<4;i++)if(banks[i])banks[i]->sampleQuality=quality==1;pr_unlock();}
JNIEXPORT void JNICALL J(equalizer)(JNIEnv*e,jclass c,jlong h,jboolean enabled,jfloat low,jfloat mid,jfloat high){pr_lock();FX*f=(FX*)h;if(f)pr_eq_request(&f->eq,enabled,low,mid,high,fx_rate(f));pr_unlock();}
JNIEXPORT jintArray JNICALL J(presetQuality)(JNIEnv*e,jclass c,jlong h,jint preset){jint v[4];pr_preset_quality((tsf*)h,preset,v);jintArray r=(*e)->NewIntArray(e,4);if(r)(*e)->SetIntArrayRegion(e,r,0,4,v);return r;}

/* Four prepared banks are committed together; old copies become tail owners. */
JNIEXPORT jboolean JNICALL J(sceneSwap)(JNIEnv*e,jclass c,jlong h,jlongArray oldArray,jlongArray newArray,jfloatArray gains,jint transpose,jfloat master){
 FX*f=(FX*)h;jlong old[4],next[4];jfloat g[4];if(!f||(*e)->GetArrayLength(e,oldArray)!=4||(*e)->GetArrayLength(e,newArray)!=4||(*e)->GetArrayLength(e,gains)!=4)return 0;
 (*e)->GetLongArrayRegion(e,oldArray,0,4,old);(*e)->GetLongArrayRegion(e,newArray,0,4,next);(*e)->GetFloatArrayRegion(e,gains,0,4,g);PrChannelFx*p[4]={0};for(int i=0;i<4;i++)if(old[i]!=next[i]){p[i]=part_new(fx_rate(f));if(!p[i]){for(int j=0;j<4;j++)free(p[j]);return 0;}}
 pr_lock();int count=0;for(int i=0;i<4;i++)count+=old[i]&&old[i]!=next[i];if(!scene_slots(f,count)){pr_unlock();for(int i=0;i<4;i++)free(p[i]);return 0;}
 for(int i=0;i<PR_TAILS;i++)if(f->tails[i].bank)f->tails[i].master*=f->tailScale;f->tailScale=1;
 for(int i=0;i<4;i++){if(old[i]!=next[i]){p[i]->pedal=f->parts[i]->pedal;scene_retire(f,(tsf*)old[i],f->parts[i],i,g[i]);p[i]->owner=++f->serial;f->parts[i]=p[i];part_rate(p[i],fx_rate(f));}else free(p[i]);f->parts[i]->transpose=transpose;f->currentBanks[i]=(tsf*)next[i];}
 f->transpose=transpose;f->masterAnchor=master;f->master=master;if(nativeStream.stream&&nativeStream.fx==f){nativeStream.a=(tsf*)next[0];nativeStream.b=(tsf*)next[1];nativeStream.c=(tsf*)next[2];nativeStream.d=(tsf*)next[3];for(int i=0;i<4;i++)if(next[i])tsf_set_output((tsf*)next[i],TSF_STEREO_INTERLEAVED,nativeStream.rate,-10);nativeStream.meterCountdown=0;aa_publish(&nativeStream,0);}
 pr_unlock();return 1;
}
typedef struct{tsf*b[PR_TAILS];PrChannelFx*p[PR_TAILS];} PrGarbage;
JNIEXPORT jlong JNICALL J(tailDetach)(JNIEnv*e,jclass c,jlong h){FX*f=(FX*)h;if(!f)return 0;if(nativeStream.stream&&nativeStream.fx==f&&!__atomic_load_n(&nativeStream.doneSnapshot,__ATOMIC_RELAXED))return 0;PrGarbage*g=calloc(1,sizeof(*g));if(!g)return 0;int count=0;pr_lock();for(int i=0;i<PR_TAILS;i++)if(f->tails[i].bank&&f->tails[i].done){g->b[i]=f->tails[i].bank;g->p[i]=f->tails[i].part;memset(&f->tails[i],0,sizeof(PrTail));count++;}if(nativeStream.stream&&nativeStream.fx==f){nativeStream.meterCountdown=0;aa_publish(&nativeStream,0);}pr_unlock();if(!count){free(g);return 0;}return (jlong)g;}
JNIEXPORT void JNICALL J(tailFree)(JNIEnv*e,jclass c,jlong h){PrGarbage*g=(PrGarbage*)h;if(!g)return;for(int i=0;i<PR_TAILS;i++){if(g->b[i])tsf_close(g->b[i]);free(g->p[i]);}free(g);}
JNIEXPORT jintArray JNICALL J(tailStats)(JNIEnv*e,jclass c,jlong h){FX*f=(FX*)h;jint v[3]={0};PrStream*s=&nativeStream;if(f&&s->stream&&s->fx==f){for(int i=0;i<3;i++)v[i]=__atomic_load_n(&s->tailSnapshot[i],__ATOMIC_RELAXED);}else{pr_lock();if(f){for(int i=0;i<PR_TAILS;i++)if(f->tails[i].bank&&!f->tails[i].done){v[0]++;v[1]+=tsf_active_voice_count(f->tails[i].bank);}v[2]=f->tailFades;}pr_unlock();}jintArray a=(*e)->NewIntArray(e,3);if(a)(*e)->SetIntArrayRegion(e,a,0,3,v);return a;}
JNIEXPORT void JNICALL J(channelConfig)(JNIEnv*e,jclass c,jlong h,jint ch,jfloat pan,jfloat chorus,jfloat delay,jfloat ms,jfloat feedback,jfloat reverb,jfloat resonance,jboolean pedal,jfloat low,jfloat mid,jfloat high,jint transpose){FX*f=(FX*)h;pr_lock();if(f&&ch>=0&&ch<5){PrChannelFx*p=(ch==4?f->yamaha:f->parts[ch]);p->pan=pan;p->chorus=chorus;p->delay=delay;p->delayMs=ms;p->feedback=feedback;p->reverb=reverb;p->resonance=resonance;int modeChanged=p->pedalMode!=pedal;p->pedalMode=pedal;if(modeChanged&&(ch==4?0:f->currentBanks[ch]))part_pedal((ch==4?0:f->currentBanks[ch]),p,p->pedal);p->transpose=transpose;if(p->eq.rate!=fx_rate(f)||p->eq.db[0]!=low||p->eq.db[1]!=mid||p->eq.db[2]!=high)pr_eq_request(&p->eq,low!=0||mid!=0||high!=0,low,mid,high,fx_rate(f));}pr_unlock();}

JNIEXPORT jboolean JNICALL J(retroEnable)(JNIEnv*e,jclass c,jboolean on){pr_lock();int r=retro_enable(on);pr_unlock();return r;}
JNIEXPORT jlongArray JNICALL J(retroSnapshot)(JNIEnv*e,jclass c){jlong v[4];v[3]=__atomic_load_n(&retroEpoch,__ATOMIC_ACQUIRE);v[0]=__atomic_load_n(&retroWrite,__ATOMIC_ACQUIRE);v[1]=__atomic_load_n(&retroRate,__ATOMIC_ACQUIRE);v[2]=pr_boottime();if(v[3]!=__atomic_load_n(&retroEpoch,__ATOMIC_ACQUIRE))v[0]=0;jlongArray a=(*e)->NewLongArray(e,4);if(a)(*e)->SetLongArrayRegion(e,a,0,4,v);return a;}
JNIEXPORT jint JNICALL J(retroRead)(JNIEnv*e,jclass c,jlong begin,jint epoch,jobject buffer,jint frames){short*p=(*e)->GetDirectBufferAddress(e,buffer);return p&&frames>0&&(*e)->GetDirectBufferCapacity(e,buffer)>=frames*4?retro_read(begin,epoch,p,frames):-1;}

JNIEXPORT void JNICALL J(nativeGains)(JNIEnv*e,jclass c,jlong h,jfloat a,jfloat b,jfloat cc,jfloat d){pr_lock();if(nativeStream.fx==(FX*)h){nativeStream.ga=a;nativeStream.gb=b;nativeStream.gc=cc;nativeStream.gd=d;}pr_unlock();}

/* Caller owns base handles; copies retain immutable sample data atomically. */
/* Original banks are retained by every active/prepared/retired clone. Atomic
   ownership is a conservative lifetime check; it needs no render lock and
   never frees a bank before tail collection has actually released its clone. */
JNIEXPORT jboolean JNICALL J(bankInUse)(JNIEnv*e,jclass c,jlong h,jlong base){tsf*b=(tsf*)base;return h&&b&&b->refCount&&__atomic_load_n(b->refCount,__ATOMIC_ACQUIRE)>1;}

JNIEXPORT void JNICALL J(tailMaster)(JNIEnv*e,jclass c,jlong h,jfloat value){FX*f=(FX*)h;pr_lock();if(f)f->tailScale=f->masterAnchor>0?value/f->masterAnchor:value;pr_unlock();}

#include "piano_profile.h"
JNIEXPORT jfloatArray JNICALL J(analyzePiano)(JNIEnv*e,jclass c,jlong bank,jint preset){float profile[792];pr_profile_estimate_extended((tsf*)bank,preset,profile);jfloatArray out=(*e)->NewFloatArray(e,792);if(out)(*e)->SetFloatArrayRegion(e,out,0,792,profile);return out;}
JNIEXPORT void JNICALL J(pianoControls)(JNIEnv*e,jclass c,jlong h,jint ch,jint blend,jint curve,jboolean contextual,jint hammer,jint key,jint pedal){FX*f=(FX*)h;pr_lock();if(f&&ch>=0&&ch<5){PrChannelFx*p=(ch==4?f->yamaha:f->parts[ch]);p->mechanics.hammer=hammer<0?0:hammer>100?100:hammer;p->mechanics.key=key<0?0:key>100?100:key;p->mechanics.pedal=pedal<0?0:pedal>100?100:pedal;tsf*b=(ch==4?0:f->currentBanks[ch]);if(b){b->prBlend=blend<0?0:blend>8?8:blend;b->prBlendCurve=curve==1?1:0;b->prContext=contextual;}}pr_unlock();}
JNIEXPORT void JNICALL J(pianoProfile)(JNIEnv*e,jclass c,jlong h,jint ch,jfloatArray values,jboolean harmonics){FX*f=(FX*)h;if(!f||ch<0||ch>=5)return;float profile[792]={0};int count=values?(*e)->GetArrayLength(e,values):0,enabled=count==264||count==792;if(enabled)(*e)->GetFloatArrayRegion(e,values,0,count,profile);
 /* Native stream rate may change while preparing coefficients; retry off callback. */
 for(int attempt=0;attempt<3;attempt++){pr_lock();int rate=fx_rate(f);pr_unlock();PrPianoResonance next;pr_res_init(&next,rate);if(enabled){pr_res_profile(&next,profile);if(harmonics)pr_res_harmonics(&next,profile,count);}pr_lock();PrChannelFx*p=(ch==4?f->yamaha:f->parts[ch]);if(rate!=fx_rate(f)){pr_unlock();continue;}memcpy(p->profile,profile,sizeof(profile));p->profileSet=enabled;p->profileCount=count;p->harmonics=harmonics;pr_res_coefficients(&p->strings,&next);pr_unlock();return;}
}

JNIEXPORT void JNICALL J(pianoBody)(JNIEnv*e,jclass c,jlong h,jint ch,jboolean enabled,jint body,jint board,jint size,jint damping,jint lid){FX*f=(FX*)h;if(!f||ch<0||ch>=5)return;
 /* All transcendental functions outside render. Reused control values skip preparation. */
 for(int attempt=0;attempt<3;attempt++){pr_lock();int rate=fx_rate(f);PrPianoBody*p=&(ch==4?f->yamaha:f->parts[ch])->body;int same=p->enabled==enabled&&p->body==body&&p->board==board&&p->size==size&&p->decay==damping&&p->lid==lid;pr_unlock();if(same)return;PrPianoBody next;pr_body_init(&next,rate);pr_body_prepare(&next,enabled,body,board,size,damping,lid,rate);pr_lock();if(rate!=fx_rate(f)){pr_unlock();continue;}pr_body_apply(&(ch==4?f->yamaha:f->parts[ch])->body,&next);pr_unlock();return;}
}

JNIEXPORT void JNICALL J(soundSpace)(JNIEnv*e,jclass c,jlong h,jint ch,jboolean enabled,jint room,jint amount,jint texture,jint duck,jint decay,jint repeat,jboolean releaseSmooth){
 FX*f=(FX*)h;if(!f||ch<0||ch>=5)return;pr_lock();PrHybridSpace*s=&(ch==4?f->yamaha:f->parts[ch])->space;
 if(s->enabled!=!!enabled||s->room!=room||s->amount!=amount||s->texture!=texture||s->duck!=duck||s->decay!=decay)pr_space_config(s,enabled,room,amount,texture,duck,decay);
 tsf*b=(ch==4?0:f->currentBanks[ch]);if(b){b->prRepeat=repeat<0?0:repeat>100?100:repeat;b->prReleaseSmooth=releaseSmooth;}pr_unlock();
}

JNIEXPORT jint JNICALL J(benchmarkCallbackMax)(JNIEnv*e,jclass c){return __atomic_exchange_n(&nativeStream.benchMax,0,__ATOMIC_RELAXED);}

JNIEXPORT jint JNICALL J(yamahaInputOpen)(JNIEnv*e,jclass c,jint id){return aa_input_start(id);}
JNIEXPORT void JNICALL J(yamahaInputStop)(JNIEnv*e,jclass c){aa_input_stop();}
JNIEXPORT void JNICALL J(yamahaInputMix)(JNIEnv*e,jclass c,jlong h,jboolean sendOnly,jfloat gain){pr_lock();FX*f=(FX*)h;if(f&&f->yamaha){f->yamaha->sendOnly=sendOnly;f->yamahaGain=fmaxf(0,fminf(1,gain));}pr_unlock();}
JNIEXPORT jintArray JNICALL J(yamahaInputStats)(JNIEnv*e,jclass c){PrStream*s=&nativeStream;jint v[6];v[0]=s->input?s->inputId:0;v[1]=__atomic_load_n(&s->inputError,__ATOMIC_ACQUIRE);v[2]=__atomic_load_n(&s->inputShort,__ATOMIC_RELAXED);v[3]=__atomic_load_n(&s->inputReads,__ATOMIC_RELAXED);v[4]=s->rate;v[5]=__atomic_load_n(&s->inputPeakSnapshot,__ATOMIC_RELAXED);jintArray a=(*e)->NewIntArray(e,6);if(a)(*e)->SetIntArrayRegion(e,a,0,6,v);return a;}

JNIEXPORT jintArray JNICALL J(benchmarkPhaseMax)(JNIEnv*e,jclass c){jint v[4];for(int i=0;i<4;i++)v[i]=__atomic_exchange_n(&nativeStream.phaseBench[i],0,__ATOMIC_RELAXED);jintArray a=(*e)->NewIntArray(e,4);if(a)(*e)->SetIntArrayRegion(e,a,0,4,v);return a;}

JNIEXPORT jboolean JNICALL J(mechanicalLoad)(JNIEnv*e,jclass c,jstring path){if(!path)return JNI_FALSE;const char*p=(*e)->GetStringUTFChars(e,path,0);if(!p)return JNI_FALSE;int result=pr_mech_load(p);(*e)->ReleaseStringUTFChars(e,path,p);return result?JNI_TRUE:JNI_FALSE;}

JNIEXPORT void JNICALL J(bankRelease)(JNIEnv*e,jclass c,jlong h,jint pedalMs,jint keyMs){if(pedalMs<0||pedalMs>8000||keyMs<0||keyMs>8000)return;pr_lock();tsf*b=(tsf*)h;if(b){b->prPedalFade=pedalMs?fmaxf(20,pedalMs)*.001f:0;b->prKeyFade=keyMs?fmaxf(20,keyMs)*.001f:0;}pr_unlock();}

JNIEXPORT jlongArray JNICALL J(performancePoll)(JNIEnv*e,jclass c){long long values[PR_PERF_VALUES];pr_performance_poll(values,nativeStream.rate);jlongArray a=(*e)->NewLongArray(e,PR_PERF_VALUES);if(a)(*e)->SetLongArrayRegion(e,a,0,PR_PERF_VALUES,(const jlong*)values);return a;}

JNIEXPORT jlongArray JNICALL J(bankMemory)(JNIEnv*e,jclass c,jlong a,jlong b,jlong cc,jlong d){tsf*banks[4]={(tsf*)a,(tsf*)b,(tsf*)cc,(tsf*)d};jlong v[3]={0};for(int i=0;i<4;i++){tsf*f=banks[i];if(!f)continue;int seen=0;for(int j=0;j<i;j++)if(banks[j]&&banks[j]->fontSamples==f->fontSamples)seen=1;if(seen)continue;v[0]+=(jlong)f->sampleCount*4;v[1]+=(jlong)f->prPreparedBytes;if(f->prMapping)v[2]+=(jlong)f->sampleCount*4;}jlongArray out=(*e)->NewLongArray(e,3);if(out)(*e)->SetLongArrayRegion(e,out,0,3,v);return out;}

JNIEXPORT void JNICALL J(nativeConfigure)(JNIEnv*e,jclass c,jint mode){aa_stop();requestedBackend=mode>=2?mode:1;}
JNIEXPORT jlongArray JNICALL J(parallelStats)(JNIEnv*e,jclass c){long long v[10]={0};pr_parallel_stats(v);jlongArray result=(*e)->NewLongArray(e,10);(*e)->SetLongArrayRegion(e,result,0,10,(jlong*)v);return result;}
