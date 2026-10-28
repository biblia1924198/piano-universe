/* Original reduced coloration model for already-recorded piano samples.
 * 4 broad body modes + 12 soundboard modes, feed-forward only. Not a FEM
 * simulation, measured IR, acoustic calibration, or a replacement for samples.
 * Causal lid radiation tilt; no lookahead, allocations or locks in processing.
 */
#ifndef PR_PIANO_BODY
#define PR_PIANO_BODY
#include <math.h>
#include <string.h>
#define PR_BODY_MODES 16
typedef struct {
 float co[16],si[16],loss[16],targetCo[16],targetSi[16],targetLoss[16];
 float x[16],y[16],dcX,dcY,low[2],dcPole,lowAlpha,smooth;
 float bodyNow,boardNow,lidNow;int enabled,body,board,size,decay,lid,rate,active;
} PrPianoBody;
static int pr_body_value(int x){return x<0?0:x>100?100:x;}
/* Called on preparation/control thread, or a stream-rate reconfiguration. */
static void pr_body_prepare(PrPianoBody*b,int enabled,int body,int board,int size,int decay,int lid,int rate){
 const float frequency[16]={64,111,173,257,92,136,193,267,349,463,611,793,1049,1360,1768,2301};
 b->enabled=enabled!=0;b->body=pr_body_value(body);b->board=pr_body_value(board);b->size=pr_body_value(size);b->decay=pr_body_value(decay);b->lid=pr_body_value(lid);b->rate=rate;
 float scale=powf(2,(50-b->size)*.007f),eta=.012f+.078f*b->decay/100.f;
 b->smooth=1-expf(-1.f/(rate*.012f));b->dcPole=expf(-6.283185307f*18/rate);b->lowAlpha=1-expf(-6.283185307f*1800/rate);
 for(int i=0;i<16;i++){float hz=frequency[i]*scale,angle=6.283185307f*hz/rate;b->targetCo[i]=cosf(angle);b->targetSi[i]=sinf(angle);b->targetLoss[i]=-expm1f(-3.141592654f*hz*(i<4?eta*2:eta)/rate);}
}
static void pr_body_reset(PrPianoBody*b){memset(b->x,0,sizeof(b->x));memset(b->y,0,sizeof(b->y));memset(b->low,0,sizeof(b->low));b->dcX=b->dcY=0;b->active=0;}
static void pr_body_init(PrPianoBody*b,int rate){memset(b,0,sizeof(*b));pr_body_prepare(b,0,0,0,50,50,100,rate);memcpy(b->co,b->targetCo,sizeof(b->co));memcpy(b->si,b->targetSi,sizeof(b->si));memcpy(b->loss,b->targetLoss,sizeof(b->loss));b->lidNow=1;}
static void pr_body_apply(PrPianoBody*b,const PrPianoBody*next){
 b->enabled=next->enabled;b->body=next->body;b->board=next->board;b->size=next->size;b->decay=next->decay;b->lid=next->lid;
 memcpy(b->targetCo,next->targetCo,sizeof(b->co));memcpy(b->targetSi,next->targetSi,sizeof(b->si));memcpy(b->targetLoss,next->targetLoss,sizeof(b->loss));
}
static void pr_body_tick(PrPianoBody*b,float*left,float*right){
 float body=b->enabled?b->body*.0022f:0,board=b->enabled?b->board*.0028f:0,lid=b->enabled?.4f+.6f*b->lid/100.f:1;
 b->bodyNow+=(body-b->bodyNow)*b->smooth;b->boardNow+=(board-b->boardNow)*b->smooth;b->lidNow+=(lid-b->lidNow)*b->smooth;
 /* Float smoothing may stall before 1.0; finish the inaudible final step. */
 if(fabsf(body-b->bodyNow)<.000001f)b->bodyNow=body;if(fabsf(board-b->boardNow)<.000001f)b->boardNow=board;if(fabsf(lid-b->lidNow)<.0001f)b->lidNow=lid;
 if(fabsf(b->bodyNow)<.000001f&&fabsf(b->boardNow)<.000001f&&fabsf(b->lidNow-1)<.000001f){b->bodyNow=b->boardNow=0;b->lidNow=1;if(b->active)pr_body_reset(b);return;}
 b->active=1;float a=*left,c=*right;if(!isfinite(a))a=0;if(!isfinite(c))c=0;
 float input=fmaxf(-8,fminf(8,(a+c)*.5f)),drive=input-b->dcX+b->dcPole*b->dcY;b->dcX=input;b->dcY=drive;float l=0,r=0;
 if(b->bodyNow>.000001f||b->boardNow>.000001f)for(int i=0;i<16;i++){
  b->co[i]+=(b->targetCo[i]-b->co[i])*b->smooth;b->si[i]+=(b->targetSi[i]-b->si[i])*b->smooth;b->loss[i]+=(b->targetLoss[i]-b->loss[i])*b->smooth;
  float radius=1-b->loss[i],x=radius*(b->co[i]*b->x[i]-b->si[i]*b->y[i])+drive*b->loss[i]*2,y=radius*(b->si[i]*b->x[i]+b->co[i]*b->y[i]);
  b->x[i]=fabsf(x)<1e-20f?0:x;b->y[i]=fabsf(y)<1e-20f?0:y;
  float amount=i<4?b->bodyNow*.25f:b->boardNow/12.f,pan=i<4?0:(i-9.5f)*.028f;l+=x*amount*(1-pan);r+=x*amount*(1+pan);
 }
 a+=l;c+=r;b->low[0]+=(a-b->low[0])*b->lowAlpha;b->low[1]+=(c-b->low[1])*b->lowAlpha;
 /* Full lid opening is exactly neutral when body/board are zero. */
 *left=b->lidNow==1?a:b->low[0]+b->lidNow*(a-b->low[0]);*right=b->lidNow==1?c:b->low[1]+b->lidNow*(c-b->low[1]);
}
#endif
