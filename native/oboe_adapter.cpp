#include "audio_api.h"
#include <oboe/Oboe.h>
#include <memory>
#include <new>
// Each stream owns its callbacks until close() has joined pending callbacks.
struct Stream final : oboe::AudioStreamDataCallback, oboe::AudioStreamErrorCallback {
 std::shared_ptr<oboe::AudioStream> audio;
 PrData data=nullptr;PrError error=nullptr;void* user=nullptr;void* errorUser=nullptr;
 oboe::DataCallbackResult onAudioReady(oboe::AudioStream*,void* out,int32_t frames) override {
  return data && data(reinterpret_cast<AAudioStream*>(this),user,out,frames) ? oboe::DataCallbackResult::Stop : oboe::DataCallbackResult::Continue;
 }
 bool onError(oboe::AudioStream*,oboe::Result result) override {
  if(error)error(reinterpret_cast<AAudioStream*>(this),errorUser,static_cast<int>(result));
  return true; // The control thread owns stopping/closing, as with legacy AAudio.
 }
};
struct Builder {oboe::AudioStreamBuilder audio;std::unique_ptr<Stream> callbacks{new Stream};};
static Builder* b(AAudioStreamBuilder* p){return reinterpret_cast<Builder*>(p);}
static Stream* s(AAudioStream* p){return reinterpret_cast<Stream*>(p);}
static int create(AAudioStreamBuilder** out){*out=nullptr;try{auto p=new Builder;*out=reinterpret_cast<AAudioStreamBuilder*>(p);return 0;}catch(...){return -1;}}
static int destroy(AAudioStreamBuilder* p){delete b(p);return 0;}
#define SET(name,method,type) static void name(AAudioStreamBuilder*p,int v){b(p)->audio.method(static_cast<type>(v));}
SET(device,setDeviceId,int32_t) SET(direction,setDirection,oboe::Direction)
SET(sharing,setSharingMode,oboe::SharingMode) SET(performance,setPerformanceMode,oboe::PerformanceMode)
SET(channels,setChannelCount,int32_t) SET(format,setFormat,oboe::AudioFormat)
SET(sampleRate,setSampleRate,int32_t) SET(inputPreset,setInputPreset,oboe::InputPreset) SET(usage,setUsage,oboe::Usage)
static void data(AAudioStreamBuilder*p,PrData fn,void*u){b(p)->callbacks->data=fn;b(p)->callbacks->user=u;b(p)->audio.setDataCallback(b(p)->callbacks.get());}
static void error(AAudioStreamBuilder*p,PrError fn,void*u){b(p)->callbacks->error=fn;b(p)->callbacks->errorUser=u;b(p)->audio.setErrorCallback(b(p)->callbacks.get());}
static int open(AAudioStreamBuilder*p,AAudioStream** out){*out=nullptr;auto result=b(p)->audio.openStream(b(p)->callbacks->audio);if(result!=oboe::Result::OK)return static_cast<int>(result);*out=reinterpret_cast<AAudioStream*>(b(p)->callbacks.release());return 0;}
static int close(AAudioStream*p){int result=static_cast<int>(s(p)->audio->close());delete s(p);return result;}
static int start(AAudioStream*p){return static_cast<int>(s(p)->audio->requestStart());}
static int stop(AAudioStream*p){return static_cast<int>(s(p)->audio->requestStop());}
#define GET(name,method) static int name(AAudioStream*p){return static_cast<int>(s(p)->audio->method());}
GET(rate,getSampleRate) GET(size,getBufferSizeInFrames) GET(capacity,getBufferCapacityInFrames)
GET(burst,getFramesPerBurst) GET(mode,getSharingMode) GET(perf,getPerformanceMode)
GET(id,getDeviceId) GET(ch,getChannelCount) GET(fmt,getFormat)
static int xruns(AAudioStream*p){auto r=s(p)->audio->getXRunCount();return r?r.value():static_cast<int>(r.error());}
static int buffer(AAudioStream*p,int frames){auto r=s(p)->audio->setBufferSizeInFrames(frames);return r?r.value():static_cast<int>(r.error());}
static int readInput(AAudioStream*p,void*out,int frames,long long timeout){auto r=s(p)->audio->read(out,frames,timeout);return r?r.value():static_cast<int>(r.error());}
extern "C" int pr_oboe_api(PrApi* api){*api={create,destroy,device,direction,sharing,performance,channels,format,sampleRate,readInput,inputPreset,usage,data,error,open,close,start,stop,rate,size,capacity,burst,mode,perf,xruns,id,ch,fmt,buffer};return 1;}
