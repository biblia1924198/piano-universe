/* Atomic stereo PCM words: snapshots never stop the real-time producer. */
#ifndef PR_RETRO_TAP
#define PR_RETRO_TAP
#define PR_RETRO_CAP (1u<<23)
static unsigned*retroPcm;static unsigned long long retroWrite;static int retroEnabled,retroRate,retroEpoch;
static int retro_enable(int on){if(on&&!retroPcm){retroPcm=calloc(PR_RETRO_CAP,sizeof(unsigned));if(!retroPcm)return 0;}__atomic_store_n(&retroEnabled,on,__ATOMIC_RELEASE);if(!on){__atomic_add_fetch(&retroEpoch,1,__ATOMIC_RELEASE);__atomic_store_n(&retroWrite,0,__ATOMIC_RELEASE);}return 1;}
static void retro_push(const short*pcm,int n,int rate){if(!__atomic_load_n(&retroEnabled,__ATOMIC_ACQUIRE)||!retroPcm)return;
 if(__atomic_load_n(&retroRate,__ATOMIC_RELAXED)!=rate){__atomic_add_fetch(&retroEpoch,1,__ATOMIC_RELEASE);__atomic_store_n(&retroWrite,0,__ATOMIC_RELEASE);__atomic_store_n(&retroRate,rate,__ATOMIC_RELEASE);}
 unsigned long long w=__atomic_load_n(&retroWrite,__ATOMIC_RELAXED);for(int i=0;i<n;i++){unsigned packed=(unsigned short)pcm[i*2]|((unsigned)(unsigned short)pcm[i*2+1]<<16);__atomic_store_n(&retroPcm[(w+i)&(PR_RETRO_CAP-1)],packed,__ATOMIC_RELAXED);}__atomic_store_n(&retroWrite,w+n,__ATOMIC_RELEASE);
}
static int retro_read(unsigned long long begin,int epoch,short*out,int n){if(!retroPcm||epoch!=__atomic_load_n(&retroEpoch,__ATOMIC_ACQUIRE))return -1;unsigned long long w=__atomic_load_n(&retroWrite,__ATOMIC_ACQUIRE);if(begin>w||n>(int)(w-begin)||w-begin>=PR_RETRO_CAP)return -1;
 for(int i=0;i<n;i++){unsigned v=__atomic_load_n(&retroPcm[(begin+i)&(PR_RETRO_CAP-1)],__ATOMIC_RELAXED);out[i*2]=(short)v;out[i*2+1]=(short)(v>>16);}w=__atomic_load_n(&retroWrite,__ATOMIC_ACQUIRE);return epoch==__atomic_load_n(&retroEpoch,__ATOMIC_ACQUIRE)&&w-begin<PR_RETRO_CAP?n:-1;
}
#endif
