/* Sample-derived estimates, not acoustic measurements of strings or coupling.
 * Runs on the import/analysis worker over immutable bank samples. No rendering.
 * Profile layout: 88 fundamental Hz, 88 amplitude decay seconds, 88 confidence.
 * Looped samples do not provide a trustworthy natural decay estimate. */
#ifndef PR_PIANO_PROFILE
#define PR_PIANO_PROFILE
#include <math.h>
static double pr_profile_rms(tsf*f,struct tsf_region*r,double begin,double seconds){
 unsigned first=r->offset+(unsigned)(begin*r->sample_rate),last=first+(unsigned)(seconds*r->sample_rate);
 if(last>=r->end||first>=last)return 0;if(r->loop_mode&&last>=r->loop_start)return 0;
 double sum=0;for(unsigned i=first;i<last;i+=4){double x=f->fontSamples[i];sum+=x*x;}
 return sqrt(sum/((last-first+3)/4));
}
static void pr_profile_estimate(tsf*f,int preset,float*out){
 memset(out,0,264*sizeof(float));if(!f||preset<0||preset>=f->presetNum)return;
 struct tsf_preset*p=&f->presets[preset];
 for(int key=21;key<=108;key++){
  struct tsf_region*r=0;int distance=999;
  for(int j=0;j<p->regionNum;j++){struct tsf_region*q=&p->regions[j];if(q->prTrigger||key<q->lokey||key>q->hikey||80<q->lovel||80>q->hivel||q->sample_rate<8000)continue;int d=key-q->pitch_keycenter;if(d<0)d=-d;if(d<distance){distance=d;r=q;}}
  if(!r)continue;
  double expected=440*pow(2,(r->pitch_keycenter-69)/12.0),period=r->sample_rate/expected;
  long long start=(long long)r->offset+(int)(r->sample_rate*.04);int window=(int)(r->sample_rate*.18);
  int first=(int)floor(period*.97),last=(int)ceil(period*1.03);
  if(first<2||last-first>256||start+window+last>=(int)r->end)continue;
  double score[260],best=-2;int bi=0;
  for(int lag=first;lag<=last;lag++){
   double xx=0,yy=0,xy=0;for(int i=0;i<window;i+=4){double x=f->fontSamples[start+i],y=f->fontSamples[start+i+lag];xx+=x*x;yy+=y*y;xy+=x*y;}
   double value=xx>1e-10&&yy>1e-10?xy/sqrt(xx*yy):-1;score[lag-first]=value;if(value>best){best=value;bi=lag-first;}
  }
  if(best<.8||bi==0||bi==last-first)continue;
  double a=score[bi-1],b=score[bi],c=score[bi+1],den=a-2*b+c;
  double delta=den< -1e-10?.5*(a-c)/den:0;if(fabs(delta)>.5)delta=0;
  double frequency=r->sample_rate/(first+bi+delta)*pow(2,((key-r->pitch_keycenter+r->transpose+r->tune/100.0)*r->pitch_keytrack/100.0)/12.0);
  double standard=440*pow(2,(key-69)/12.0);if(frequency<standard*.97||frequency>standard*1.03)continue;
  int n=key-21;out[n]=(float)frequency;out[n+176]=(float)fmax(0,fmin(1,(best-.8)/.2));
  double early=pr_profile_rms(f,r,.1,.1),late=pr_profile_rms(f,r,.7,.1);
  if(early>1e-5&&late>1e-6&&late<early*.95){double tau=.6/log(early/late);if(tau>=.2&&tau<=12)out[n+88]=(float)tau;}
 }
}
/* v2 adds 264 measured partial frequencies and 264 relative amplitudes.
   Narrow spectral search with Hann-windowed Goertzel; low confidence and weak
   partials stay zero. Estimates belong to the samples, not the original strings. */
static double pr_profile_power(const float*x,int count,double frequency,double rate){
 double co=2*cos(6.283185307179586*frequency/rate),a=0,b=0;
 for(int i=0;i<count;i++){double v=x[i]+co*a-b;b=a;a=v;}
 return fmax(0,a*a+b*b-co*a*b);
}
static void pr_profile_estimate_extended(tsf*f,int preset,float*out){
 memset(out,0,792*sizeof(float));pr_profile_estimate(f,preset,out);if(!f||preset<0||preset>=f->presetNum)return;
 struct tsf_preset*p=&f->presets[preset];float signal[8192];
 for(int n=0;n<88;n++){
  if(out[n+176]<.5f)continue;int key=n+21,distance=999;struct tsf_region*r=0;
  for(int j=0;j<p->regionNum;j++){struct tsf_region*q=&p->regions[j];if(q->prTrigger||key<q->lokey||key>q->hikey||80<q->lovel||80>q->hivel||q->sample_rate<8000)continue;int d=key-q->pitch_keycenter;if(d<0)d=-d;if(d<distance){distance=d;r=q;}}
  if(!r)continue;int step=r->sample_rate>48000?2:1,count=(int)(r->sample_rate*.3)/step;long long start=(long long)r->offset+(int)(r->sample_rate*.07);if(count>8192)count=8192;if(start+count*step>=(int)r->end||count<128)continue;
  double scale=pow(2,((key-r->pitch_keycenter+r->transpose+r->tune/100.0)*r->pitch_keytrack/100.0)/12.0),base=out[n]/scale,rate=(double)r->sample_rate/step,energy=0;
  for(int i=0;i<count;i++){double v=f->fontSamples[start+i*step]*(.5-.5*cos(6.283185307179586*i/(count-1)));signal[i]=v;energy+=v*v;}
  double fundamental=pr_profile_power(signal,count,base,rate);if(fundamental<1e-8||energy<1e-8)continue;
  out[264+n*3]=out[n];out[528+n*3]=1;
  for(int h=2;h<=3;h++){
   double center=base*h;if(center*1.012>rate*.45)continue;double powers[33],best=0;int bi=0;
   for(int j=0;j<33;j++){double freq=center*(.996+j*.0005);powers[j]=pr_profile_power(signal,count,freq,rate);if(powers[j]>best){best=powers[j];bi=j;}}
   if(bi==0||bi==32||best/fundamental<.0025||best/(energy*count)<.003)continue;
   double a=powers[bi-1],b=powers[bi],c=powers[bi+1],den=a-2*b+c,delta=den< -1e-12?.5*(a-c)/den:0;if(fabs(delta)>.5)delta=0;
   double freq=center*(.996+(bi+delta)*.0005);if(freq<base*h*.9995||freq>base*h*1.012)continue;
   out[264+n*3+h-1]=freq*scale;out[528+n*3+h-1]=fmin(1.5,sqrt(best/fundamental));
  }
 }
}
#endif
