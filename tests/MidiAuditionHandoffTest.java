package com.luci.pianoroom;
import java.util.*;
public final class MidiAuditionHandoffTest{
 public static void main(String[]args){List<String> log=new ArrayList<>();List<Runnable> queue=new ArrayList<>();boolean[] preview={false};MidiAuditionHandoff[] gate={null};
 gate[0]=new MidiAuditionHandoff(()->preview[0],()->{log.add("stop");preview[0]=false;},queue::add,(s,a,b)->{log.add("note"+a);if(a==60)gate[0].accept(144,62,80);});
 gate[0].accept(144,40,80);if(!log.equals(Arrays.asList("note40"))||!queue.isEmpty())throw new AssertionError("Normal MIDI must dispatch immediately");log.clear();preview[0]=true;gate[0].accept(144,60,80);gate[0].accept(128,61,0);if(queue.size()!=1||!log.isEmpty())throw new AssertionError("Preview queues one handoff");queue.remove(0).run();if(!log.equals(Arrays.asList("stop","note60","note61","note62")))throw new AssertionError(log.toString());gate[0].accept(144,63,80);if(!log.get(4).equals("note63")||!queue.isEmpty())throw new AssertionError("Direct dispatch resumes only after queue drains");System.out.println("PASS: live MIDI stays immediate; preview stops before first live note; note-off and concurrent events keep FIFO order; direct dispatch resumes after drain.");}
}
