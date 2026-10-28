#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../native/engine.h"
static int sequence(tsf*base,float*reference,int compare){
 FX*f=calloc(1,sizeof(*f));config_fx(f,.2f,1,.6f,0,120,4,.6f);assert(scene_init(f));f->protect=1;
 tsf*banks[4];float gains[4]={.4f,.3f,.2f,.1f},out[BLOCK*2];int offset=0;
 for(int c=0;c<4;c++){banks[c]=copy_font(base,512);assert(banks[c]);banks[c]->sampleQuality=1;banks[c]->prRepeat=1;tsf_channel_set_presetindex(banks[c],0,c==0?0:c==1?4:16);f->parts[c]->chorus=.3f;f->parts[c]->delay=.15f;f->parts[c]->resonance=.1f;}
 scene_event(f,banks,(PrMidiEvent){0xB0,64,127,15});
 for(int block=0;block<900;block++){
  int frames=(int[]){96,192,256,73}[block%4];
  if(block<100){for(int k=0;k<4;k++){int note=36+(block*4+k)%48;scene_event(f,banks,(PrMidiEvent){0x90,note,60+(block*7)%67,15});scene_event(f,banks,(PrMidiEvent){0x80,note,0,15});}}
  if(block==220)scene_event(f,banks,(PrMidiEvent){0xB0,64,60,15});
  if(block==300)scene_event(f,banks,(PrMidiEvent){0xB0,64,0,15});
  if(block==150){scene_retire(f,banks[0],f->parts[0],0,gains[0]);banks[0]=copy_font(base,512);assert(banks[0]);f->parts[0]=part_new(44100);assert(f->parts[0]);scene_event(f,banks,(PrMidiEvent){0x90,64,100,1});}
  render_mixer_float(banks,f,gains,out,frames);
  for(int j=0;j<frames*2;j++){assert(isfinite(out[j]));if(compare){if(out[j]!=reference[offset+j]){fprintf(stderr,"mismatch block=%d sample=%d %.9g %.9g\n",block,j,out[j],reference[offset+j]);return 1;}}else reference[offset+j]=out[j];}offset+=frames*2;
 }
 for(int c=0;c<4;c++){tsf_close(banks[c]);free(f->parts[c]);}for(int i=0;i<PR_TAILS;i++){if(f->tails[i].bank)tsf_close(f->tails[i].bank);free(f->tails[i].part);}free(f);return 0;
}
int main(int argc,char**argv){assert(argc==2);tsf*base=load_font(argv[1]);assert(base);float*reference=calloc(900*BLOCK*2,sizeof(float));assert(reference);pr_parallel_start(0);assert(!sequence(base,reference,0));pr_parallel_start(1);assert(!sequence(base,reference,1));long long stats[10];pr_parallel_stats(stats);assert(stats[0]==2&&stats[1]>0&&stats[4]+stats[7]>0);printf("PASS: serial/parallel exact waveform, 4 channels, HQ, sustain/release, scene tails, effects, variable frames; batches=%lld worker jobs=%lld/%lld\n",stats[1],stats[4],stats[7]);pr_parallel_stop();pr_parallel_stats(stats);assert(stats[0]==0);for(int i=0;i<50;i++){pr_parallel_start(1);pr_parallel_stop();}tsf_close(base);free(reference);puts("PASS: 50 worker lifecycle cycles");return 0;}
