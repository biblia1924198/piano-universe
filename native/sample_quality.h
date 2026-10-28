/* Piano Room extension: four-point Catmull-Rom interpolation.
   Called only after TSF's types are defined. No allocation or extra audio queue. */
#ifndef PR_SAMPLE_QUALITY
#define PR_SAMPLE_QUALITY
static unsigned pr_sample_index(const tsf*f,const struct tsf_voice*v,long long index,unsigned pos){
 unsigned count=f->sampleCount,lo=v->region->offset,hi=v->region->end;
 if(lo>=count)lo=count?count-1:0;if(hi>count)hi=count;if(hi<=lo)hi=lo+1;
 unsigned start=v->loopStart,end=v->loopEnd;
 if(start<end&&end<count&&pos>=start&&pos<=end){
  long long length=(long long)end-start+1;
  if(index>end)index=start+(index-start)%length;
  else if(index<start&&v->qualityLooped)index=end-((start-1-index)%length);
 }
 if(index<lo)index=lo;if(index>=hi)index=hi-1;return (unsigned)index;
}
static float pr_interpolate(const tsf*f,const struct tsf_voice*v,unsigned pos,unsigned next,float t){
 if(!f->sampleCount||pos>=f->sampleCount)return 0;
 if(next>=v->region->end)next=v->region->end-1;
 if(next>=f->sampleCount)next=f->sampleCount-1;
 if(!v->sampleQuality)return f->fontSamples[pos]*(1-t)+f->fontSamples[next]*t;
 /* Interior samples need no wrap/clamp. Keep the exact old boundary path. */
 unsigned lo=v->region->offset,hi=v->region->end,start=v->loopStart,end=v->loopEnd;
 int inside=pos>lo && hi<=f->sampleCount && hi>2 && pos<hi-2;
 int loop=start<end && end<f->sampleCount && pos>=start && pos<=end;
 if(loop && (pos==start || end-pos<2))inside=0;
 float y0,y1=f->fontSamples[pos],y2,y3;
 if(inside){y0=f->fontSamples[pos-1];y2=f->fontSamples[pos+1];y3=f->fontSamples[pos+2];}
 else{y0=f->fontSamples[pr_sample_index(f,v,(long long)pos-1,pos)];y2=f->fontSamples[pr_sample_index(f,v,(long long)pos+1,pos)];y3=f->fontSamples[pr_sample_index(f,v,(long long)pos+2,pos)];}
 float a=.5f*(y3-y0)+1.5f*(y1-y2),b=y0-2.5f*y1+2*y2-.5f*y3,c=.5f*(y2-y0);
 return ((a*t+b)*t+c)*t+y1;
}
/* Immutable preset metadata, read on the control thread. Stereo regions with
   identical velocity ranges count as one zone. This describes sample selection,
   not a manufacturer's count of separately recorded velocity layers. */
static void pr_preset_quality(const tsf*f,int preset,int out[4]){
 for(int i=0;i<4;i++)out[i]=0;
 if(!f||preset<0||preset>=f->presetNum)return;
 const struct tsf_preset*p=&f->presets[preset];out[2]=p->regionNum;
 for(int r=0;r<p->regionNum;r++){unsigned rate=p->regions[r].sample_rate;if(rate){if(!out[0]||rate<(unsigned)out[0])out[0]=rate;if(rate>(unsigned)out[1])out[1]=rate;}}
 for(int velocity=1;velocity<=127;velocity++){
  int changed=velocity==1,active=0;
  for(int r=0;r<p->regionNum;r++){const struct tsf_region*z=&p->regions[r];if(z->lokey>60||z->hikey<60)continue;
#ifdef TSF_PR_BANK
  if(f->prBank&&z->prTrigger)continue;
#endif
 int now=velocity>=z->lovel&&velocity<=z->hivel;int before=velocity>1&&velocity-1>=z->lovel&&velocity-1<=z->hivel;if(now)active=1;if(now!=before)changed=1;}
  if(changed&&active)out[3]++;
 }
}
#endif
