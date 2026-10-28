#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*PrRenderJob)(void*,float*,int);
#ifdef PR_PARALLEL
void pr_parallel_start(int enabled);
void pr_parallel_stop(void);
void pr_parallel_render(PrRenderJob fn,void** banks,float** outputs,int count,int frames);
void pr_parallel_stats(long long* out);
int pr_parallel_thread_ids(int* ids);
#else
static void pr_parallel_start(int enabled){(void)enabled;}
static void pr_parallel_stop(void){}
static int pr_parallel_thread_ids(int* ids){(void)ids;return 0;}
static void pr_parallel_render(PrRenderJob fn,void** banks,float** outputs,int count,int frames){for(int i=0;i<count;i++)fn(banks[i],outputs[i],frames);}
#endif
#ifdef __cplusplus
}
#endif
