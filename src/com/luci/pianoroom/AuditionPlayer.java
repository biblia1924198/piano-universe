package com.luci.pianoroom;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.LockSupport;
/** One short preview. Cleanup completes on the player thread before the UI restores the mixer. */
public final class AuditionPlayer {
 public interface Sink {boolean ready();void event(int type,int key,int value);void finished(String error);}
 private final AtomicBoolean running=new AtomicBoolean(true);private final Sink sink;private final AuditionSequence sequence;private final Thread thread;
 public AuditionPlayer(AuditionSequence sequence,Sink sink){this.sequence=sequence;this.sink=sink;thread=new Thread(this::run,"PianoAudition");}
 public void start(){thread.start();}public boolean active(){return running.get();}
 public void cancel(){running.set(false);LockSupport.unpark(thread);}
 public boolean finished(){return !thread.isAlive();}
 public void join(long ms)throws InterruptedException{thread.join(ms);}
 private boolean until(long deadline){while(running.get()){long delay=deadline-System.nanoTime();if(delay<=0)return true;LockSupport.parkNanos(Math.min(delay,5000000L));}return false;}
 private void run(){String error="";int[] held=new int[128];try{
  long deadline=System.nanoTime()+2000000000L;while(running.get()&&!sink.ready()&&System.nanoTime()<deadline)LockSupport.parkNanos(5000000L);
  if(running.get()&&!sink.ready())throw new IllegalStateException("A saída de áudio não ficou pronta. Use Retomar áudio.");long origin=System.nanoTime();
  for(MidiTape.Event e:sequence.events){long target=origin+e.micros*1000;if(!until(target)||!running.get())break;if(System.nanoTime()-target>250000000L)throw new IllegalStateException("A reprodução foi pausada por muito tempo. Toque novamente.");
   if(!sink.ready())throw new IllegalStateException("A saída de áudio foi interrompida.");int type=e.status&240;sink.event(type,e.a,e.b);if(type==0x90&&e.b>0)held[e.a]++;else if(type==0x80||type==0x90&&e.b==0)held[e.a]=Math.max(0,held[e.a]-1);
  }
  if(running.get())until(origin+sequence.durationMicros*1000);
 }catch(Throwable e){error=e.getMessage()==null?"Não foi possível tocar a demonstração.":e.getMessage();}
 finally{try{for(int k=0;k<128;k++)while(held[k]-->0)sink.event(0x80,k,0);sink.event(0xB0,64,0);}catch(Throwable ignored){}running.set(false);sink.finished(error);}}
}
