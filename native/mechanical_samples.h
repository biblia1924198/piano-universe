/* Real recorded piano action. Asset preparation/loading is outside render.
 * One immutable process-lifetime bank is shared by channels and scene tails. */
#ifndef PR_MECHANICAL_SAMPLES
#define PR_MECHANICAL_SAMPLES
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#define PR_MECH_RECORDINGS 92
#define PR_MECH_VOICES 32
typedef struct {const float*data;uint32_t frames;} PrMechanicalSample;
typedef struct {float*data;PrMechanicalSample sample[PR_MECH_RECORDINGS];int rate;} PrMechanicalBank;
typedef struct {const PrMechanicalSample*sample,*old;float position,oldPosition,step,oldStep,gain,oldGain;int fade;} PrRecordedVoice;
static PrMechanicalBank*pr_mechanical_bank;
static PrMechanicalBank*pr_recorded_bank(void){return __atomic_load_n(&pr_mechanical_bank,__ATOMIC_ACQUIRE);}
static int pr_mech_read(int fd,void*data,size_t bytes){char*p=data;while(bytes){ssize_t n=read(fd,p,bytes);if(n<0&&errno==EINTR)continue;if(n<=0)return 0;p+=n;bytes-=n;}return 1;}
static uint32_t pr_mech_u32(const unsigned char*b){return (uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}
static int pr_mech_load(const char*path){
 int fd=open(path,O_RDONLY);if(fd<0)return 0;unsigned char h[20],table[PR_MECH_RECORDINGS*8];PrMechanicalBank*b=0;int ok=0;
 if(!pr_mech_read(fd,h,20)||memcmp(h,"PRM1",4)||pr_mech_u32(h+4)!=1||pr_mech_u32(h+8)!=48000||pr_mech_u32(h+12)!=PR_MECH_RECORDINGS)goto finish;
 uint32_t frames=pr_mech_u32(h+16);if(frames<PR_MECH_RECORDINGS*4||frames>48000*48||!pr_mech_read(fd,table,sizeof(table)))goto finish;
 b=calloc(1,sizeof(*b));if(!b)goto finish;b->data=malloc((size_t)frames*8);if(!b->data)goto finish;b->rate=48000;
 uint32_t previous=0;for(int i=0;i<PR_MECH_RECORDINGS;i++){uint32_t at=pr_mech_u32(table+i*8),n=pr_mech_u32(table+i*8+4);if(at!=previous||n<4||n>96000||at>frames||n>frames-at)goto finish;b->sample[i].data=b->data+at*2;b->sample[i].frames=n;previous=at+n;}if(previous!=frames)goto finish;
 if(!pr_mech_read(fd,b->data,(size_t)frames*8))goto finish;unsigned char extra;if(read(fd,&extra,1)!=0)goto finish;
 for(size_t i=0;i<(size_t)frames*2;i++)if(!isfinite(b->data[i])||fabsf(b->data[i])>2)goto finish;
 PrMechanicalBank*expected=0;if(__atomic_compare_exchange_n(&pr_mechanical_bank,&expected,b,0,__ATOMIC_RELEASE,__ATOMIC_ACQUIRE)){b=0;}ok=1;
 finish:if(b){free(b->data);free(b);}close(fd);return ok;
}
static int pr_mech_sample(const PrMechanicalSample*s,float position,float gain,float*l,float*r){
 if(!s||position<0||position>=s->frames-1)return 0;uint32_t i=(uint32_t)position;float a=position-i;const float*p=s->data+i*2;*l=(p[0]+(p[2]-p[0])*a)*gain;*r=(p[1]+(p[3]-p[1])*a)*gain;return 1;
}
static void pr_mech_record(PrRecordedVoice*v,const PrMechanicalSample*s,int rate,float gain){
 if(v->sample){v->old=v->sample;v->oldPosition=v->position;v->oldStep=v->step;v->oldGain=v->gain;v->fade=64;}
 v->sample=s;v->position=0;v->step=48000.f/rate;v->gain=gain;
}
static int pr_mech_record_tick(PrRecordedVoice*v,float*l,float*r){
 float a=0,b=0;if(v->sample){if(pr_mech_sample(v->sample,v->position,v->gain,&a,&b)){v->position+=v->step;*l+=a;*r+=b;}else v->sample=0;}
 if(v->old&&v->fade>0){a=b=0;if(pr_mech_sample(v->old,v->oldPosition,v->oldGain*v->fade/64.f,&a,&b)){v->oldPosition+=v->oldStep;*l+=a;*r+=b;}if(--v->fade==0)v->old=0;}
 return v->sample!=0||v->old!=0;
}
#endif
