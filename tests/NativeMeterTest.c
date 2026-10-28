#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "../native/bridge.c"
typedef struct {float floats[8];int ints[8];} TestArray;
static jfloatArray new_float(JNIEnv*e,jsize n){assert(n<=8);return (jfloatArray)calloc(1,sizeof(TestArray));}
static jintArray new_int(JNIEnv*e,jsize n){assert(n<=8);return (jintArray)calloc(1,sizeof(TestArray));}
static void set_float(JNIEnv*e,jfloatArray a,jsize from,jsize n,const jfloat*v){memcpy(((TestArray*)a)->floats+from,v,n*sizeof(float));}
static void set_int(JNIEnv*e,jintArray a,jsize from,jsize n,const jint*v){memcpy(((TestArray*)a)->ints+from,v,n*sizeof(int));}
int main(void){struct JNINativeInterface_ table={.NewFloatArray=new_float,.NewIntArray=new_int,.SetFloatArrayRegion=set_float,.SetIntArrayRegion=set_int};JNIEnv env=&table;FX*f=calloc(1,sizeof(*f));assert(f);f->rate=48000;f->peakBefore=.7f;f->peakAfter=.5f;f->limiterGain=.9f;f->limitedSamples=3;f->totalSamples=100;f->layerPeak[2]=.55f;
 PrStream*s=&nativeStream;s->stream=(AAudioStream*)1;s->fx=f;s->rate=48000;aa_publish(s,0);assert(f->totalSamples==0);engineBusy=1;alarm(2); // any attempt to acquire the render lock fails this test
 TestArray*a=(TestArray*)J(levels)(&env,0,(jlong)f);assert(a&&fabsf(a->floats[0]-.7f)<1e-6&&fabsf(a->floats[2]-3)<1e-6);free(a);
 a=(TestArray*)J(mixerLevels)(&env,0,(jlong)f);assert(a&&fabsf(a->floats[2]-.55f)<1e-6);free(a);
 a=(TestArray*)J(voiceStats)(&env,0,0,0,0,0);assert(a&&a->ints[0]==0);free(a);
 a=(TestArray*)J(tailStats)(&env,0,(jlong)f);assert(a&&a->ints[0]==0);free(a);
 a=(TestArray*)J(yamahaInputStats)(&env,0);assert(a&&a->ints[0]==0);free(a);
 a=(TestArray*)J(midiQueueStats)(&env,0,(jlong)f);assert(a&&a->ints[1]==0);free(a);
 tsf font={0};int references=2;font.refCount=&references;assert(J(bankInUse)(&env,0,(jlong)f,(jlong)&font));references=1;assert(!J(bankInUse)(&env,0,(jlong)f,(jlong)&font));
 assert(J(tailDetach)(&env,0,(jlong)f)==0);assert(engineBusy==1&&s->drops==0);s->phaseBench[1]=200000;a=(TestArray*)J(benchmarkPhaseMax)(&env,0);assert(a&&a->ints[1]==200000);free(a);a=(TestArray*)J(benchmarkPhaseMax)(&env,0);assert(a&&a->ints[1]==0);free(a);alarm(0);engineBusy=0;PrChannelFx*part=part_new(48000);assert(part);f->tails[0].bank=&font;f->tails[0].part=part;s->meterCountdown=0;aa_publish(s,0);assert(s->tailSnapshot[0]==1);J(panic)(&env,0,0,0,(jlong)f);assert(s->tailSnapshot[0]==0&&s->doneSnapshot==1);f->tails[0].bank=0;free(part);s->stream=0;s->fx=0;free(f);
 puts("PASS: actual JNI levels/channel meters/voices/tails/USB health/MIDI stats and empty collection are independent of a held render lock; no audio conflicts introduced by these reads.");return 0;
}
