/* Same float32 DSP, four lanes at a time. No fast-math or reduced precision. */
#ifndef PR_DSP_MATH
#define PR_DSP_MATH
#include <string.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif
#if !defined(PR_DSP_SCALAR) && defined(__aarch64__)
#include <arm_neon.h>
#define PR_DSP_NEON 1
#elif !defined(PR_DSP_SCALAR) && defined(__SSE2__)
#include <emmintrin.h>
#define PR_DSP_SSE 1
#endif
typedef struct {float r,i;} PrComplex;
/* Only the audio thread opts into flushing subnormal (<~1e-38) float states.
   These are hundreds of dB below audibility, but can make long effect tails
   exceptionally expensive on a CPU. Normal float32 samples are unchanged. */
static void pr_dsp_denormals(void){
#if defined(__aarch64__)
 unsigned long fpcr;__asm__ volatile("mrs %0, fpcr":"=r"(fpcr));fpcr|=1UL<<24;__asm__ volatile("msr fpcr, %0"::"r"(fpcr));
#elif defined(__SSE2__)
 _mm_setcsr(_mm_getcsr()|0x8040u);
#endif
}
static float pr_dot128(const float*a,const float*b){
 float total=0;
#if PR_DSP_NEON
 float32x4_t sum=vdupq_n_f32(0);for(int i=0;i<128;i+=4)sum=vaddq_f32(sum,vmulq_f32(vld1q_f32(a+i),vld1q_f32(b+i)));float lanes[4];vst1q_f32(lanes,sum);for(int i=0;i<4;i++)total+=lanes[i];
#elif PR_DSP_SSE
 __m128 sum=_mm_setzero_ps();for(int i=0;i<128;i+=4)sum=_mm_add_ps(sum,_mm_mul_ps(_mm_loadu_ps(a+i),_mm_loadu_ps(b+i)));float lanes[4];_mm_storeu_ps(lanes,sum);for(int i=0;i<4;i++)total+=lanes[i];
#else
 for(int i=0;i<128;i++)total+=a[i]*b[i];
#endif
 return total;
}
static void pr_complex_add(PrComplex*sum,const PrComplex*x,const PrComplex*h){
#if PR_DSP_NEON
 for(int k=0;k<256;k+=4){float32x4x2_t a=vld2q_f32((const float*)(x+k)),b=vld2q_f32((const float*)(h+k)),v=vld2q_f32((const float*)(sum+k));v.val[0]=vaddq_f32(v.val[0],vsubq_f32(vmulq_f32(a.val[0],b.val[0]),vmulq_f32(a.val[1],b.val[1])));v.val[1]=vaddq_f32(v.val[1],vaddq_f32(vmulq_f32(a.val[0],b.val[1]),vmulq_f32(a.val[1],b.val[0])));vst2q_f32((float*)(sum+k),v);}
#elif PR_DSP_SSE
 const __m128 signs=_mm_set_ps(1,-1,1,-1);for(int k=0;k<256;k+=2){__m128 a=_mm_loadu_ps((const float*)(x+k)),b=_mm_loadu_ps((const float*)(h+k));__m128 re=_mm_shuffle_ps(a,a,_MM_SHUFFLE(2,2,0,0)),im=_mm_shuffle_ps(a,a,_MM_SHUFFLE(3,3,1,1)),swap=_mm_shuffle_ps(b,b,_MM_SHUFFLE(2,3,0,1));__m128 v=_mm_add_ps(_mm_mul_ps(re,b),_mm_mul_ps(_mm_mul_ps(im,swap),signs));_mm_storeu_ps((float*)(sum+k),_mm_add_ps(_mm_loadu_ps((const float*)(sum+k)),v));}
#else
 for(int k=0;k<256;k++){sum[k].r+=x[k].r*h[k].r-x[k].i*h[k].i;sum[k].i+=x[k].r*h[k].i+x[k].i*h[k].r;}
#endif
}
#endif
