package com.luci.pianoroom;

/** Wait for an opened stream and a stable negotiated format, rather than a fixed sleep. */
public final class AudioStartGate {
 public static final int WAIT=0,READY=1,FAILED=-1;
 private final long deadline;private long stableAt=-1;private int rate,backend;
 public AudioStartGate(long now){deadline=now+4000;}
 public int observe(long now,boolean running,boolean ready,int sampleRate,int engine){
  if(!running||now>=deadline)return FAILED;
  if(!ready||sampleRate<8000||sampleRate>192000){stableAt=-1;return WAIT;}
  if(stableAt<0||rate!=sampleRate||backend!=engine){stableAt=now;rate=sampleRate;backend=engine;}
  return now-stableAt>=100?READY:WAIT;
 }
}
