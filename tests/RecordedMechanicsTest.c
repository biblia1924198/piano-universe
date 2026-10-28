#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../native/piano_mechanics.h"

static int active(const PrMechanics*m){int n=0;for(int i=0;i<PR_MECH_VOICES;i++)n+=m->recorded[i].sample!=0;return n;}
static void invalid_pack(const unsigned char*source,size_t size,size_t offset,const void*value,size_t bytes,int truncate){
 char path[]="/tmp/pr-recorded-invalid-XXXXXX";int fd=mkstemp(path);assert(fd>=0);unsigned char*copy=malloc(size);assert(copy);memcpy(copy,source,size);memcpy(copy+offset,value,bytes);assert(write(fd,copy,truncate?size-8:size)==(ssize_t)(truncate?size-8:size));close(fd);assert(!pr_mech_load(path));unlink(path);free(copy);
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);size_t size=ftell(f);rewind(f);unsigned char*source=malloc(size);assert(source&&fread(source,1,size,f)==size);fclose(f);
 unsigned wrong=93,badOffset=1;float nan=NAN;invalid_pack(source,size,12,&wrong,4,0);invalid_pack(source,size,20,&badOffset,4,0);invalid_pack(source,size,20+92*8,&nan,4,0);invalid_pack(source,size,0,source,4,1);free(source);assert(!pr_recorded_bank());
 assert(pr_mech_load(argv[1]));PrMechanicalBank*bank=pr_recorded_bank();assert(bank&&bank->rate==48000);assert(pr_mech_load(argv[1])&&pr_recorded_bank()==bank);
 for(int i=0;i<92;i++){double energy=0;const PrMechanicalSample*s=&bank->sample[i];for(unsigned n=0;n<s->frames*2;n++)energy+=s->data[n]*s->data[n];assert(energy>0&&s->frames<=48000);}
 // At its original rate a mechanical capture must reproduce the actual stereo samples.
 PrMechanics m={0};m.hammer=70;m.key=20;m.pedal=80;pr_mech_event(&m,0,60,127,0,48000);assert(!active(&m));pr_mech_event(&m,1,60,127,0,48000);assert(active(&m)==1);
 const PrMechanicalSample*s=&bank->sample[39];float gain=.7f*.045f;
 for(unsigned n=0;n<s->frames-1;n++){float l,r;pr_mech_tick(&m,&l,&r);assert(fabsf(l-s->data[n*2]*gain)<1e-7f&&fabsf(r-s->data[n*2+1]*gain)<1e-7f);}float l,r;pr_mech_tick(&m,&l,&r);assert(!m.recordedActive);
 // The two legacy action controls select one joint key/hammer-return recording.
 m.key=70;pr_mech_event(&m,1,60,127,0,48000);assert(active(&m)==1&&fabsf(m.recorded[0].gain-gain)<1e-7f);pr_mech_clear(&m);
 pr_mech_event(&m,2,64,127,0,48000);assert(m.recorded[0].sample==&bank->sample[88]);pr_mech_clear(&m);pr_mech_event(&m,2,64,127,0,48000);assert(m.recorded[0].sample==&bank->sample[89]);pr_mech_clear(&m);
 pr_mech_event(&m,2,64,-127,0,48000);assert(m.recorded[0].sample==&bank->sample[90]);pr_mech_clear(&m);pr_mech_event(&m,2,64,-127,0,48000);assert(m.recorded[0].sample==&bank->sample[91]);pr_mech_clear(&m);
 for(int rate=22050;rate<=96000;rate=rate==22050?44100:rate==44100?48000:rate==48000?96000:96001){
  pr_mech_event(&m,1,21,127,0,rate);assert(m.recorded[0].sample==&bank->sample[0]);pr_mech_clear(&m);pr_mech_event(&m,1,108,127,0,rate);assert(m.recorded[0].sample==&bank->sample[87]);pr_mech_clear(&m);
  for(int block=0;block<500;block++){for(int n=0;n<9;n++)pr_mech_event(&m,1,21+(block*9+n)%88,20+(block+n)%108,0,rate);if(block%3==0)pr_mech_event(&m,2,64,block%2?-127:127,0,rate);assert(active(&m)<=32);for(int n=0;n<64;n++){pr_mech_tick(&m,&l,&r);assert(isfinite(l)&&isfinite(r)&&fabsf(l)<2&&fabsf(r)<2);}}
  for(int n=0;n<rate*2;n++)pr_mech_tick(&m,&l,&r);assert(!m.recordedActive);pr_mech_clear(&m);
 }
 m.hammer=m.key=m.pedal=0;pr_mech_event(&m,1,60,127,0,48000);pr_mech_event(&m,2,64,127,0,48000);assert(!active(&m));pr_mech_tick(&m,&l,&r);assert(l==0&&r==0);
 __atomic_store_n(&pr_mechanical_bank,0,__ATOMIC_RELEASE);free(bank->data);free(bank);
 puts("PASS: recorded pack corruption/NaN/truncation rejected; all 92 real stereo captures; one joint return, no artificial attack; distinct pedal directions/takes; bounded stealing, finite resampling at four rates, disable and complete drain.");
}
