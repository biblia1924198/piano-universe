#ifndef PR_SCENE_ENGINE
#define PR_SCENE_ENGINE
static int scene_init(FX*f){f->tailScale=1;f->masterAnchor=f->master;for(int i=0;i<4;i++){f->parts[i]=part_new(fx_rate(f));if(!f->parts[i])return 0;f->parts[i]->owner=++f->serial;}return 1;}
static int scene_slots(FX*f,int count){int n=0;for(int i=0;i<PR_TAILS;i++)n+=!f->tails[i].bank;return n>=count;}
static void scene_retire(FX*f,tsf*b,PrChannelFx*p,int channel,float gain){
 if(!b){free(p);return;}for(int i=0;i<PR_TAILS;i++)if(!f->tails[i].bank){PrTail*t=&f->tails[i];t->bank=b;t->part=p;t->channel=channel;t->gain=gain;t->master=f->master;t->wet=f->wet;t->feedback=f->feedback;t->transpose=p->transpose;t->serial=++f->serial;t->done=!tsf_active_voice_count(b)&&!p->ever;break;}
 int live=0;PrTail*old=0;for(int i=0;i<PR_TAILS;i++){PrTail*t=&f->tails[i];if(t->bank&&!t->done){live++;if(!t->part->fade&&(!old||t->serial<old->serial))old=t;}}
 if(live>12&&old&&!old->part->fade){old->part->fade=fx_rate(f)/10;f->tailFades++;}
}
static void scene_all_silent(FX*f){part_reset(f->yamaha);memset(f->yamahaInput,0,sizeof(f->yamahaInput));memset(f->keys,0,sizeof(f->keys));memset(f->keyHead,0,sizeof(f->keyHead));memset(f->keyCount,0,sizeof(f->keyCount));for(int i=0;i<4;i++){part_reset(f->parts[i]);if(f->currentBanks[i])bank_clear_context(f->currentBanks[i]);}for(int i=0;i<PR_TAILS;i++)if(f->tails[i].bank){tsf*b=f->tails[i].bank;for(int j=0;j<b->voiceNum;j++)tsf_voice_kill(&b->voices[j]);bank_clear_context(b);part_reset(f->tails[i].part);f->tails[i].done=1;}}
static void yamaha_event(FX*f,PrMidiEvent ev){
 PrChannelFx*p=f->yamaha;if(!p||!f->yamahaEnabled)return;
 if(ev.type==0x90&&ev.b&&ev.a>=0&&ev.a<128){if(p->held[ev.a]<65535)p->held[ev.a]++;f->yamahaVelocity[ev.a]=ev.b;f->yamahaOn[ev.a]=f->yamahaFrame;pr_mech_event(&p->mechanics,0,ev.a,ev.b,0,p->rate);}
 else if((ev.type==0x80||ev.type==0x90)&&ev.a>=0&&ev.a<128){if(p->held[ev.a]){p->held[ev.a]--;pr_mech_event(&p->mechanics,1,ev.a,f->yamahaVelocity[ev.a],(f->yamahaFrame-f->yamahaOn[ev.a])/(float)p->rate,p->rate);}}
 else if(ev.type==0xB0&&ev.a==64){int before=p->pedal;p->pedal=ev.b;if((before>=64)!=(ev.b>=64))pr_mech_event(&p->mechanics,2,64,(ev.b-before),0,p->rate);}
 else if(ev.type==0xB0&&(ev.a==120||ev.a==121||ev.a==123)){part_reset(p);memset(f->yamahaVelocity,0,sizeof(f->yamahaVelocity));}
}
static void scene_event(FX*f,tsf**banks,PrMidiEvent ev){yamaha_event(f,ev);
 if(!f->parts[0]){apply_event(banks,ev);return;}
 if(ev.type==0x90&&ev.b){if(ev.a<0||ev.a>127)return;if(f->keyCount[ev.a]==32)scene_event(f,banks,(PrMidiEvent){0x80,ev.a,0,15});PrHeld*h=&f->keys[ev.a][(f->keyHead[ev.a]+f->keyCount[ev.a])&31];memset(h,0,sizeof(*h));for(int c=0;c<4;c++){tsf*b=banks[c];PrChannelFx*p=f->parts[c];if(b&&p&&(ev.mask&(1<<c))){int key=ev.a+p->transpose;if(key>=0&&key<128){smart_note(b,key,ev.b);pr_mech_event(&p->mechanics,0,key,ev.b,0,p->rate);h->owner[c]=p->owner;if(p->held[ev.a]<65535)p->held[ev.a]++;}}}f->keyCount[ev.a]++;return;}
 if(ev.type==0x80||ev.type==0x90){if(ev.a<0||ev.a>127||!f->keyCount[ev.a])return;PrHeld*h=&f->keys[ev.a][f->keyHead[ev.a]];for(int c=0;c<4;c++)if(h->owner[c]){PrChannelFx*p=f->parts[c];if(p&&p->owner==h->owner[c]&&banks[c]){if(p->held[ev.a])p->held[ev.a]--;part_release(banks[c],p,ev.a+p->transpose);}else for(int i=0;i<PR_TAILS;i++){PrTail*t=&f->tails[i];if(t->bank&&!t->done&&t->channel==c&&t->part->owner==h->owner[c]){if(t->part->held[ev.a])t->part->held[ev.a]--;part_release(t->bank,t->part,ev.a+t->transpose);break;}}}f->keyHead[ev.a]=(f->keyHead[ev.a]+1)&31;f->keyCount[ev.a]--;return;}
 if(ev.type==0xB0&&(ev.a==120||ev.a==123)){memset(f->keyCount,0,sizeof(f->keyCount));memset(f->keyHead,0,sizeof(f->keyHead));}
 for(int c=0;c<4;c++){tsf*b=banks[c];PrChannelFx*p=f->parts[c];if(!p)continue;{

   if(b){if(ev.type==0xB0&&ev.a==64)part_pedal(b,p,ev.b);else if(ev.type==0xB0&&(ev.a==120||ev.a==121||ev.a==123)){tsf_channel_midi_control(b,0,ev.a,ev.b);if(ev.a!=121)bank_clear_context(b);if(ev.a!=121)memset(p->held,0,sizeof(p->held));if(ev.a==121)p->pedal=0;}else if(ev.type==0xB0&&b->prBank&&(ev.a==7||ev.a==10||ev.a==11||ev.a==39||ev.a==42||ev.a==43||(ev.a>=0&&ev.a<128&&(b->prVelCCMask[ev.a]||b->prCCMask[ev.a]))))tsf_channel_midi_control(b,0,ev.a,ev.b);else if(ev.type==0xE0)tsf_channel_set_pitchwheel(b,0,ev.a|(ev.b<<7));}
   for(int i=0;i<PR_TAILS;i++){PrTail*t=&f->tails[i];if(!t->bank||t->done||t->channel!=c)continue;if(ev.type==0xB0&&ev.a==64)part_pedal(t->bank,t->part,ev.b);else if(ev.type==0xB0&&(ev.a==120||ev.a==121||ev.a==123)){tsf_channel_midi_control(t->bank,0,ev.a,ev.b);if(ev.a!=121)bank_clear_context(t->bank);if(ev.a!=121)memset(t->part->held,0,sizeof(t->part->held));if(ev.a==121)t->part->pedal=0;}else if(ev.type==0xB0&&t->bank->prBank&&(ev.a==7||ev.a==10||ev.a==11||ev.a==39||ev.a==42||ev.a==43||(ev.a>=0&&ev.a<128&&(t->bank->prVelCCMask[ev.a]||t->bank->prCCMask[ev.a]))))tsf_channel_midi_control(t->bank,0,ev.a,ev.b);else if(ev.type==0xE0)tsf_channel_set_pitchwheel(t->bank,0,ev.a|(ev.b<<7));}
  }
 }
}
static void scene_sample_job(void* bank,float* out,int frames){tsf_render_float((tsf*)bank,out,frames,0);}
static void scene_render(FX*f,tsf**banks,const float*gains,float*mix,int frames){
 float b[BLOCK*2],parallel[4][BLOCK*2];int ready[4]={0},order[4],count=0,total=0;
 for(int c=0;c<4;c++)if(banks[c]){int voices=tsf_active_voice_count(banks[c]);total+=voices;if(voices>0)order[count++]=c;}
 // Do not dispatch two jobs for an aliased instance, including synthetic tests.
 int distinct=1;for(int i=0;i<count;i++)for(int j=0;j<i;j++)if(banks[order[i]]==banks[order[j]])distinct=0;
 if(count>=2&&total>=192&&distinct){
  for(int i=0;i<count;i++)for(int j=i+1;j<count;j++)if(tsf_active_voice_count(banks[order[j]])>tsf_active_voice_count(banks[order[i]])){int t=order[i];order[i]=order[j];order[j]=t;}
  void* jobs[4];float* outs[4];for(int i=0;i<count;i++){jobs[i]=banks[order[i]];outs[i]=parallel[order[i]];ready[order[i]]=1;}
  long long stamp=f->profileAudio?pr_boottime():0;pr_parallel_render(scene_sample_job,jobs,outs,count,frames);if(f->profileAudio)f->sampleNanos+=pr_boottime()-stamp;
 }
 for(int c=0;c<4;c++)if(banks[c]){PrChannelFx*p=f->parts[c];if(gains[c]==0&&p->gain<=.000001f&&p->energy==0&&!tsf_active_voice_count(banks[c])){banks[c]->prSeconds+=frames/banks[c]->outSampleRate;continue;}part_rate(p,fx_rate(f));long long stamp=f->profileAudio?pr_boottime():0;if(ready[c])memcpy(b,parallel[c],frames*2*sizeof(float));else tsf_render_float(banks[c],b,frames,0);if(f->profileAudio){long long now=pr_boottime();f->sampleNanos+=now-stamp;stamp=now;}part_process(p,b,mix,frames,gains[c],f->master,f->wet,f->feedback);if(f->profileAudio)f->effectNanos+=pr_boottime()-stamp;if(p->energy>f->layerPeak[c])f->layerPeak[c]=p->energy;}
 for(int i=0;i<PR_TAILS;i++){PrTail*t=&f->tails[i];if(!t->bank||t->done)continue;PrChannelFx*p=t->part;part_rate(p,fx_rate(f));tsf_set_output(t->bank,TSF_STEREO_INTERLEAVED,fx_rate(f),-10);int fading=p->fade;long long stamp=f->profileAudio?pr_boottime():0;tsf_render_float(t->bank,b,frames,0);if(f->profileAudio){long long now=pr_boottime();f->sampleNanos+=now-stamp;stamp=now;}part_process(p,b,mix,frames,t->gain,t->master*f->tailScale,t->wet,t->feedback);if(f->profileAudio)f->effectNanos+=pr_boottime()-stamp;
  if(fading&&!p->fade){t->done=1;continue;}if(!tsf_active_voice_count(t->bank))p->idle+=frames;else p->idle=0;
  if(p->idle>(unsigned)fx_rate(f)*8&&!p->fade)p->fade=fx_rate(f)/10;else if(p->idle>(unsigned)fx_rate(f)*2&&p->energy<.0000005f&&p->delay==0)t->done=1;
 }
}
#endif
