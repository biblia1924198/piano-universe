/* Piano Room 0.11 — original, reduced sympathetic string model.
 * Parallel complex resonators: 88 notes, three inharmonic partials per note.
 * Feed-forward excitation from the sampler (no feedback to the dry signal).
 * Coefficients are prepared at stream rate changes; audio processing allocates
 * nothing, locks nothing, and adds no look-ahead or playback queue.
 * This is a generic A440 model, not a measured physical replica of a piano.
 */
#ifndef PR_PIANO_RESONANCE_H
#define PR_PIANO_RESONANCE_H
#include <math.h>
#include <string.h>
#include "dsp_math.h"
#define PR_RESONANCE_MODES (88*3)
typedef struct {
 float x[PR_RESONANCE_MODES],y[PR_RESONANCE_MODES];
 float co[PR_RESONANCE_MODES],si[PR_RESONANCE_MODES];
 float openLoss[PR_RESONANCE_MODES],closedLoss[PR_RESONANCE_MODES];
 float loss[PR_RESONANCE_MODES],targetLoss[PR_RESONANCE_MODES];
 float weight[PR_RESONANCE_MODES],left[PR_RESONANCE_MODES];
 float coupling[PR_RESONANCE_MODES],couplingTarget[PR_RESONANCE_MODES];
 float dcX,dcY,dcPole,smooth,drive;int rate,active;
} PrPianoResonance;
static void pr_res_reset(PrPianoResonance*r){
 memset(r->x,0,sizeof(r->x));memset(r->y,0,sizeof(r->y));
 memcpy(r->loss,r->closedLoss,sizeof(r->loss));
 memset(r->coupling,0,sizeof(r->coupling));r->dcX=r->dcY=0;r->active=0;
}
static void pr_res_init(PrPianoResonance*r,int rate){
 memset(r,0,sizeof(*r));r->rate=rate;r->drive=.65f/rate;
 r->smooth=1-expf(-1.f/(rate*.005f));r->dcPole=expf(-6.28318530718f*18/rate);
 for(int key=21;key<=108;key++)for(int h=1;h<=3;h++){
  int k=(key-21)*3+h-1;double f=440*pow(2,(key-69)/12.0);
  double stiffness=.000035+.00013*pow((key-21)/87.0,3);
  double hz=f*h*sqrt((1+stiffness*h*h)/(1+stiffness));
  double w=6.283185307179586*hz/rate;
  r->co[k]=(float)cos(w);r->si[k]=(float)sin(w);
  float seconds=(float)(3.4/(1+hz/1800));
  r->openLoss[k]=-expm1f(-1.f/(rate*seconds));
  r->closedLoss[k]=-expm1f(-1.f/(rate*(.012f+.024f/(1+(float)hz/700))));
  r->weight[k]=hz<rate*.45?1.f/(h*(1+(float)hz/7000)):0;
  r->left[k]=.65f-.3f*(key-21)/87.f;
 }pr_res_reset(r);
}
static void pr_res_profile(PrPianoResonance*r,const float*profile){
 for(int n=0;n<88;n++){
  float hz=profile[n],tau=profile[n+88],confidence=profile[n+176];
  double generic=440*pow(2,(n+21-69)/12.0);
  if(!isfinite(hz)||!isfinite(tau)||!isfinite(confidence)||confidence<.5f||confidence>1||hz<generic*.97||hz>generic*1.03)continue;
  double fundamental=generic+(hz-generic)*confidence;
  for(int h=1;h<=3;h++){int k=n*3+h-1;double stiffness=.000035+.00013*pow(n/87.0,3),frequency=fundamental*h*sqrt((1+stiffness*h*h)/(1+stiffness));
   r->co[k]=cos(6.283185307179586*frequency/r->rate);r->si[k]=sin(6.283185307179586*frequency/r->rate);
   if(tau>=.2f&&tau<=12){float measured=tau/(1+(h-1)*.45f),base=3.4f/(1+frequency/1800);float seconds=base+(measured-base)*confidence;r->openLoss[k]=-expm1f(-1.f/(r->rate*seconds));}
  }
 }
}
/* Optional sample-derived upper partials. No measured value is trusted unless
   it passes pitch, confidence, amplitude and Nyquist bounds independently. */
