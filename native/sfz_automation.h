/* Immutable PRB7 controller tables. Audio path has no allocation or parsing. */
static float pr7_curve(struct pr_sfz_mod*m,struct tsf_channel*c){return m->curve[c->prCC[m->cc]];}
static float pr7_sum(struct tsf_region*r,struct tsf_channel*c,int kind){float sum=0;struct pr_sfz_ext*x=r->prExt;for(int i=0;i<x->mods;i++)if(x->mod[i].kind==kind)sum+=x->mod[i].amount*pr7_curve(&x->mod[i],c);return sum;}
static float pr7_gain(struct tsf_region*r,struct tsf_channel*c){float gain=1;for(int i=0;i<r->prExt->mods;i++){struct pr_sfz_mod*m=&r->prExt->mod[i];if(m->kind==1)gain*=m->amount*.01f*pr7_curve(m,c);}return gain*tsf_decibelsToGain(fmaxf(-144,fminf(144,pr7_sum(r,c,2))));}
static int pr7_kind(struct tsf_region*r,int kind){if(!r->prExt)return 0;for(int i=0;i<r->prExt->mods;i++)if(r->prExt->mod[i].kind==kind)return 1;return 0;}
static int pr7_matches(tsf*f,struct tsf_region*r,int triggerCC){
 if(!r->prExt)return 1;struct pr_sfz_ext*x=r->prExt;int ch=f->channels?f->channels->activeChannel:0;struct tsf_channel*c=f->channels?&f->channels->channels[ch]:0;int triggered=0,hasTrigger=0;
 for(int i=0;i<x->conditions;i++){struct pr_sfz_condition*q=&x->condition[i];int value=c?c->prCC[q->cc]:f->prDefaultCC[q->cc]?f->prDefaultCC[q->cc]-1:0;if(q->trigger){hasTrigger=1;if(triggerCC==q->cc&&value>=q->lo&&value<=q->hi)triggered=1;}else if(value<q->lo||value>q->hi)return 0;}
 return triggerCC<0?!hasTrigger:triggered;
}
static void pr7_pan(struct tsf_region*r,struct tsf_channel*c,struct tsf_voice*v){
 float pan=fmaxf(-.5f,fminf(.5f,r->pan+(pr7_kind(r,3)?pr7_sum(r,c,3)*.005f:c->panOffset)));
 if(r->prExt&&r->prExt->side>=0){float balance=fmaxf(-1,fminf(1,pr7_kind(r,3)?pr7_sum(r,c,3)*.01f:c->panOffset*2));v->panFactorLeft=r->prExt->side==0?(balance>0?1-balance:1):0;v->panFactorRight=r->prExt->side==1?(balance<0?1+balance:1):0;}
 else{v->panFactorLeft=TSF_SQRTF(.5f-pan);v->panFactorRight=TSF_SQRTF(.5f+pan);}
}
static int pr7_sustain(tsf*f,struct tsf_region*r,int channel,int fallback){if(r->prSfz5&&(r->prSfzFlags&1))return 0;if(!r->prExt)return fallback;return f->channels->channels[channel].prCC[r->prExt->sustain]>=64;}
