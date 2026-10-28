#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../native/engine.h"
static int count_index(tsf*f,unsigned index){int n=0;for(int i=0;i<f->voiceNum;i++)if(f->voices[i].playingPreset>=0&&f->voices[i].playIndex==index)n++;return n;}
int main(int argc,char**argv){assert(argc==2);tsf*bank=load_font(argv[1]);assert(bank);tsf*f=copy_font(bank,64);assert(f);tsf_channel_set_presetindex(f,0,0);tsf_channel_midi_control(f,0,64,127);short pcm[BLOCK*2];FX*fx=calloc(1,sizeof(FX));config_fx(fx,.25f,1,.8f,0,120,4,.6f);fx->protect=1;
 // A key still pressed must survive pressure from pedal-held released keys.
 smart_note(f,60,127);unsigned protected=f->voicePlayIndex-1;int regions=count_index(f,protected);assert(regions>0);
 for(int i=0;i<1200;i++){int key=30+(i%65);if(key==60)key=96;smart_note(f,key,80+i%40);unsigned newest=f->voicePlayIndex-1;assert(count_index(f,newest)>0);tsf_channel_note_off(f,0,key);render_audio(f,0,fx,.8f,0,pcm,128);assert(count_index(f,protected)==regions);}
 assert(smartSoft>0&&smartDropped==0);long soft=smartSoft;
 // Simulate a dense burst before rendering: overflow must not discard new notes.
 for(int i=0;i<600;i++){int key=24+i%84;smart_note(f,key,110);assert(count_index(f,f->voicePlayIndex-1)>0);tsf_channel_note_off(f,0,key);}assert(smartHard>0&&smartDropped==0);
 tsf_channel_midi_control(f,0,64,0);tsf_channel_note_off_all(f,0);for(int i=0;i<f->voiceNum;i++)assert(f->voices[i].playingPreset<0||f->voices[i].ampenv.segment>=TSF_SEGMENT_RELEASE);for(int i=0;i<1800;i++)render_audio(f,0,fx,.8f,0,pcm,256);assert(tsf_active_voice_count(f)==0);
 printf("PASS: 1200 pedal-held repeated notes, new notes always start, held key/stereo group protected; %ld soft tail releases; dense 600-note burst recycles without omitted notes; pedal-up releases all.\n",soft);tsf_close(f);tsf_close(bank);free(fx);
 // Bounded tap: ordered wrap, overflow is reported instead of blocking audio.
 rec_start(44100);for(int i=0;i<10000;i++){short input[64*2],output[64*2];for(int j=0;j<128;j++)input[j]=(short)(i+j);rec_push(input,64,44100);assert(rec_pull(output,64)==64);for(int j=0;j<128;j++)assert(input[j]==output[j]);}assert(recFirst>0&&recFailure==0);rec_stop();rec_start(44100);rec_push(pcm,64,48000);assert(recFailure==2&&!recActive);rec_start(44100);for(unsigned i=0;i<PR_REC_FRAMES/256+1;i++)rec_push(pcm,256,44100);assert(recFailure==1&&!recActive);puts("PASS: PCM tap wraps in order, bounded overflow and sample-rate changes stop capture safely.");return 0;
}
