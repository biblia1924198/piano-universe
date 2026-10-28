#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../native/engine.h"
#include "../native/aaudio_backend.h"
static FX* shared;static int finished;
static void* producer(void* arg){for(int i=0;i<100000;i++){PrMidiEvent ev={0xE0,i&127,(i>>7)&127,15};while(__atomic_load_n(&shared->eventWrite,__ATOMIC_ACQUIRE)-__atomic_load_n(&shared->eventRead,__ATOMIC_ACQUIRE)>=PR_MIDI_EVENTS){}enqueue_midi(shared,ev);}__atomic_store_n(&finished,1,__ATOMIC_RELEASE);return 0;}
int main(int argc,char**argv){
 assert(argc==2);tsf*bank=load_font(argv[1]);assert(bank);tsf*a=copy_font(bank,128);assert(a);tsf_channel_set_presetindex(a,0,0);tsf*banks[4]={a,0,0,0};
 FX*f=calloc(1,sizeof(FX));config_fx(f,0,1,.8f,0,120,4,.6f);f->protect=1;nativeStream.fx=f;nativeStream.a=a;nativeStream.ga=1;nativeStream.rate=f->rate=48000;
 // Enqueue does not take the engine lock; no MIDI producer enters the callback's lock.
 pr_lock();enqueue_midi(f,(PrMidiEvent){0x90,60,127,1});assert(f->eventWrite==1&&f->eventRead==0);pr_unlock();
 float out[192*2];aa_data(0,&nativeStream,out,192);assert(f->eventRead==1&&tsf_active_voice_count(a)>0);
 f->toneLeft=96000;aa_data(0,&nativeStream,out,192);int precise=0;for(int i=0;i<384;i++){float grid=out[i]*32768;precise+=fabsf(grid-roundf(grid))>.001f;}assert(precise>300);
 // Rare configuration contention fades rather than suddenly zeroing a whole block.
 nativeStream.last[0]=.4f;nativeStream.last[1]=-.2f;rec_start(48000);pr_lock();aa_data(0,&nativeStream,out,192);pr_unlock();assert(out[0]>.38f&&out[1]<-.19f);for(int i=1;i<64;i++){assert(out[i*2]>=0&&out[i*2]<=out[(i-1)*2]);assert(fabsf(out[i*2]-out[(i-1)*2])<=.006251f);}assert(out[128]==0);short recorded[384];assert(rec_pull(recorded,192)==192&&recorded[0]>12000);rec_stop();
 aa_data(0,&nativeStream,out,192);assert(fabsf(out[0])<.01f&&isfinite(out[383]));
 // Saturated bursts explicitly panic rather than losing note-offs silently.
 clear_midi(f);for(unsigned i=0;i<PR_MIDI_EVENTS+1;i++)enqueue_midi(f,(PrMidiEvent){0x90,60,127,1});assert(f->eventOverflow==1);drain_midi(f,banks);assert(tsf_active_voice_count(a)==0&&f->eventRead==f->eventWrite&&a->channels);enqueue_midi(f,(PrMidiEvent){0x90,62,90,1});drain_midi(f,banks);assert(tsf_active_voice_count(a)>0);
 // Real producer/consumer concurrency and ring wrap; monotonic event order.
 clear_midi(f);shared=f;pthread_t thread;assert(!pthread_create(&thread,0,producer,0));int expected=0;
 while(!__atomic_load_n(&finished,__ATOMIC_ACQUIRE)||f->eventRead!=__atomic_load_n(&f->eventWrite,__ATOMIC_ACQUIRE)){unsigned r=f->eventRead,w=__atomic_load_n(&f->eventWrite,__ATOMIC_ACQUIRE);if(r==w)continue;PrMidiEvent ev=f->events[r&(PR_MIDI_EVENTS-1)];assert(ev.a==(expected&127)&&ev.b==((expected>>7)&127));expected++;__atomic_store_n(&f->eventRead,r+1,__ATOMIC_RELEASE);}pthread_join(thread,0);assert(expected==100000&&f->eventOverflow==1);
 // A later loud peak no longer changes gain abruptly at the block boundary.
 panic_fx(a,0,f);float mix[BLOCK*2]={0},limited[BLOCK*2];for(int i=0;i<BLOCK;i++)mix[i*2]=mix[i*2+1]=.2f;output_audio_float(f,mix,limited,BLOCK);float previous=limited[BLOCK*2-1];mix[128*2]=mix[128*2+1]=8;output_audio_float(f,mix,limited,BLOCK);assert(fabsf(limited[0]-previous)<.01f);for(int i=0;i<BLOCK*2;i++)assert(fabsf(limited[i])<=.94f);
 free(f);tsf_close(a);tsf_close(bank);puts("PASS: MIDI enqueue without engine lock; 100000 concurrent events in order; overflow panic; float32 precision; contention fade/recovery + recording continuity; smoothed stereo limiter.");return 0;
}
