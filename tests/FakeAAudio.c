/* Host-only test double for the public AAudio ABI, never included in the APK. */
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
typedef struct Stream Stream;typedef int(*Callback)(Stream*,void*,void*,int);typedef void(*Error)(Stream*,void*,int);
typedef struct{int id,sharing,perf,ch,format,direction,rate;Callback cb;Error err;void*user;}Builder;
struct Stream{Builder b;pthread_t thread;int running,size;long long frames;};
int AAudio_createStreamBuilder(Builder**p){*p=calloc(1,sizeof(Builder));return 0;}
int AAudioStreamBuilder_delete(Builder*b){free(b);return 0;}
#define SET(name,field) void AAudioStreamBuilder_set##name(Builder*b,int v){b->field=v;}
SET(DeviceId,id) SET(SharingMode,sharing) SET(PerformanceMode,perf) SET(ChannelCount,ch) SET(Format,format) SET(SampleRate,rate) 
void AAudioStreamBuilder_setDirection(Builder*b,int v){b->direction=v;}void AAudioStreamBuilder_setInputPreset(Builder*b,int v){}void AAudioStreamBuilder_setUsage(Builder*b,int v){}
void AAudioStreamBuilder_setDataCallback(Builder*b,Callback cb,void*user){b->cb=cb;b->user=user;}
void AAudioStreamBuilder_setErrorCallback(Builder*b,Error err,void*user){b->err=err;}
int AAudioStreamBuilder_openStream(Builder*b,Stream**p){if(b->direction&&getenv("PR_FAKE_INPUT_REJECT"))return -899;const char*delay=getenv("PR_FAKE_OPEN_DELAY_MS");if(delay)usleep(atoi(delay)*1000);if(getenv("PR_FAKE_REJECT_ALL"))return -899;if(getenv("PR_FAKE_SHARED")&&b->sharing==0)return -899;*p=calloc(1,sizeof(Stream));(*p)->b=*b;(*p)->size=1536;return 0;}
static void*loop(void*user){Stream*s=user;float out[192*2];float peak=0;int errorSent=0;while(__atomic_load_n(&s->running,__ATOMIC_ACQUIRE)){memset(out,0,sizeof(out));if(s->b.cb)s->b.cb(s,s->b.user,out,192);for(int i=0;i<384;i++){if(!isfinite(out[i])||fabsf(out[i])>.981f){fprintf(stderr,"invalid callback PCM\n");abort();}if(fabsf(out[i])>peak)peak=fabsf(out[i]);}s->frames+=192;if(getenv("PR_FAKE_DISCONNECT")&&s->frames>24000&&!errorSent){errorSent=1;s->b.err(s,s->b.user,-899);}usleep(4000);}if(peak==0&&!getenv("PR_FAKE_DISCONNECT")&&!getenv("PR_FAKE_ALLOW_SILENT")){fprintf(stderr,"callback was never audible\n");abort();}return 0;}
int AAudioStream_requestStart(Stream*s){if(s->b.direction)return 0;__atomic_store_n(&s->running,1,__ATOMIC_RELEASE);return pthread_create(&s->thread,0,loop,s);}
int AAudioStream_requestStop(Stream*s){if(__atomic_exchange_n(&s->running,0,__ATOMIC_ACQ_REL))pthread_join(s->thread,0);return 0;}
int AAudioStream_close(Stream*s){AAudioStream_requestStop(s);free(s);return 0;}
int AAudioStream_getSampleRate(Stream*s){return 48000;}int AAudioStream_getChannelCount(Stream*s){return s->b.ch;}int AAudioStream_getFormat(Stream*s){return s->b.format;}
int AAudioStream_getBufferSizeInFrames(Stream*s){return s->size;}int AAudioStream_getBufferCapacityInFrames(Stream*s){return 5760;}int AAudioStream_getFramesPerBurst(Stream*s){return 192;}
int AAudioStream_getSharingMode(Stream*s){return s->b.sharing;}int AAudioStream_getPerformanceMode(Stream*s){return s->b.perf;}int AAudioStream_getXRunCount(Stream*s){return 0;}int AAudioStream_getDeviceId(Stream*s){return s->b.direction&&getenv("PR_FAKE_INPUT_WRONG_ID")?999:s->b.id;}
int AAudioStream_setBufferSizeInFrames(Stream*s,int frames){s->size=frames>5760?5760:frames;return s->size;}

int AAudioStream_read(Stream*s,void*out,int frames,long long timeout){if(!s->b.direction||timeout!=0)return -1;if(getenv("PR_FAKE_INPUT_READ_ERROR"))return -899;float*p=out;int n=(getenv("PR_FAKE_INPUT_SHORT")&&s->frames>384&&s->frames<1000)?frames/2:frames;for(int i=0;i<n;i++){float x=.2f*sinf((s->frames+i)*.05759586f);p[i*2]=x;p[i*2+1]=x*.8f;}s->frames+=n;return n;}
