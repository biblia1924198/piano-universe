#define main previous_realism_main
#include "PianoRealismTest.c"
#undef main
int main(){tsf*f=synthetic();for(unsigned i=0;i<f->sampleCount;i++){float t=i/48000.f;f->fontSamples[i]=expf(-t/2)*(.25f*sinf(6.28318530718f*440*t)+.08f*sinf(6.28318530718f*884*t)+.035f*sinf(6.28318530718f*1328*t));}
 float p[792];pr_profile_estimate_extended(f,0,p);int n=69-21,k=n*3;printf("estimated %.2f / %.2f / %.2f Hz; amplitudes %.2f / %.2f\n",p[n],p[264+k+1],p[264+k+2],p[528+k+1],p[528+k+2]);assert(p[n+176]>=.5&&fabsf(p[264+k+1]-884)<3&&fabsf(p[264+k+2]-1328)<3);
 PrPianoResonance a,b;pr_res_init(&a,48000);pr_res_profile(&a,p);b=a;pr_res_harmonics(&b,p,792);assert(b.co[k+1]!=a.co[k+1]&&b.weight[k+1]>0);p[264+k+1]=NAN;b=a;pr_res_harmonics(&b,p,792);assert(b.co[k+1]==a.co[k+1]);b=a;pr_res_harmonics(&b,p,264);assert(memcmp(&a,&b,sizeof(a))==0);
 PrChannelFx*fx=part_new(48000);f->prRepeat=75;smart_note(f,69,90);advance(f,64);unsigned held=f->voices[0].playIndex;smart_note(f,69,90);for(int i=0;i<f->voiceNum;i++)if(f->voices[i].playingPreset>=0&&f->voices[i].playIndex==held)assert(f->voices[i].prRepeatTarget==1);
 part_pedal(f,fx,127);part_release(f,fx,69);smart_note(f,69,90);int softened=0;for(int i=0;i<f->voiceNum;i++){struct tsf_voice*v=&f->voices[i];if(v->playingPreset>=0&&v->playIndex==held){assert(v->prRepeatTarget<1&&v->prRepeatGain==1);softened++;}else if(v->playingPreset>=0&&!v->prKeyReleased)assert(v->prRepeatTarget==1);}assert(softened>0);advance(f,4800);for(int i=0;i<f->voiceNum;i++){struct tsf_voice*v=&f->voices[i];if(v->playingPreset>=0&&v->playIndex==held)assert(v->prRepeatGain<.8f&&v->prRepeatGain>=.12f);}
 free(fx);tsf_close(f);puts("PASS: measured inharmonic partial estimates, corrupt/legacy fallbacks, repeat smoothing protects held keys and preserves sustain ownership.");
}
