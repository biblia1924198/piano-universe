/* Original Piano Room DSP. Measured early IR + orthogonal FDN late tail.
 * Direct first partition: no dry/look-ahead latency. Remaining partitions use
 * overlap-add, scheduled at their causal offsets with work distributed per frame. Immutable plans are prepared
 * on the control thread, never allocated or transformed in an audio callback.
 * IRs are MONO measurements; stereo width comes from the algorithmic late tail.
 */
#ifndef PR_HYBRID_SPACE
#define PR_HYBRID_SPACE
#include <math.h>
#include <string.h>
#include "room_ir.h"
#include "dsp_math.h"
#define PR_IR_BLOCK 128
#define PR_IR_FFT 256
#define PR_IR_PARTS 64
typedef struct {int count;float head[128],headReverse[128];PrComplex h[PR_IR_PARTS][256];} PrRoomPlan;
static PrRoomPlan pr_rooms[3][3];static float pr_twR[128],pr_twI[128];static int pr_rooms_ready,pr_head_any;
static void pr_fft(PrComplex*a,int inverse){
 for(int i=1,j=0;i<256;i++){int bit=128;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j){PrComplex t=a[i];a[i]=a[j];a[j]=t;}}
 for(int len=2;len<=256;len*=2)for(int i=0;i<256;i+=len)for(int j=0;j<len/2;j++){
  int q=j*256/len;float wr=pr_twR[q],wi=inverse?-pr_twI[q]:pr_twI[q];PrComplex b=a[i+j+len/2],x=a[i+j];float re=b.r*wr-b.i*wi,im=b.r*wi+b.i*wr;a[i+j]=(PrComplex){x.r+re,x.i+im};a[i+j+len/2]=(PrComplex){x.r-re,x.i-im};
 }if(inverse)for(int i=0;i<256;i++){a[i].r/=256;a[i].i/=256;}
}
static void pr_rooms_prepare(void){
 int expected=0;if(!__atomic_compare_exchange_n(&pr_rooms_ready,&expected,1,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)){while(__atomic_load_n(&pr_rooms_ready,__ATOMIC_ACQUIRE)!=2){}return;}
 for(int i=0;i<128;i++){pr_twR[i]=cosf(-6.28318530718f*i/256);pr_twI[i]=sinf(-6.28318530718f*i/256);}
 const int rates[3]={44100,48000,96000};
 for(int room=0;room<3;room++)for(int ri=0;ri<3;ri++){
  PrRoomPlan*p=&pr_rooms[room][ri];int n=(int)(PR_ROOM_IR_SAMPLES*rates[ri]/48000.f);p->count=(n+127)/128;if(p->count>64)p->count=64;
  for(int part=0;part<p->count;part++){
   for(int i=0;i<128;i++){float pos=(part*128+i)*48000.f/rates[ri];int j=(int)pos;float v=j+1<PR_ROOM_IR_SAMPLES?pr_room_ir[room][j]+(pr_room_ir[room][j+1]-pr_room_ir[room][j])*(pos-j):0;if(!part){p->headReverse[127-i]=v;p->head[i]=v;if(v!=0)pr_head_any=1;}else p->h[part][i].r=v;}
   if(part)pr_fft(p->h[part],0);
  }
 }__atomic_store_n(&pr_rooms_ready,2,__ATOMIC_RELEASE);
}
typedef struct {
 int enabled,room,amount,texture,duck,decay;int rate,ri,pos,hist,di[8],len[8],shiftPos;float active,env,duckGain;
 float dry[128],headDry[256],pending[128],overlap[128];
 int tailNext[3],tailJobsLast,tailJobsMax,boundaryJobsMax;PrComplex tailWork[3][256];PrComplex history[64][256],scratch[256];
 float fdn[8][8192],damp[8],feedback[8],lp,hpPole,shiftInputLP,shiftAlpha,shift[8192],phase,shiftLP,hpX,hpY;
 float amountNow,textureNow,duckNow,roomNow[3],roomTarget[3],decayNow;
} PrHybridSpace;
static void pr_space_init(PrHybridSpace*s,int rate){
 memset(s,0,sizeof(*s));s->rate=rate;s->ri=rate==44100?0:rate==96000?2:1;s->decay=35;s->duckGain=1;s->roomNow[0]=s->roomTarget[0]=1;
 const int lengths[8]={1423,1789,2017,2269,2551,2851,3181,3559};
 for(int i=0;i<8;i++){s->len[i]=(int)(lengths[i]*rate/48000.f);if(s->len[i]<1)s->len[i]=1;if(s->len[i]>8191)s->len[i]=8191;s->feedback[i]=.75f;}
 s->lp=1-expf(-6.28318530718f*5200/rate);s->hpPole=expf(-6.28318530718f*180/rate);s->shiftAlpha=1-expf(-6.28318530718f*3000/rate);
}
static void pr_space_config(PrHybridSpace*s,int enabled,int room,int amount,int texture,int duck,int decay);
static void pr_space_clear(PrHybridSpace*s){int enabled=s->enabled,room=s->room,amount=s->amount,texture=s->texture,duck=s->duck,decay=s->decay,rate=s->rate;pr_space_init(s,rate);s->enabled=enabled;s->room=room;s->amount=amount;s->texture=texture;s->duck=duck;s->decay=decay;s->roomNow[0]=s->roomTarget[0]=0;s->roomNow[room]=s->roomTarget[room]=1;pr_space_config(s,enabled,room,amount,texture,duck,decay);}
static void pr_space_config(PrHybridSpace*s,int enabled,int room,int amount,int texture,int duck,int decay){
 s->enabled=enabled!=0;s->room=room<0?0:room>2?2:room;s->amount=amount<0?0:amount>100?100:amount;s->texture=texture<0?0:texture>100?100:texture;s->duck=duck<0?0:duck>100?100:duck;s->decay=decay<0?0:decay>100?100:decay;
 for(int j=0;j<3;j++)s->roomTarget[j]=j==s->room;
 float seconds=.35f+s->decay*.055f;for(int i=0;i<8;i++)s->feedback[i]=powf(.001f,s->len[i]/(seconds*s->rate));
}
static float pr_space_read(PrHybridSpace*s,float delay){float p=s->shiftPos-delay;while(p<0)p+=8192;int k=(int)p;float f=p-k;return s->shift[k&8191]+(s->shift[(k+1)&8191]-s->shift[k&8191])*f;}
/* Partitions >=2 use completed history available at the START of this block.
   Spread them across its 128 frames. Only the current partition and FFT/IFFT
   remain at its boundary; IR offsets and direct audio latency stay unchanged.
   Each room is accumulated independently; its end-of-block weight is applied
   once, preserving the existing room transition instead of fixing old weights. */
