#ifndef PR_HINT_THREAD_IDS
#define PR_HINT_THREAD_IDS(ids) pr_parallel_thread_ids(ids)
#endif
/* Optional ADPF control runs on the monitor thread, never in the audio callback.
   Snapshot fields are atomic; seq avoids mixing callback generations. */
#ifndef PR_PERFORMANCE_SESSION
#define PR_PERFORMANCE_SESSION
#ifdef __ANDROID__
#include <sys/resource.h>
#include <time.h>
#endif
#define PR_PERF_BINS 17
#define PR_PERF_VALUES (16+PR_PERF_BINS)
typedef struct {
 unsigned seq; long long workNs,workFrames,tid,count,elapsed,frames,minFrames,maxFrames,maxPermille,cpu,cpuPeriod,cpuCount,hist[PR_PERF_BINS];
 void *library,*manager,*session;
 void*(*getManager)(void);void*(*create)(void*,const int*,unsigned long,long long);
 int(*target)(void*,long long);int(*report)(void*,long long);void(*close)(void*);
 int status,enabled,errors;long long reports,lastCount,lastTime,lastFrames,targetNs;
} PrPerformance;
static PrPerformance prPerformance;
#define PR_GET(p) __atomic_load_n(&(p),__ATOMIC_ACQUIRE)
#define PR_SET(p,v) __atomic_store_n(&(p),(v),__ATOMIC_RELEASE)
static void pr_performance_close(void){PrPerformance*p=&prPerformance;if(p->session){p->close(p->session);p->session=0;}p->status=0;}
static void pr_performance_start(void){
 PrPerformance*p=&prPerformance;pr_performance_close();
 p->seq=0;p->workNs=p->workFrames=0;p->tid=p->count=p->elapsed=p->frames=p->minFrames=p->maxFrames=p->maxPermille=p->cpu=p->cpuPeriod=p->cpuCount=0;
 memset(p->hist,0,sizeof(p->hist));p->reports=p->lastCount=p->lastTime=p->lastFrames=p->targetNs=0;p->errors=0;p->enabled=1;p->status=1;
#ifdef __ANDROID__
 if(!p->library){p->library=dlopen("libandroid.so",2);if(p->library){
  p->getManager=(void*)dlsym(p->library,"APerformanceHint_getManager");p->create=(void*)dlsym(p->library,"APerformanceHint_createSession");
  p->target=(void*)dlsym(p->library,"APerformanceHint_updateTargetWorkDuration");p->report=(void*)dlsym(p->library,"APerformanceHint_reportActualWorkDuration");p->close=(void*)dlsym(p->library,"APerformanceHint_closeSession");
 }}
 if(!p->getManager||!p->create||!p->target||!p->report||!p->close){p->status=-1;return;}
 p->manager=p->getManager();if(!p->manager)p->status=-1;
#else
 p->status=-1;
#endif
}
static long long pr_cpu_clock(void){
#ifdef __ANDROID__
 struct timespec t;if(!clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t))return (long long)t.tv_sec*1000000000LL+t.tv_nsec;
