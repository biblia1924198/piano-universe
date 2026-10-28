/* Single-producer/single-consumer PCM tap. Disk and codecs never run here. */
#ifndef PR_RECORD_TAP
#define PR_RECORD_TAP
#define PR_REC_FRAMES (1u<<19)
#ifndef _STRUCT_TIMESPEC
#define _STRUCT_TIMESPEC 1
struct timespec {long tv_sec,tv_nsec;};
#endif
extern int clock_gettime(int,struct timespec*);
static long long pr_boottime(void){struct timespec t;if(clock_gettime(7,&t))clock_gettime(1,&t);return (long long)t.tv_sec*1000000000LL+t.tv_nsec;}
static short recPCM[PR_REC_FRAMES*2];
static unsigned recWrite,recRead;static int recActive,recRate,recFailure,recWriter;static long long recFirst;
static void rec_stop(void){__atomic_store_n(&recActive,0,__ATOMIC_RELEASE);while(__atomic_load_n(&recWriter,__ATOMIC_ACQUIRE)){} }
static void rec_start(int rate){rec_stop();recWrite=recRead=0;recRate=rate;recFailure=0;recFirst=0;__atomic_store_n(&recActive,1,__ATOMIC_RELEASE);}
static void rec_push(const short*pcm,int frames,int rate){
 if(!__atomic_load_n(&recActive,__ATOMIC_ACQUIRE))return;
 __atomic_add_fetch(&recWriter,1,__ATOMIC_ACQ_REL);
 if(!__atomic_load_n(&recActive,__ATOMIC_ACQUIRE))goto finished;
 if(rate!=recRate){__atomic_store_n(&recFailure,2,__ATOMIC_RELEASE);__atomic_store_n(&recActive,0,__ATOMIC_RELEASE);goto finished;}
 unsigned w=__atomic_load_n(&recWrite,__ATOMIC_RELAXED),r=__atomic_load_n(&recRead,__ATOMIC_ACQUIRE);
 if((unsigned)(w-r)+(unsigned)frames>PR_REC_FRAMES){__atomic_store_n(&recFailure,1,__ATOMIC_RELEASE);__atomic_store_n(&recActive,0,__ATOMIC_RELEASE);goto finished;}
 if(!recFirst)__atomic_store_n(&recFirst,pr_boottime(),__ATOMIC_RELEASE);
 for(int i=0;i<frames;i++){unsigned p=(w+i)&(PR_REC_FRAMES-1);recPCM[p*2]=pcm[i*2];recPCM[p*2+1]=pcm[i*2+1];}
 __atomic_store_n(&recWrite,w+frames,__ATOMIC_RELEASE);
finished:__atomic_sub_fetch(&recWriter,1,__ATOMIC_RELEASE);
}
static int rec_pull(short*out,int frames){unsigned r=__atomic_load_n(&recRead,__ATOMIC_RELAXED),w=__atomic_load_n(&recWrite,__ATOMIC_ACQUIRE);if((unsigned)frames>w-r)frames=w-r;for(int i=0;i<frames;i++){unsigned p=(r+i)&(PR_REC_FRAMES-1);out[i*2]=recPCM[p*2];out[i*2+1]=recPCM[p*2+1];}__atomic_store_n(&recRead,r+frames,__ATOMIC_RELEASE);return frames;}
#endif
