package com.luci.pianoroom;
import java.io.*;import java.util.*;
public class AuditionTest {
 static void check(boolean ok,String msg){if(!ok)throw new AssertionError(msg);}
 static void invalid(String text)throws Exception{try{AuditionSequence.notes(text,90,80,true);throw new AssertionError("accepted: "+text);}catch(IOException expected){}}
 static void balanced(AuditionSequence s){int[]held=new int[128];int pedal=-1;long before=-1;for(MidiTape.Event e:s.events){check(e.micros>=before,"monotonic");before=e.micros;if(e.status==0x90&&e.b>0)held[e.a]++;else if(e.status==0x80){check(held[e.a]>0,"off has on");held[e.a]--;}else if(e.status==0xB0&&e.a==64)pedal=e.b;}for(int n:held)check(n==0,"all released");check(pedal==0,"pedal up");}
 static class Fake implements AuditionPlayer.Sink{volatile boolean ready=true;volatile String error;final List<int[]>events=Collections.synchronizedList(new ArrayList<>());public boolean ready(){return ready;}public void event(int type,int key,int value){events.add(new int[]{type,key,value});}public void finished(String e){error=e;}}
 public static void main(String[]args)throws Exception{
  AuditionSequence example=AuditionSequence.notes(AuditionSequence.EXAMPLE,90,88,true);balanced(example);check(example.steps==8,"example steps");
  AuditionSequence aliases=AuditionSequence.notes("Dó4 C4 Réb4 Db4 Fá#4 F#4 Si3 B3 pausa:2 C4+C4+E4:2",120,80,false);balanced(aliases);int[]keys=aliases.events.stream().filter(e->e.status==0x90).mapToInt(e->e.a).toArray();check(Arrays.equals(keys,new int[]{60,60,61,61,66,66,59,59,60,64}),"accent/English/sharp/flat/chord dedup");
  for(String bad:new String[]{"","pausa:1","Dó10","C-2","G#9","C4:NaN","C4:Infinity","C4:0","C4:99","C4::2","C4+","C4:abc","???"})invalid(bad);
  StringBuilder longSong=new StringBuilder();for(int i=0;i<257;i++)longSong.append("C4 ");invalid(longSong.toString());invalid("C4:16 C4:16 C4:16 C4:16 C4:16 C4:16 C4:16 C4:16 C4:16 C4:16 C4:16 C4:16 C4:16 C4:16 C4:16 C4:16 C4:16 C4:16");
  AuditionSequence hymn=AuditionSequence.midi(MidiTape.read(new File(args[0])),100);balanced(hymn);check(hymn.steps==197&&hymn.durationMicros>44000000&&hymn.durationMicros<47000000,"real hymn and timing");check(AuditionSequence.midi(MidiTape.read(new File(args[0])),50).durationMicros>87000000,"speed scaling");
  Fake f=new Fake();AuditionPlayer player=new AuditionPlayer(AuditionSequence.notes("C4:16",40,90,true),f);player.start();Thread.sleep(80);player.cancel();player.join(500);check(player.finished()&&f.error!=null,"cancel completes");int held=0,pedal=0;for(int[]e:f.events){if(e[0]==0x90)held++;if(e[0]==0x80)held--;if(e[0]==0xB0)pedal=e[2];}check(held==0&&pedal==0,"cancel releases note and pedal");
  Fake missing=new Fake();missing.ready=false;AuditionPlayer waiting=new AuditionPlayer(example,missing);waiting.start();Thread.sleep(20);waiting.cancel();waiting.join(500);check(waiting.finished()&&missing.events.stream().noneMatch(e->e[0]==0x90),"cancel while waiting for audio starts no notes");
  Fake done=new Fake();AuditionPlayer natural=new AuditionPlayer(AuditionSequence.notes("C4:0.125",200,80,false),done);natural.start();natural.join(3000);check(natural.finished()&&"".equals(done.error),"natural finish and tail");check(done.events.stream().filter(e->e[0]==0x90).count()==1&&done.events.stream().filter(e->e[0]==0x80).count()==1,"natural finish does not duplicate notes");
  System.out.println("PASS: Portuguese/English notes, accidentals, durations/chords/rests, invalid input/limits, real 197-note hymn, speed scaling, audio-wait cancellation, active-note/pedal cleanup and natural completion.");
 }
}