#endif
 return 0;
}
static long long pr_performance_begin(void){
#ifdef __ANDROID__
 if(!PR_GET(prPerformance.tid))PR_SET(prPerformance.tid,gettid());
#endif
 return (PR_GET(prPerformance.count)&31)==0?pr_cpu_clock():0;
}
static void pr_performance_end(long long elapsed,int frames,int rate,long long cpuBegin){
 PrPerformance*p=&prPerformance;if(frames<=0||rate<=0)return;
 long long period=(long long)frames*1000000000LL/rate;
 long long ratio=elapsed*1000/(period?period:1);int bin=ratio/250;if(bin>=PR_PERF_BINS)bin=PR_PERF_BINS-1;
 long long cpu=cpuBegin?pr_cpu_clock()-cpuBegin:0;
 __atomic_add_fetch(&p->seq,1,__ATOMIC_ACQ_REL);
 PR_SET(p->workNs,elapsed);PR_SET(p->workFrames,frames);
 PR_SET(p->elapsed,PR_GET(p->elapsed)+elapsed);PR_SET(p->frames,PR_GET(p->frames)+frames);
 if(!PR_GET(p->minFrames)||frames<PR_GET(p->minFrames))PR_SET(p->minFrames,frames);
 if(frames>PR_GET(p->maxFrames))PR_SET(p->maxFrames,frames);
 if(ratio>PR_GET(p->maxPermille))PR_SET(p->maxPermille,ratio);
 PR_SET(p->hist[bin],PR_GET(p->hist[bin])+1);
 if(cpuBegin&&cpu>=0){PR_SET(p->cpu,PR_GET(p->cpu)+cpu);PR_SET(p->cpuPeriod,PR_GET(p->cpuPeriod)+period);PR_SET(p->cpuCount,PR_GET(p->cpuCount)+1);}
 PR_SET(p->count,PR_GET(p->count)+1);__atomic_add_fetch(&p->seq,1,__ATOMIC_RELEASE);
}
/* Control thread only; report the last fully measured callback work every monitor tick.
   The target is 70% of that callback period, requesting headroom without
   inventing actual work durations. No claim that the OS grants the request. */
static void pr_performance_poll(long long *v,int rate){
 PrPerformance*p=&prPerformance;memset(v,0,PR_PERF_VALUES*sizeof(*v));
 long long work=0,workFrames=0;int valid=0;for(int attempt=0;attempt<4;attempt++){
  unsigned seq=PR_GET(p->seq);if(seq&1)continue;
  work=PR_GET(p->workNs);workFrames=PR_GET(p->workFrames);
  v[0]=PR_GET(p->tid);v[1]=PR_GET(p->count);v[2]=PR_GET(p->elapsed);v[3]=PR_GET(p->frames);
  v[4]=PR_GET(p->minFrames);v[5]=PR_GET(p->maxFrames);v[6]=PR_GET(p->maxPermille);
  v[7]=PR_GET(p->cpu);v[8]=PR_GET(p->cpuPeriod);v[9]=PR_GET(p->cpuCount);
  for(int i=0;i<PR_PERF_BINS;i++)v[16+i]=PR_GET(p->hist[i]);
  if(seq==PR_GET(p->seq)){valid=1;break;}
  if(attempt==3){memset(v,0,PR_PERF_VALUES*sizeof(*v));return;}
 }
if(!valid){memset(v,0,PR_PERF_VALUES*sizeof(*v));return;}
#ifdef __ANDROID__
 struct rusage usage;if(!getrusage(RUSAGE_SELF,&usage)){v[10]=usage.ru_minflt;v[11]=usage.ru_majflt;}
#endif
 long long count=v[1]-p->lastCount,frames=v[3]-p->lastFrames,time=v[2]-p->lastTime;
 if(count>0&&frames>0&&time>0&&rate>0&&work>0&&workFrames>0){
  long long target=workFrames*1000000000LL/rate*7/10;if(target<1)target=1;
  if(p->enabled&&p->status==1&&v[0]>0){int tids[3]={(int)v[0],0,0};int additional=PR_HINT_THREAD_IDS(tids+1);if(additional>=0){p->session=p->create(p->manager,tids,1+additional,target);p->status=p->session?2:-2;p->targetNs=target;}}
  if(p->session){int result=0;if(target!=p->targetNs){result=p->target(p->session,target);if(!result)p->targetNs=target;}
   if(!result)result=p->report(p->session,work);if(result){p->errors++;p->close(p->session);p->session=0;p->status=-2;}else p->reports++;
  }
  p->lastCount=v[1];p->lastFrames=v[3];p->lastTime=v[2];
 }
 v[12]=p->status;v[13]=p->reports;v[14]=p->targetNs;v[15]=p->errors;
}
#endif
