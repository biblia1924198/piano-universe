#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../native/engine.h"
int main(void){
 FX*f=calloc(1,sizeof(FX));config_fx(f,0,1,1,0,120,4,.6f);f->protect=1;
 float input[BLOCK*2];short pcm[BLOCK*2];
 for(int i=0;i<BLOCK;i++)input[i*2]=input[i*2+1]=.2f*sinf(i*.04f);
 output_audio(f,input,pcm,BLOCK);for(int i=0;i<BLOCK*2;i++)assert(abs(pcm[i]-(short)(input[i]*32767))<=1);
 for(int i=0;i<BLOCK;i++){input[i*2]=8*sinf(i*.04f);input[i*2+1]=input[i*2]*.5f;}
 output_audio(f,input,pcm,BLOCK);assert(f->peakBefore>7&&f->limitedSamples>0&&f->limiterGain<.2f);for(int i=0;i<BLOCK*2;i++)assert(abs(pcm[i])<=30803);
 for(int i=2;i<BLOCK;i++)if(pcm[i*2]!=0)assert(abs(pcm[i*2]/2-pcm[i*2+1])<=1);
 for(int i=0;i<BLOCK*2;i++)input[i]=0;for(int k=0;k<180;k++)output_audio(f,input,pcm,BLOCK);assert(f->limiterGain>.99f);
 input[0]=NAN;input[1]=INFINITY;output_audio(f,input,pcm,BLOCK);assert(pcm[0]==0&&pcm[1]==0&&f->invalidSamples==2);
 panic_fx(0,0,f);assert(f->protect==1);f->toneLeft=RATE*2;int crossings=0,previous=0,peak=0,total=0;
 while(f->toneLeft>0){render_audio(0,0,f,0,0,pcm,BLOCK);for(int i=0;i<BLOCK;i++){int v=pcm[i*2];assert(v==pcm[i*2+1]);if(previous<=0&&v>0)crossings++;if(v!=0)previous=v;if(abs(v)>peak)peak=abs(v);total++;}}
 assert(crossings>=879&&crossings<=881&&peak>3000&&peak<4000&&total>=RATE*2);
 panic_fx(0,0,f);f->rate=48000;f->toneLeft=96000;crossings=0;previous=0;total=0;
 while(f->toneLeft>0){render_audio(0,0,f,0,0,pcm,BLOCK);for(int i=0;i<BLOCK;i++){int v=pcm[i*2];if(previous<=0&&v>0)crossings++;if(v!=0)previous=v;total++;}}
 assert(crossings>=879&&crossings<=881&&total>=96000&&total<96256);
 printf("PASS: transparent normal PCM, stereo-linked overload protection, release, invalid samples, panic preservation and clean 440Hz stereo tone (%d crossings).\n",crossings);free(f);return 0;
}
