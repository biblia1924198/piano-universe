#pragma once
#include <memory>
#include <cstdint>
namespace oboe {
enum class Result{OK=0,Error=-99};enum class DataCallbackResult{Continue=0,Stop=1};enum class Direction{Output=0,Input=1};enum class SharingMode{Exclusive=0,Shared=1};enum class PerformanceMode{None=10,LowLatency=12};enum class AudioFormat{Float=2};enum class InputPreset{Unprocessed=9};enum class Usage{Game=14};
class AudioStream;
class AudioStreamDataCallback{public:virtual ~AudioStreamDataCallback()=default;virtual DataCallbackResult onAudioReady(AudioStream*,void*,int32_t)=0;};
class AudioStreamErrorCallback{public:virtual ~AudioStreamErrorCallback()=default;virtual bool onError(AudioStream*,Result)=0;};
struct Value {int v;explicit operator bool()const{return v>=0;}int value()const{return v;}Result error()const{return static_cast<Result>(v);}};
inline int failures=0,closed=0;inline std::weak_ptr<AudioStream> latest;
class AudioStream{public:AudioStreamDataCallback*data=nullptr;AudioStreamErrorCallback*error=nullptr;int device=0,channels=2,rate=48000,sharing=0,performance=12,format=2,buffer=384,readResult=64;
 Result close(){closed++;return Result::OK;}Result requestStart(){return Result::OK;}Result requestStop(){return Result::OK;}
#define GET(method,field) int method(){return field;}
 GET(getSampleRate,rate) GET(getDeviceId,device) GET(getChannelCount,channels) GET(getSharingMode,sharing) GET(getPerformanceMode,performance) GET(getFormat,format) GET(getBufferSizeInFrames,buffer)
 int getBufferCapacityInFrames(){return 2048;}int getFramesPerBurst(){return 96;}Value getXRunCount(){return {7};}Value setBufferSizeInFrames(int v){buffer=v;return {v};}Value read(void*,int,int64_t){return {readResult};}
};
class AudioStreamBuilder{AudioStream config;public:
#define SET(method,field,type) AudioStreamBuilder*method(type v){config.field=static_cast<int>(v);return this;}
 SET(setDeviceId,device,int32_t) SET(setChannelCount,channels,int32_t) SET(setSampleRate,rate,int32_t) SET(setSharingMode,sharing,SharingMode) SET(setPerformanceMode,performance,PerformanceMode) SET(setFormat,format,AudioFormat)
 AudioStreamBuilder*setDirection(Direction){return this;}AudioStreamBuilder*setInputPreset(InputPreset){return this;}AudioStreamBuilder*setUsage(Usage){return this;}
 AudioStreamBuilder*setDataCallback(AudioStreamDataCallback*p){config.data=p;return this;}AudioStreamBuilder*setErrorCallback(AudioStreamErrorCallback*p){config.error=p;return this;}
 Result openStream(std::shared_ptr<AudioStream>&out){if(failures){failures--;return Result::Error;}out=std::make_shared<AudioStream>(config);latest=out;return Result::OK;}
};
}
