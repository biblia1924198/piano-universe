#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../native/engine.h"
static void interpolation(void){
 float data[1024];tsf f={0};struct tsf_region r={0};struct tsf_voice v={0};f.sampleCount=1024;f.fontSamples=data;r.end=1024;v.region=&r;
 const double frequencies[]={.02,.1,.2};
 for(int k=0;k<3;k++){
  double w=frequencies[k]*6.283185307179586,linear=0,cubic=0;for(int i=0;i<1024;i++)data[i]=(float)sin(w*i);
  for(int pos=2;pos<1000;pos++)for(int j=1;j<10;j++){float t=j*.1f;double exact=sin(w*(pos+t));v.sampleQuality=0;double a=pr_interpolate(&f,&v,pos,pos+1,t)-exact;v.sampleQuality=1;double b=pr_interpolate(&f,&v,pos,pos+1,t)-exact;linear+=a*a;cubic+=b*b;}
  assert(cubic<linear*.4);printf("Sine %.2f cycles/sample: cubic/linear RMS ratio %.4f\n",frequencies[k],sqrt(cubic/linear));
 }
 for(int i=0;i<1024;i++)data[i]=i*.001f;f.sampleCount=16;r.offset=2;r.end=12;v.sampleQuality=1;v.loopStart=4;v.loopEnd=8;
 assert(pr_sample_index(&f,&v,-1,2)==2&&pr_sample_index(&f,&v,13,11)==11);assert(pr_sample_index(&f,&v,9,8)==4&&pr_sample_index(&f,&v,10,8)==5);assert(fabsf(pr_interpolate(&f,&v,8,4,1)-data[4])<1e-7f);
 assert(pr_sample_index(&f,&v,3,4)==3);v.qualityLooped=1;assert(pr_sample_index(&f,&v,3,4)==8);
 for(int loop=0;loop<2;loop++){v.qualityLooped=loop;for(unsigned pos=2;pos<12;pos++)for(int j=0;j<11;j++)assert(isfinite(pr_interpolate(&f,&v,pos,pos+1,j*.1f)));}
 float*edge=malloc(2*sizeof(float));edge[0]=.4f;edge[1]=-.2f;f.fontSamples=edge;f.sampleCount=2;r.offset=0;r.end=2;v.loopStart=v.loopEnd=0;for(int j=0;j<10;j++)assert(isfinite(pr_interpolate(&f,&v,1,2,j*.1f)));free(edge);f.sampleCount=0;assert(pr_interpolate(&f,&v,0,1,.5f)==0);
}
static double response(int rate,int band,float db,double frequency){
 PrToneEq eq={0};float gains[3]={0};gains[band]=db;pr_eq_request(&eq,1,gains[0],gains[1],gains[2],rate);pr_eq_reset(&eq,rate);double input=0,output=0;float samples[256],original[256];
 for(int block=0;block<rate*2/128;block++){for(int i=0;i<128;i++)samples[i*2]=samples[i*2+1]=.1f*(float)sin(6.283185307179586*frequency*(block*128+i)/rate);memcpy(original,samples,sizeof(samples));pr_eq_process(&eq,samples,128);for(int i=0;i<128;i++){assert(samples[i*2]==samples[i*2+1]&&isfinite(samples[i*2]));if(block*128+i>rate/2){input+=original[i*2]*original[i*2];output+=samples[i*2]*samples[i*2];}}}
 return 10*log10(output/input);
}
static void equalization(void){
 for(int rate=44100;rate<=48000;rate+=3900){
  for(int band=0;band<3;band++)for(int sign=-1;sign<=1;sign+=2)assert(fabs(response(rate,band,sign*6,band==0?20:band==1?2000:16000)-sign*6)<.2);assert(fabs(response(rate,1,0,1739))<1e-5);
  PrToneEq eq={0};float samples[256],original[256];for(int i=0;i<256;i++)samples[i]=sinf(i*.1f)*.2f;memcpy(original,samples,sizeof(samples));pr_eq_process(&eq,samples,128);assert(!memcmp(samples,original,sizeof(samples)));
  pr_eq_request(&eq,1,6,-6,6,rate);for(int block=0;block<25;block++){for(int i=0;i<128;i++){samples[i*2]=.1f;samples[i*2+1]=0;}if(block==1)pr_eq_request(&eq,1,-2,1,2,rate);pr_eq_process(&eq,samples,128);for(int i=0;i<128;i++)assert(isfinite(samples[i*2])&&fabsf(samples[i*2])<.3f&&samples[i*2+1]==0);}assert(!eq.fade&&!eq.dirty&&eq.db[0]==-2&&eq.db[2]==2);
  pr_eq_request(&eq,0,0,0,0,rate);for(int block=0;block<8;block++){memcpy(samples,original,sizeof(samples));pr_eq_process(&eq,samples,128);}assert(!memcmp(samples,original,sizeof(samples)));
 }
 FX*f=calloc(1,sizeof(FX));pr_eq_request(&f->eq,1,2,1,-1,44100);panic_fx(0,0,f);assert(f->eq.enabled&&f->eq.db[0]==2&&f->eq.current.enabled&&!f->eq.fade);pr_eq_reset(&f->eq,48000);assert(f->eq.rate==48000&&f->eq.db[2]==-1);free(f);puts("PASS: EQ frequency response 44.1/48kHz, neutral/bypass, stereo isolation, finite transitions, latest request, panic/rate preservation.");
}
static void metadata(void){
 tsf f={0};struct tsf_preset p={0};struct tsf_region regions[3]={0};int q[4];f.presets=&p;f.presetNum=1;p.regions=regions;p.regionNum=3;for(int i=0;i<3;i++){regions[i].lokey=0;regions[i].hikey=127;regions[i].sample_rate=i==0?22050:44100;regions[i].lovel=i==2?65:1;regions[i].hivel=i==2?127:64;}pr_preset_quality(&f,0,q);assert(q[0]==22050&&q[1]==44100&&q[2]==3&&q[3]==2);pr_preset_quality(&f,1,q);assert(q[0]==0&&q[3]==0);
}
static void real_font(const char*path){
 tsf*base=load_font(path);assert(base);tsf*f=copy_font(base,128);assert(f);tsf_channel_set_presetindex(f,0,0);tsf_channel_note_on(f,0,60,.8f);int old=0,new=0;for(int i=0;i<f->voiceNum;i++)if(f->voices[i].playingPreset>=0){old++;assert(!f->voices[i].sampleQuality);}f->sampleQuality=1;tsf_channel_note_on(f,0,61,.8f);for(int i=0;i<f->voiceNum;i++)if(f->voices[i].playingPreset>=0){if(f->voices[i].playingKey==60)assert(!f->voices[i].sampleQuality);if(f->voices[i].playingKey==61){new++;assert(f->voices[i].sampleQuality);}}assert(old&&new);
 int q[4];pr_preset_quality(f,0,q);printf("Preset %s: %d–%dHz, %d regions, %d velocity zones at C4\n",tsf_get_presetname(f,0),q[0],q[1],q[2],q[3]);FX*fx=calloc(1,sizeof(FX));config_fx(fx,.3f,1,.7f,0,120,4,.6f);fx->protect=1;pr_eq_request(&fx->eq,1,-1,1,2,44100);float pcm[512],gains[4]={.8f,0,0,0};tsf*banks[4]={f,0,0,0};clock_t start=clock();tsf_channel_midi_control(f,0,64,127);
 for(int i=0;i<240;i++){smart_note(f,36+i%49,60+i%68);tsf_channel_note_off(f,0,36+i%49);render_mixer_float(banks,fx,gains,pcm,128);for(int s=0;s<256;s++)assert(isfinite(pcm[s])&&fabsf(pcm[s])<=.981f);}assert(!smartDropped);printf("240 sustain notes HQ+EQ: %.3fs audio in %.3fs host CPU; not a phone measurement\n",240*128/44100.0,(double)(clock()-start)/CLOCKS_PER_SEC);tsf_close(f);tsf_close(base);free(fx);
}
int main(int argc,char**argv){assert(argc==2);interpolation();equalization();metadata();real_font(argv[1]);puts("PASS: interpolation accuracy, guarded samples/loops, new-note quality, HQ+EQ sustain and source metadata.");return 0;}
