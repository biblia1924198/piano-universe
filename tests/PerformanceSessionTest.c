static int hintids(int* ids){ids[0]=456;ids[1]=789;return 2;}
#define PR_HINT_THREAD_IDS(ids) hintids(ids)
#include <stdio.h>
#include <assert.h>
#include "../native/engine.h"
#include "../native/aaudio_backend.h"
static int creates,closes,reports,fail;static long long reported,target;
static void* mockcreate(void*m,const int*t,unsigned long n,long long goal){assert(m&&(n==3)&&t[0]==123&&t[1]==456&&t[2]==789);creates++;target=goal;return (void*)2;}
static void mockclose(void*s){assert(s);closes++;}
static int mocktarget(void*s,long long n){assert(s);target=n;return 0;}
static int mockreport(void*s,long long n){assert(s);reports++;reported=n;return fail;}
int main(void){long long v[PR_PERF_VALUES];pr_performance_start();prPerformance.manager=(void*)1;prPerformance.create=mockcreate;prPerformance.close=mockclose;prPerformance.target=mocktarget;prPerformance.report=mockreport;prPerformance.status=1;prPerformance.tid=123;
 pr_performance_end(500000,96,48000,0);pr_performance_poll(v,48000);assert(v[1]==1&&v[4]==96&&v[5]==96&&v[6]==250&&v[17]==1);assert(creates==1&&reports==1&&reported==500000&&target==1400000&&v[12]==2);
 pr_performance_end(1000000,192,48000,0);pr_performance_poll(v,48000);assert(creates==1&&reports==2&&reported==1000000&&target==2800000&&v[5]==192);
 pr_performance_end(100000,96,48000,0);pr_performance_end(900000,96,48000,0);pr_performance_poll(v,48000);assert(reported==900000&&target==1400000);
 fail=1;pr_performance_end(8000000,96,48000,0);pr_performance_poll(v,48000);assert(v[12]==-2&&v[15]==1&&v[32]==1&&closes==1);
 pr_performance_close();assert(closes==1);puts("PASS: native deadline histogram, actual duration, 70% target, variable frames, ADPF creation/report/error/close lifecycle.");return 0;}
