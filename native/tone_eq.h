/* Original implementation of RBJ/W3C biquads: low shelf120Hz, presence2kHz,
   high shelf6kHz. Double state; coefficients computed outside audio callbacks. */
#ifndef PR_TONE_EQ
#define PR_TONE_EQ
extern double sin(double),cos(double);
typedef struct{double b0,b1,b2,a1,a2;} PrBiquad;
typedef struct{PrBiquad band[3];double x1[3][2],x2[3][2],y1[3][2],y2[3][2];int enabled;} PrEqBank;
typedef struct{PrEqBank current,previous,desired;float db[3];int rate,enabled,dirty,fade,total;} PrToneEq;
static float pr_db(float d){if(d!=d)return 0;return d< -6?-6:d>6?6:d;}
static PrBiquad pr_biquad(int band,float db,int rate){
 double freq=band==0?120:band==1?2000:6000;if(freq>rate*.4)freq=rate*.4;
 double w=6.283185307179586*freq/rate,c=cos(w),s=sin(w),A=pow(10.0,db/40.0),alpha=s*.7071067811865476;
 double b0,b1,b2,a0,a1,a2;
 if(band==1){alpha=s/(2*.7);b0=1+alpha*A;b1=-2*c;b2=1-alpha*A;a0=1+alpha/A;a1=-2*c;a2=1-alpha/A;}
 else{double root=2*sqrtf((float)A)*alpha;
  if(band==0){b0=A*((A+1)-(A-1)*c+root);b1=2*A*((A-1)-(A+1)*c);b2=A*((A+1)-(A-1)*c-root);a0=(A+1)+(A-1)*c+root;a1=-2*((A-1)+(A+1)*c);a2=(A+1)+(A-1)*c-root;}
  else{b0=A*((A+1)+(A-1)*c+root);b1=-2*A*((A-1)+(A+1)*c);b2=A*((A+1)+(A-1)*c-root);a0=(A+1)-(A-1)*c+root;a1=2*((A-1)-(A+1)*c);a2=(A+1)-(A-1)*c-root;}
 }
 PrBiquad q={b0/a0,b1/a0,b2/a0,a1/a0,a2/a0};return q;
}
static void pr_eq_request(PrToneEq*e,int enabled,float low,float mid,float high,int rate){
 e->enabled=enabled;e->db[0]=pr_db(low);e->db[1]=pr_db(mid);e->db[2]=pr_db(high);e->rate=rate;
 memset(&e->desired,0,sizeof(e->desired));e->desired.enabled=enabled;
 for(int i=0;i<3;i++)e->desired.band[i]=pr_biquad(i,e->db[i],rate);
 e->dirty=1;
}
static void pr_eq_reset(PrToneEq*e,int rate){
 pr_eq_request(e,e->enabled,e->db[0],e->db[1],e->db[2],rate);e->current=e->previous=e->desired;e->dirty=e->fade=0;
}
static double pr_eq_tick(PrEqBank*e,double x,int channel){
 if(!e->enabled)return x;
 for(int i=0;i<3;i++){PrBiquad*q=&e->band[i];double y=q->b0*x+q->b1*e->x1[i][channel]+q->b2*e->x2[i][channel]-q->a1*e->y1[i][channel]-q->a2*e->y2[i][channel];e->x2[i][channel]=e->x1[i][channel];e->x1[i][channel]=x;e->y2[i][channel]=e->y1[i][channel];e->y1[i][channel]=y;x=y;}
 return x;
}
static void pr_eq_process(PrToneEq*e,float*mix,int frames){
 if(e->dirty&&!e->fade){e->previous=e->current;e->current=e->desired;e->dirty=0;e->total=e->fade=e->rate/100;if(e->fade<1)e->total=e->fade=1;}
 if(!e->fade&&!e->current.enabled)return;
 for(int i=0;i<frames;i++){
  float t=e->fade?1-(e->fade-1)/(float)e->total:1;
  for(int c=0;c<2;c++){float x=mix[i*2+c];if(x!=x||x>=1e8f||x<=-1e8f)x=0;double y=pr_eq_tick(&e->current,x,c);if(e->fade)y=pr_eq_tick(&e->previous,x,c)*(1-t)+y*t;mix[i*2+c]=(float)y;}
  if(e->fade)e->fade--;
 }
}
#endif