static void pr_res_harmonics(PrPianoResonance*r,const float*p,int count){
 if(count!=792)return;
 for(int n=0;n<88;n++){if(!isfinite(p[n+176])||p[n+176]<.5f||p[n+176]>1)continue;
  for(int h=2;h<=3;h++){int k=n*3+h-1;float hz=p[264+k],amp=p[528+k];double base=p[n]*h;
   if(!isfinite(hz)||!isfinite(amp)||hz<base*.9995||hz>base*1.012||amp<.05f||amp>1.5f)continue;
   if(hz>=r->rate*.45f){r->weight[k]=0;continue;}
   r->co[k]=cos(6.283185307179586*hz/r->rate);r->si[k]=sin(6.283185307179586*hz/r->rate);
   /* Bound excitation to the generic energy budget. Spectrum only shapes it. */
   r->weight[k]*=fmaxf(.25f,fminf(1.5f,amp*h));
  }
 }
}
/* Transfer only precomputed coefficients, retaining the current vibration. */
static void pr_res_coefficients(PrPianoResonance*r,const PrPianoResonance*next){
 memcpy(r->co,next->co,sizeof(r->co));memcpy(r->si,next->si,sizeof(r->si));
 memcpy(r->openLoss,next->openLoss,sizeof(r->openLoss));memcpy(r->closedLoss,next->closedLoss,sizeof(r->closedLoss));memcpy(r->weight,next->weight,sizeof(r->weight));
}
static void pr_res_prepare(PrPianoResonance*r,const unsigned short*held,int transpose,int pedal,int progressive){
 float opening=progressive?fmaxf(0,fminf(1,(pedal-8)/104.f)):(pedal>=64?1:0);
 /* Damper position changes loss as well as excitation, not just wet volume. */
 opening=opening*opening*(3-2*opening);
 for(int key=21;key<=108;key++){
  int input=key-transpose;float a=(key>=89||(input>=0&&input<128&&held[input]))?1:opening;
  for(int h=0;h<3;h++){int k=(key-21)*3+h;
   r->targetLoss[k]=r->closedLoss[k]+a*(r->openLoss[k]-r->closedLoss[k]);
   r->couplingTarget[k]=a*r->drive*r->weight[k];
  }
 }
}
static void pr_res_tick(PrPianoResonance*r,float input,float amount,float*outL,float*outR){
 *outL=*outR=0;
 if(amount<=.00001f){if(r->active)pr_res_reset(r);return;}r->active=1;
 /* Bound pathological sources locally; the output limiter remains downstream. */
 if(!isfinite(input))input=0;input=fmaxf(-8,fminf(8,input));
 float drive=input-r->dcX+r->dcPole*r->dcY;r->dcX=input;r->dcY=drive;
 float l=0,rr=0;
#if PR_DSP_NEON
 const float32x4_t one=vdupq_n_f32(1),smooth=vdupq_n_f32(r->smooth),excitation=vdupq_n_f32(drive),tiny=vdupq_n_f32(1e-20f);
 for(int k=0;k<PR_RESONANCE_MODES;k+=4){
  float32x4_t loss=vld1q_f32(r->loss+k),coupling=vld1q_f32(r->coupling+k);loss=vaddq_f32(loss,vmulq_f32(vsubq_f32(vld1q_f32(r->targetLoss+k),loss),smooth));coupling=vaddq_f32(coupling,vmulq_f32(vsubq_f32(vld1q_f32(r->couplingTarget+k),coupling),smooth));vst1q_f32(r->loss+k,loss);vst1q_f32(r->coupling+k,coupling);
  float32x4_t radius=vsubq_f32(one,loss),co=vld1q_f32(r->co+k),si=vld1q_f32(r->si+k),oldx=vld1q_f32(r->x+k),oldy=vld1q_f32(r->y+k);
  float32x4_t x=vaddq_f32(vmulq_f32(radius,vsubq_f32(vmulq_f32(co,oldx),vmulq_f32(si,oldy))),vmulq_f32(excitation,coupling)),y=vmulq_f32(radius,vaddq_f32(vmulq_f32(si,oldx),vmulq_f32(co,oldy)));
  vst1q_f32(r->x+k,vbslq_f32(vcgeq_f32(vabsq_f32(x),tiny),x,vdupq_n_f32(0)));vst1q_f32(r->y+k,vbslq_f32(vcgeq_f32(vabsq_f32(y),tiny),y,vdupq_n_f32(0)));float values[4];vst1q_f32(values,x);for(int j=0;j<4;j++){l+=values[j]*r->left[k+j];rr+=values[j]*(1-r->left[k+j]);}
 }
#elif PR_DSP_SSE
 const __m128 one=_mm_set1_ps(1),smooth=_mm_set1_ps(r->smooth),excitation=_mm_set1_ps(drive),tiny=_mm_set1_ps(1e-20f),sign=_mm_set1_ps(-0.f);
 for(int k=0;k<PR_RESONANCE_MODES;k+=4){
  __m128 loss=_mm_loadu_ps(r->loss+k),coupling=_mm_loadu_ps(r->coupling+k);loss=_mm_add_ps(loss,_mm_mul_ps(_mm_sub_ps(_mm_loadu_ps(r->targetLoss+k),loss),smooth));coupling=_mm_add_ps(coupling,_mm_mul_ps(_mm_sub_ps(_mm_loadu_ps(r->couplingTarget+k),coupling),smooth));_mm_storeu_ps(r->loss+k,loss);_mm_storeu_ps(r->coupling+k,coupling);
  __m128 radius=_mm_sub_ps(one,loss),co=_mm_loadu_ps(r->co+k),si=_mm_loadu_ps(r->si+k),oldx=_mm_loadu_ps(r->x+k),oldy=_mm_loadu_ps(r->y+k);
  __m128 x=_mm_add_ps(_mm_mul_ps(radius,_mm_sub_ps(_mm_mul_ps(co,oldx),_mm_mul_ps(si,oldy))),_mm_mul_ps(excitation,coupling)),y=_mm_mul_ps(radius,_mm_add_ps(_mm_mul_ps(si,oldx),_mm_mul_ps(co,oldy)));
  _mm_storeu_ps(r->x+k,_mm_and_ps(_mm_cmpge_ps(_mm_andnot_ps(sign,x),tiny),x));_mm_storeu_ps(r->y+k,_mm_and_ps(_mm_cmpge_ps(_mm_andnot_ps(sign,y),tiny),y));float values[4];_mm_storeu_ps(values,x);for(int j=0;j<4;j++){l+=values[j]*r->left[k+j];rr+=values[j]*(1-r->left[k+j]);}
 }
#else
 for(int k=0;k<PR_RESONANCE_MODES;k++){
  float loss=r->loss[k]+(r->targetLoss[k]-r->loss[k])*r->smooth;r->loss[k]=loss;float radius=1-loss;
  float coupling=r->coupling[k]+(r->couplingTarget[k]-r->coupling[k])*r->smooth;r->coupling[k]=coupling;
  float x=radius*(r->co[k]*r->x[k]-r->si[k]*r->y[k])+drive*coupling;
  float y=radius*(r->si[k]*r->x[k]+r->co[k]*r->y[k]);
  /* Flush sub-audible state to avoid denormal CPU spikes during long tails. */
  r->x[k]=fabsf(x)<1e-20f?0:x;r->y[k]=fabsf(y)<1e-20f?0:y;
  l+=x*r->left[k];rr+=x*(1-r->left[k]);
 }
#endif
*outL=l*amount;*outR=rr*amount;
}
#endif