static void pr_space_tail_until(PrHybridSpace*s,int room,int end){
 PrRoomPlan*p=&pr_rooms[room][s->ri];if(end>p->count)end=p->count;
 for(int part=s->tailNext[room];part<end;part++){pr_complex_add(s->tailWork[room],s->history[(s->hist-(part-2)+64)&63],p->h[part]);s->tailJobsLast++;}s->tailNext[room]=end;
}
static float pr_space_early(PrHybridSpace*s,float input){
 if(s->pos==0){memset(s->tailWork,0,sizeof(s->tailWork));for(int j=0;j<3;j++)s->tailNext[j]=2;}
 s->tailJobsLast=0;float early=s->pending[s->pos];s->dry[s->pos]=input;s->headDry[s->pos]=s->headDry[s->pos+128]=input;
 if(pr_head_any)for(int j=0;j<3;j++)if(s->roomNow[j]!=0)early+=pr_dot128(s->headDry+s->pos+1,pr_rooms[j][s->ri].headReverse)*s->roomNow[j];
 for(int j=0;j<3;j++)if(s->roomNow[j]>=.00001f||s->roomTarget[j]>0){int parts=pr_rooms[j][s->ri].count;pr_space_tail_until(s,j,2+(s->pos+1)*(parts>2?parts-2:0)/128);}
 if(s->tailJobsLast>s->tailJobsMax)s->tailJobsMax=s->tailJobsLast;
 if(++s->pos==128){
  /* An environment requested mid-block catches up once. Notes never trigger
     this path; all state remains private per channel and retained scene tail. */
  for(int j=0;j<3;j++)if(s->roomNow[j]>=.00001f)pr_space_tail_until(s,j,pr_rooms[j][s->ri].count);
  if(s->tailJobsLast>s->boundaryJobsMax)s->boundaryJobsMax=s->tailJobsLast;
  s->pos=0;s->hist=(s->hist+1)&63;PrComplex*in=s->history[s->hist];memset(in,0,256*sizeof(*in));for(int k=0;k<128;k++)in[k].r=s->dry[k];pr_fft(in,0);memset(s->scratch,0,sizeof(s->scratch));
  for(int j=0;j<3;j++){if(s->roomNow[j]<.00001f)continue;PrRoomPlan*p=&pr_rooms[j][s->ri];if(p->count<2)continue;PrComplex current[256]={{0}};pr_complex_add(current,in,p->h[1]);float a=s->roomNow[j];for(int k=0;k<256;k++){s->scratch[k].r+=a*(current[k].r+s->tailWork[j][k].r);s->scratch[k].i+=a*(current[k].i+s->tailWork[j][k].i);}}
  pr_fft(s->scratch,1);for(int k=0;k<128;k++){s->pending[k]=s->scratch[k].r+s->overlap[k];s->overlap[k]=s->scratch[k+128].r;}
 }
 return early;
}
static float pr_space_shimmer(PrHybridSpace*s,float input){
 int grain=(int)(s->rate*.04f);if(grain>7000)grain=7000;if(grain<64)grain=64;s->phase+=1.f/grain;if(s->phase>=1)s->phase-=1;float q=s->phase+.5f;if(q>=1)q-=1;
 float a=1-fabsf(2*s->phase-1),b=1-fabsf(2*q-1);float sh=a*pr_space_read(s,32+grain*(1-s->phase))+b*pr_space_read(s,32+grain*(1-q));
 s->shiftLP+=(sh-s->shiftLP)*s->lp;float hp=s->shiftLP-s->hpX+s->hpPole*s->hpY;s->hpX=s->shiftLP;s->hpY=hp;
 s->shiftInputLP+=(input-s->shiftInputLP)*s->shiftAlpha;s->shift[s->shiftPos]=s->shiftInputLP;s->shiftPos=(s->shiftPos+1)&8191;
 return hp;
}
static void pr_space_tick(PrHybridSpace*s,float input,float*l,float*r,float*blend){
 *l=*r=0;float smooth=1.f/(s->rate*.02f);s->active+=(s->enabled-s->active)*smooth;*blend=s->active;
 if(s->active<.00001f&&!s->enabled){if(s->active>0)pr_space_clear(s);s->active=0;return;}
 if(!isfinite(input))input=0;input=fmaxf(-4,fminf(4,input));
 s->amountNow+=(s->amount*.01f-s->amountNow)*smooth;s->textureNow+=(s->texture*.01f-s->textureNow)*smooth;s->duckNow+=(s->duck*.01f-s->duckNow)*smooth;
 for(int j=0;j<3;j++)s->roomNow[j]+=(s->roomTarget[j]-s->roomNow[j])*smooth;
 float early=pr_space_early(s,input);
 float value=fabsf(input),envSmooth=value>s->env?1.f/(s->rate*.006f):1.f/(s->rate*.4f);s->env+=(value-s->env)*envSmooth;float want=1.f/(1+s->env*s->duckNow*18);s->duckGain+=(want-s->duckGain)*smooth;
 float values[8],lateL=0,lateR=0;for(int i=0;i<8;i++){float v=s->fdn[i][s->di[i]];s->damp[i]+=(v-s->damp[i])*s->lp;values[i]=s->damp[i];lateL+=v*(i&1?-.19f:.19f);lateR+=v*(i&2?-.19f:.19f);}
 /* Two overlapping triangular grains: +12 semitones, no direct-signal pitch shift.
    Feedback injection is band limited and bounded independently of RT60. */
 float hp=pr_space_shimmer(s,(lateL+lateR)*.5f);
 for(int step=1;step<8;step*=2)for(int i=0;i<8;i+=step*2)for(int j=0;j<step;j++){float u=values[i+j],v=values[i+j+step];values[i+j]=u+v;values[i+j+step]=u-v;}
 for(int i=0;i<8;i++){float write=values[i]*.35355339f*s->feedback[i]+input*.13f+(early*.035f+hp*s->textureNow*.055f)*(i&1?-1:1);s->fdn[i][s->di[i]]=fabsf(write)<1e-20f?0:fmaxf(-8,fminf(8,write));if(++s->di[i]>=s->len[i])s->di[i]=0;}
 float amount=s->amountNow*s->duckGain;*l=(early*.65f+lateL+hp*s->textureNow*.45f)*amount;*r=(early*.65f+lateR+hp*s->textureNow*.45f)*amount;
}
#endif
