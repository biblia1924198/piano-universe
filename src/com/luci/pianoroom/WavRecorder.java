package com.luci.pianoroom;
import java.io.*;import java.nio.*;import java.nio.channels.*;import java.util.concurrent.locks.LockSupport;
/** PCM after mixer/reverb/limiter. A worker drains the native bounded ring. */
public final class WavRecorder {
 private Thread thread;private volatile boolean stop;public volatile boolean active;public volatile String error="";public volatile long bytes,firstNanos;public File file;public int rate;
 public synchronized void start(File target,int sampleRate)throws IOException {
  if(thread!=null)throw new IOException("Já existe gravação de áudio");file=target;rate=sampleRate;bytes=0;firstNanos=0;error="";stop=false;
  RandomAccessFile out=new RandomAccessFile(target,"rw");out.setLength(0);out.write(new byte[44]);
  ByteBuffer pcm=ByteBuffer.allocateDirect(4096*4).order(ByteOrder.LITTLE_ENDIAN);Synth.recordStart(rate);active=true;
  thread=new Thread(()->{try{FileChannel channel=out.getChannel();while(true){int n=Synth.recordRead(pcm,4096);if(n>0){pcm.position(0);pcm.limit(n*4);while(pcm.hasRemaining())channel.write(pcm);bytes+=n*4;firstNanos=Synth.recordFirstNanos();if(bytes>3500000000L){error="Limite WAV atingido; gravação finalizada.";stop=true;Synth.recordStop();}}else if(stop||Synth.recordFailure()!=0)break;else LockSupport.parkNanos(2000000L);}
   int failure=Synth.recordFailure();if(failure!=0)error=failure==1?"Gravação interrompida: armazenamento não acompanhou o áudio.":"Gravação encerrada: taxa de áudio mudou.";
  }catch(Throwable e){error="Falha ao gravar: "+e.getMessage();}finally{Synth.recordStop();try{out.setLength(44+bytes);out.seek(0);out.write(header(bytes,rate));out.close();}catch(IOException e){error=e.getMessage();}active=false;}},"PianoWavWriter");thread.start();
 }
 public synchronized void finish(){if(thread==null)return;stop=true;Synth.recordStop();try{thread.join();}catch(InterruptedException e){Thread.currentThread().interrupt();}thread=null;active=false;}
 static byte[] header(long bytes,int rate){ByteBuffer b=ByteBuffer.allocate(44).order(ByteOrder.LITTLE_ENDIAN);b.put(new byte[]{'R','I','F','F'}).putInt((int)(bytes+36)).put(new byte[]{'W','A','V','E','f','m','t',' '}).putInt(16).putShort((short)1).putShort((short)2).putInt(rate).putInt(rate*4).putShort((short)4).putShort((short)16).put(new byte[]{'d','a','t','a'}).putInt((int)bytes);return b.array();}
}
