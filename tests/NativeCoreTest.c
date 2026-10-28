#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../native/engine.h"
#include "../native/aaudio_backend.h"
int main(int argc,char**argv){
 assert(argc==2);tsf*bank=load_font(argv[1]);assert(bank);tsf*a=copy_font(bank,128),*b=copy_font(bank,128);assert(a&&b);
 tsf_channel_set_presetindex(a,0,4);tsf_channel_set_presetindex(b,0,89);FX*f=calloc(1,sizeof(FX));config_fx(f,.2f,1,2,0,120,4,.6f);f->protect=1;
 assert(aa_start(a,b,f,2,.4f,-1,12)==0);
 pr_lock();assert(tsf_channel_get_preset_index(a,0)==4&&tsf_channel_get_preset_index(b,0)==89);assert(fx_rate(f)==48000);tsf_channel_note_on(a,0,60,1);tsf_channel_note_on(b,0,60,.6f);pr_unlock();usleep(120000);
 assert(aa_buffer(8)==384);assert(aa_buffer(4)==192&&aa_buffer(2)==192);aa_stop();assert(!nativeStream.stream);
 assert(tsf_channel_get_preset_index(a,0)==4&&tsf_channel_get_preset_index(b,0)==89);
 pr_rates(a,b,f,44100);assert(fx_rate(f)==44100);tsf_close(a);tsf_close(b);tsf_close(bank);free(f);
 puts("PASS: AAudio start preserves selected electric piano/pad presets, negotiated 48kHz, buffer at burst boundaries, stop and 44.1kHz restoration; host ABI double.");return 0;
}
