package com.luci.pianoroom;
import java.io.*;import java.util.*;
/** Standard MIDI file type 0, 480 PPQ, 120 BPM. Timing is monotonic. */
public final class MidiTape {
 public static final class Event {public final long micros;public final int status,a,b;Event(long t,int s,int x,int y){micros=t;status=s;a=x;b=y;}}
 private final ArrayList<Event> events=new ArrayList<>();private long origin,endMicros;public volatile boolean recording;public String warning="";
 public synchronized void start(){events.clear();origin=System.nanoTime();recording=true;endMicros=0;warning="";}
 public void add(int s,int a,int b){if(!recording)return;synchronized(this){if(!recording)return;int type=s&240;if(type!=128&&type!=144&&type!=176&&type!=224&&type!=192)return;if(events.size()>=180000){recording=false;warning="Limite de eventos MIDI atingido.";return;}events.add(new Event((System.nanoTime()-origin)/1000,s,a,b));}}
 public synchronized void stop(){if(recording)endMicros=Math.max(0,(System.nanoTime()-origin)/1000);recording=false;}
 public void save(File file)throws IOException{ArrayList<Event> snapshot;long end;synchronized(this){stop();snapshot=new ArrayList<>(events);end=endMicros;}write(file,snapshot,end);}
 public static void write(File file,List<Event> events,long duration)throws IOException{
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream track=new DataOutputStream(bytes);track.write(new byte[]{0,(byte)255,0x51,3,7,(byte)161,0x20});long last=0;int channels=0;
  for(Event e:events){channels|=1<<(e.status&15);long tick=e.micros*960/1000000;var(track,Math.max(0,tick-last));track.writeByte(e.status);track.writeByte(e.a);if((e.status&240)!=192&&(e.status&240)!=208)track.writeByte(e.b);last=tick;}
  long end=Math.max(last,duration*960/1000000);var(track,end-last);if(channels==0)channels=1;boolean first=true;for(int ch=0;ch<16;ch++)if((channels&(1<<ch))!=0){if(!first)track.writeByte(0);first=false;track.write(new byte[]{(byte)(176|ch),64,0});track.write(new byte[]{0,(byte)(176|ch),123,0});}track.write(new byte[]{0,(byte)255,0x2F,0});
  try(DataOutputStream out=new DataOutputStream(new FileOutputStream(file))){out.writeBytes("MThd");out.writeInt(6);out.writeShort(0);out.writeShort(1);out.writeShort(480);out.writeBytes("MTrk");out.writeInt(bytes.size());bytes.writeTo(out);}
 }
 private static void var(DataOutputStream out,long value)throws IOException{if(value>0x0FFFFFFF)throw new IOException("Intervalo MIDI excessivo");int v=(int)value,buffer=v&127;while((v>>=7)>0)buffer=(buffer<<8)|((v&127)|128);while(true){out.writeByte(buffer);if((buffer&128)==0)break;buffer>>>=8;}}
 private static long readVar(DataInputStream in)throws IOException{long v=0;for(int i=0;i<4;i++){int b=in.readUnsignedByte();v=(v<<7)|(b&127);if((b&128)==0)return v;}throw new IOException("MIDI inválido");}
 public static List<Event> read(File file)throws IOException{
  try(DataInputStream in=new DataInputStream(new FileInputStream(file))){if(in.readInt()!=0x4D546864)throw new IOException("MIDI inválido");int header=in.readInt(),format=in.readUnsignedShort(),tracks=in.readUnsignedShort(),division=in.readUnsignedShort();if(header<6||format!=0||tracks!=1||(division&32768)!=0||division==0)throw new IOException("Reprodução aceita gravações MIDI tipo 0 do app");in.skipBytes(header-6);if(in.readInt()!=0x4D54726B)throw new IOException("Trilha MIDI inválida");int len=in.readInt();if(len<0||len>16*1024*1024)throw new IOException("Trilha excessiva");byte[] data=new byte[len];in.readFully(data);DataInputStream tr=new DataInputStream(new ByteArrayInputStream(data));List<Event> out=new ArrayList<>();long micros=0;int tempo=500000,running=0;
   while(tr.available()>0){long delta=readVar(tr);micros+=delta*tempo/division;int s=tr.readUnsignedByte(),first=-1;if(s<128){first=s;s=running;}else if(s<240)running=s;if(s==255){int type=tr.readUnsignedByte(),n=(int)readVar(tr);byte[] meta=new byte[n];tr.readFully(meta);if(type==0x51&&n==3)tempo=((meta[0]&255)<<16)|((meta[1]&255)<<8)|(meta[2]&255);if(type==0x2F)break;continue;}if(s==240||s==247){tr.skipBytes((int)readVar(tr));continue;}if(s<128||s>=240)throw new IOException("Evento MIDI inválido");int a=first>=0?first:tr.readUnsignedByte(),type=s&240,b=type==192||type==208?0:tr.readUnsignedByte();out.add(new Event(micros,s,a,b));if(out.size()>180100)throw new IOException("MIDI excessivo");}
   return out;
  }
 }
}
