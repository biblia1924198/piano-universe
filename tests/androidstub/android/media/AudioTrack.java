package android.media;
import java.nio.ByteBuffer;
/** Host test adapter. Not Android or a USB HAL. Delayed head reports are deliberate. */
public class AudioTrack{
 public static volatile AudioTrack latest;public static boolean rejectShrink,freezeHead;public static int requiredFrames;public static long openDelayMs;
 private int capacity=8832,size=8832,threshold=8832,underruns;private long written,played,lastClock,faultClock;private boolean playing,started,released;
 public volatile long maxQueue,nonzero,writeCalls;public volatile boolean thresholdSet,blockingUsed,directUsed;
 public static int getMinBufferSize(int a,int b,int c){return 17664;}
 public static class Builder{int capacity=8832;public Builder setAudioAttributes(AudioAttributes a){return this;}public Builder setAudioFormat(AudioFormat a){return this;}public Builder setTransferMode(int n){return this;}public Builder setBufferSizeInBytes(int n){capacity=n/4;return this;}public Builder setPerformanceMode(int n){return this;}public AudioTrack build(){if(openDelayMs>0)try{Thread.sleep(openDelayMs);}catch(InterruptedException e){Thread.currentThread().interrupt();}AudioTrack t=new AudioTrack();t.capacity=t.size=t.threshold=capacity;latest=t;return t;}}
 private void update(){long now=System.nanoTime();if(started&&playing){long frames=(now-lastClock)*44100/1000000000L;if(frames>0){played+=Math.min(written-played,frames);lastClock+=frames*1000000000L/44100;}if(size<requiredFrames&&now-faultClock>=100000000L){underruns++;faultClock=now;}}}
 public synchronized int getPlaybackHeadPosition(){update();return freezeHead?0:(int)(played/512*512);}
 public synchronized int getBufferSizeInFrames(){return size;}public int getBufferCapacityInFrames(){return capacity;}public int getPerformanceMode(){return 0;}
 public synchronized int getUnderrunCount(){update();return underruns;}
 public synchronized int setBufferSizeInFrames(int n){size=rejectShrink?capacity:Math.min(capacity,n);notifyAll();return size;}
 public synchronized int setStartThresholdInFrames(int n){threshold=Math.min(size,n);thresholdSet=true;return threshold;}public int getStartThresholdInFrames(){return threshold;}
 public synchronized boolean setPreferredDevice(AudioDeviceInfo d){return true;}public AudioDeviceInfo getRoutedDevice(){return new AudioDeviceInfo();}
 public synchronized void play(){playing=true;notifyAll();}public synchronized void pause(){playing=false;notifyAll();}public synchronized void flush(){written=played=0;started=false;notifyAll();}public synchronized void release(){released=true;notifyAll();}
 public synchronized int write(ByteBuffer pcm,int count,int mode){writeCalls++;blockingUsed|=mode==0;directUsed|=pcm.isDirect();int requested=count/4,total=0;while(total<requested){if(released)return total==0?-6:total*4;if(!playing)return total*4;update();int available=(int)Math.max(0,size-(written-played));int frames=Math.min(requested-total,available);if(frames>0){for(int i=0;i<frames*2;i++)if(pcm.getShort()!=0)nonzero++;written+=frames;total+=frames;maxQueue=Math.max(maxQueue,written-played);if(!started&&written>=threshold){started=true;lastClock=faultClock=System.nanoTime();}}else{if(mode!=0)return total*4;try{wait(1);}catch(InterruptedException e){Thread.currentThread().interrupt();return total*4;}}}return total*4;}
}
