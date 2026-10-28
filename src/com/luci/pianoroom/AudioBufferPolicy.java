package com.luci.pianoroom;
/** Tune the effective AudioTrack buffer. Stability is not capped at 16 ms. */
public final class AudioBufferPolicy {
 public static final int RATE=44100, BLOCK=256, MAX_MS=120;
 private int target=framesForMs(32),capacity=framesForMs(200),lastUnderruns;
 private boolean adaptive=true;private long nextCheck;
 public int frames(){return target;}
 public void configure(int profile,int manualMs,boolean adapt,int capacityFrames){
  capacity=Math.max(128,capacityFrames);adaptive=adapt;
  target=Math.min(capacity,framesForMs(profile==0?32:profile==1?12:manualMs));
  lastUnderruns=0;nextCheck=0;
 }
 public void profile(int p){configure(p,32,true,framesForMs(200));}
 public boolean observe(int underruns,long now){
  if(nextCheck==0||underruns<lastUnderruns){lastUnderruns=underruns;nextCheck=now+500000000L;return false;}
  if(now<nextCheck)return false;
  nextCheck=now+500000000L;int delta=underruns-lastUnderruns;lastUnderruns=underruns;
  if(!adaptive||delta<=0)return false;
  int limit=Math.min(capacity,framesForMs(MAX_MS));
  int wanted=delta>=3?target*2:target+framesForMs(8);
  int next=Math.min(limit,wanted);if(next<=target)return false;target=next;return true;
 }
 public static int framesForMs(int ms){int clamped=Math.max(8,Math.min(200,ms));return ((RATE*clamped+999)/1000+15)/16*16;}
 public static long queued(long written,long played){return Math.max(0,written-played);}
 public static double millis(long frames){return frames*1000.0/RATE;}
}
