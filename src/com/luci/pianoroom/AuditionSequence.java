package com.luci.pianoroom;
import java.io.IOException;
import java.text.Normalizer;
import java.util.*;
/** Readable note sequences use scientific octave numbers: C4/Do4 is MIDI 60. */
public final class AuditionSequence {
 public static final String EXAMPLE="Dó4 Ré4 Mi4 Sol4 | Dó3+Mi3+Sol3:2 | Fá3+Lá3+Dó4:2 | Sol3+Si3+Ré4:2 | Dó3+Mi3+Sol3+Dó4:4";
 public final List<MidiTape.Event> events;public final long durationMicros;public final int steps;
 private AuditionSequence(List<MidiTape.Event> e,long duration,int count){events=Collections.unmodifiableList(e);durationMicros=duration;steps=count;}
 public static AuditionSequence notes(String raw,int bpm,int velocity,boolean pedal)throws IOException {
  if(bpm<40||bpm>200||velocity<1||velocity>127)throw new IOException("Andamento ou intensidade fora da faixa.");
  if(raw==null||raw.length()>4096)throw new IOException("Use uma sequência de até 4096 caracteres.");
  String normalized=Normalizer.normalize(raw,Normalizer.Form.NFD).replaceAll("\\p{M}+","").replace('♯','#').replace('♭','b').trim();
  String[] tokens=normalized.split("[\\s,;|]+");if(normalized.isEmpty())throw new IOException("Escreva notas, como Dó4 Ré4 Mi4.");
  ArrayList<MidiTape.Event> events=new ArrayList<>();long at=0;int count=0;if(pedal)events.add(new MidiTape.Event(0,0xB0,64,127));
  for(String token:tokens){if(token.isEmpty())continue;if(++count>256)throw new IOException("Use até 256 passos.");String[] timed=token.split(":",-1);if(timed.length>2)throw invalid(token);
   double beats=1;try{if(timed.length==2)beats=Double.parseDouble(timed[1]);}catch(NumberFormatException e){throw invalid(token);}
   if(!Double.isFinite(beats)||beats<.125||beats>16)throw new IOException("Duração deve estar entre 0.125 e 16 tempos: "+token);
   long length=Math.round(beats*60000000.0/bpm);String chord=timed[0].toLowerCase(Locale.ROOT);
   if(!chord.equals("pausa")&&!chord.equals("rest")&&!chord.equals("-")){
    Set<Integer> notes=new LinkedHashSet<>();String[] parts=chord.split("\\+",-1);if(parts.length>10)throw new IOException("Use até dez notas por acorde.");for(String p:parts)notes.add(pitch(p));
    for(int key:notes)events.add(new MidiTape.Event(at,0x90,key,velocity));
    for(int key:notes)events.add(new MidiTape.Event(at+Math.max(1,length*9/10),0x80,key,0));
   }
   at+=length;if(at>180000000L)throw new IOException("A sequência deve durar até três minutos.");
  }
  if(events.isEmpty()||pedal&&events.size()==1)throw new IOException("A sequência precisa conter ao menos uma nota.");
  events.add(new MidiTape.Event(at,0xB0,64,0));return new AuditionSequence(events,at+2000000L,count);
 }
 private static IOException invalid(String token){return new IOException("Não entendi “"+token+"”. Use Dó4, C4, Fá#4 ou Dó3+Mi3+Sol3:2.");}
 static int pitch(String word)throws IOException {
  java.util.regex.Matcher m=java.util.regex.Pattern.compile("^(do|re|mi|fa|sol|la|si|[a-g])([#b]?)(-?[0-9])$").matcher(word.toLowerCase(Locale.ROOT));if(!m.matches())throw invalid(word);
  String n=m.group(1);int semitone=n.equals("do")||n.equals("c")?0:n.equals("re")||n.equals("d")?2:n.equals("mi")||n.equals("e")?4:n.equals("fa")||n.equals("f")?5:n.equals("sol")||n.equals("g")?7:n.equals("la")||n.equals("a")?9:11;
  int key=(Integer.parseInt(m.group(3))+1)*12+semitone+(m.group(2).equals("#")?1:m.group(2).equals("b")?-1:0);if(key<0||key>127)throw new IOException("Nota fora da extensão MIDI: "+word);return key;
 }
 public static AuditionSequence midi(List<MidiTape.Event> source,int speedPercent)throws IOException {
  if(speedPercent<50||speedPercent>150||source==null||source.size()>20000)throw new IOException("Demonstração inválida.");
  ArrayList<MidiTape.Event> events=new ArrayList<>();long end=0,previous=-1;int on=0;int[] held=new int[128];
  for(MidiTape.Event e:source){int type=e.status&240;if(e.micros<previous||e.micros<0||e.a<0||e.a>127||e.b<0||e.b>127)throw new IOException("Eventos de demonstração inválidos.");previous=e.micros;long time=Math.round(e.micros*100.0/speedPercent);if(time>180000000L)throw new IOException("Demonstração muito longa.");
   if(type==0x90&&e.b>0){held[e.a]++;on++;events.add(new MidiTape.Event(time,0x90,e.a,e.b));}
   else if(type==0x80||type==0x90&&e.b==0){if(held[e.a]<=0)throw new IOException("Demonstração contém soltura sem nota.");held[e.a]--;events.add(new MidiTape.Event(time,0x80,e.a,0));}
   else if(type==0xB0&&e.a==64)events.add(new MidiTape.Event(time,0xB0,64,e.b));
   end=Math.max(end,time);
  }
  for(int n:held)if(n!=0)throw new IOException("Demonstração contém notas sem soltura.");if(on==0)throw new IOException("Demonstração sem notas.");events.add(new MidiTape.Event(end,0xB0,64,0));return new AuditionSequence(events,end+2000000L,on);
 }
}
