package com.luci.pianoroom;
import java.util.ArrayDeque;
import java.util.function.BooleanSupplier;
import java.util.function.Consumer;
/** Normal MIDI stays on its receiving thread; a preview handoff drains in order. */
public final class MidiAuditionHandoff {
 public interface Receiver{void accept(int status,int a,int b);}
 private final BooleanSupplier preview;private final Runnable stop;private final Consumer<Runnable> schedule;private final Receiver receive;private final ArrayDeque<int[]> pending=new ArrayDeque<>();private boolean draining;
 public MidiAuditionHandoff(BooleanSupplier preview,Runnable stop,Consumer<Runnable> schedule,Receiver receive){this.preview=preview;this.stop=stop;this.schedule=schedule;this.receive=receive;}
 public void accept(int status,int a,int b){boolean post=false,direct=false;synchronized(pending){if(preview.getAsBoolean()||draining){pending.add(new int[]{status,a,b});if(!draining){draining=true;post=true;}}else direct=true;}if(post)schedule.accept(this::drain);if(direct)receive.accept(status,a,b);}
 private void drain(){stop.run();for(;;){int[] e;synchronized(pending){e=pending.poll();if(e==null){draining=false;return;}}receive.accept(e[0],e[1],e[2]);}}
}
