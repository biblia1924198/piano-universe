#include "parallel_sampler.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdint>
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif
#ifdef _WIN32
#include <windows.h>
#else
#include <semaphore.h>
#include <errno.h>
#include <sys/resource.h>
#include <unistd.h>
#endif
// Only independent channel instances are dispatched. No voice redistribution,
// sample copy, allocation, mutex or quality change occurs during rendering.
namespace {
using Clock=std::chrono::steady_clock;
long long now(){return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();}
struct Slot {
 std::atomic<int> state{0}; // 1: queued, 2: owned, 3: complete
 std::atomic<bool> wake{false},quit{false};
 std::thread thread;PrRenderJob fn=nullptr;void* bank=nullptr;float* output=nullptr;int frames=0;
 std::atomic<long long> jobs{0},timeNs{0},priority{0};std::atomic<int> tid{0};
#ifdef _WIN32
 HANDLE sem=nullptr;
 bool init(){sem=CreateSemaphoreW(nullptr,0,1,nullptr);return sem!=nullptr;}void post(){ReleaseSemaphore(sem,1,nullptr);}void wait(){WaitForSingleObject(sem,INFINITE);}void close(){CloseHandle(sem);}
#else
 sem_t sem;
 bool init(){return sem_init(&sem,0,0)==0;}void post(){sem_post(&sem);}void wait(){while(sem_wait(&sem)!=0&&errno==EINTR){}}void close(){sem_destroy(&sem);}
#endif
 void signal(){if(!wake.exchange(true,std::memory_order_acq_rel))post();}
 bool execute(bool worker){int expected=1;if(!state.compare_exchange_strong(expected,2,std::memory_order_acq_rel))return false;
  long long begin=worker?now():0;fn(bank,output,frames);if(worker){jobs.fetch_add(1,std::memory_order_relaxed);timeNs.fetch_add(now()-begin,std::memory_order_relaxed);}state.store(3,std::memory_order_release);return true;
 }
 void run(){
#if defined(__aarch64__)
 uint64_t fpcr;asm volatile("mrs %0, fpcr":"=r"(fpcr));fpcr|=1ULL<<24;asm volatile("msr fpcr, %0"::"r"(fpcr));
#endif
#ifdef __ANDROID__
 tid.store(gettid(),std::memory_order_release);priority.store(setpriority(PRIO_PROCESS,gettid(),-16)==0?-16:0);
#endif
 for(;;){wait();wake.store(false,std::memory_order_release);if(quit.load(std::memory_order_acquire))return;execute(true);}
 }
};
Slot slots[2];std::atomic<int> workers{0};std::atomic<long long>batches{0},waitMax{0},waitTotal{0};
}
extern "C" void pr_parallel_stop(){for(int i=0;i<workers;i++){slots[i].quit.store(true);slots[i].signal();}for(int i=0;i<workers;i++){slots[i].thread.join();slots[i].close();}workers=0;}
extern "C" void pr_parallel_start(int enabled){pr_parallel_stop();batches=0;waitMax=0;waitTotal=0;for(auto&s:slots){s.jobs=0;s.timeNs=0;s.priority=0;s.tid=0;}if(!enabled)return;
 for(auto&s:slots){s.state=0;s.wake=false;s.quit=false;if(!s.init())break;try{s.thread=std::thread([slot=&s]{slot->run();});workers++;}catch(...){s.close();break;}}
}
extern "C" void pr_parallel_render(PrRenderJob fn,void** banks,float** outputs,int count,int frames){
 if(!workers||count<2){for(int i=0;i<count;i++)fn(banks[i],outputs[i],frames);return;}
 // Caller owns the largest bank, workers the next banks. Caller can claim
 // work before a delayed worker, avoiding a mandatory worker wakeup deadline.
 int assigned=count-1;if(assigned>workers)assigned=workers;
 for(int i=0;i<assigned;i++){auto&s=slots[i];s.fn=fn;s.bank=banks[i+1];s.output=outputs[i+1];s.frames=frames;s.state.store(1,std::memory_order_release);s.signal();}
 fn(banks[0],outputs[0],frames);for(int i=assigned+1;i<count;i++)fn(banks[i],outputs[i],frames);
 long long begin=now();for(int i=0;i<assigned;i++){auto&s=slots[i];s.execute(false);while(s.state.load(std::memory_order_acquire)!=3){
#if defined(__aarch64__)
 asm volatile("yield");
#elif defined(__x86_64__) || defined(_M_X64)
 _mm_pause();
#endif
 }}long long waited=now()-begin;batches.fetch_add(1,std::memory_order_relaxed);waitTotal.fetch_add(waited,std::memory_order_relaxed);long long previous=waitMax.load();if(waited>previous)waitMax.store(waited);
}
extern "C" void pr_parallel_stats(long long*out){out[0]=workers;out[1]=batches.load();out[2]=waitMax.load();out[3]=waitTotal.load();for(int i=0;i<2;i++){out[4+i*3]=slots[i].jobs.load();out[5+i*3]=slots[i].timeNs.load();out[6+i*3]=slots[i].priority.load();}}

extern "C" int pr_parallel_thread_ids(int*ids){int count=workers.load();for(int i=0;i<count;i++){ids[i]=slots[i].tid.load(std::memory_order_acquire);if(ids[i]<=0)return -1;}return count;}
