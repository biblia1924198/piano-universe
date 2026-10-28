#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifndef PR_ENGINE_HEADER
#define PR_ENGINE_HEADER "../native/engine.h"
#endif
#include PR_ENGINE_HEADER
static double stamp(void){struct timespec t;clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t);return t.tv_sec+t.tv_nsec*1e-9;}
static int order(const void*a,const void*b){double x=*(const double*)a,y=*(const double*)b;return (x>y)-(x<y);}
int main(int argc,char**argv){int frames=argc>1?atoi(argv[1]):32,count=12000; if(frames<1||frames>256)return 1;
#ifdef PR_DSP_MATH
 pr_dsp_denormals();
#endif
 PrChannelFx*p[4];double *times=malloc(count*sizeof(double)),sum=0,checksum=0;float input[512],mix[512];
 for(int c=0;c<4;c++){p[c]=part_new(48000);if(!p[c])return 2;p[c]->chorus=.35f;p[c]->delay=.2f;p[c]->resonance=.45f;p[c]->pedal=127;p[c]->pedalMode=1;pr_space_config(&p[c]->space,1,c%3,70,35,65,80);pr_body_prepare(&p[c]->body,1,40,35,65,45,75,48000);}
 for(int block=-1500;block<count;block++){
  for(int i=0;i<frames;i++){float x=.18f*sinf((block*frames+i)*.071f)+.1f*sinf((block*frames+i)*.017f);input[i*2]=x;input[i*2+1]=x*.8f;}memset(mix,0,frames*2*sizeof(float));
  double begin=stamp();for(int c=0;c<4;c++)part_process(p[c],input,mix,frames,.45f,.8f,.35f,.82f);double duration=stamp()-begin;
  if(block>=0){times[block]=duration;sum+=duration;for(int i=0;i<frames*2;i++){if(!isfinite(mix[i]))return 3;checksum+=mix[i]*mix[i];}}
 }
 qsort(times,count,sizeof(double),order);printf("frames=%d channels=4 mean_us=%.3f p50_us=%.3f p95_us=%.3f p99_us=%.3f p999_us=%.3f max_us=%.3f checksum=%.9f\n",frames,sum/count*1e6,times[count/2]*1e6,times[count*95/100]*1e6,times[count*99/100]*1e6,times[count*999/1000]*1e6,times[count-1]*1e6,checksum);
 for(int c=0;c<4;c++)free(p[c]);free(times);return 0;
}
