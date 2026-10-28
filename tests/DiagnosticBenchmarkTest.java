package com.luci.pianoroom;
public class DiagnosticBenchmarkTest {
 static void check(boolean b,String s){if(!b)throw new AssertionError(s);}
 static class Fake implements DiagnosticBenchmark.DiagnosticDriver {
  PianoBenchmark.Metrics m=new PianoBenchmark.Metrics();int[] held=new int[128];int pedal,done,saves,phase;String report;
  public void buffer(int ms){m.bufferMs=ms;m.nativeAudio=true;m.rate=48000;m.ready=true;}
  public void midi(int t,int k,int v){if(t==0x90)held[k]++;if(t==0x80){check(held[k]>0,"balanced off");held[k]--;}if(t==0xB0)pedal=v;}
  public PianoBenchmark.Metrics metrics(){return m.copy();}public void phase(int p){phase=p;}public String details(){return "fake";}
  public void checkpoint(String r){saves++;report=r;}public void finished(String r,int rec){done++;report=r;check(rec==-1,"diagnostic cannot prescribe buffer");}
  void clear(){for(int n:held)check(n==0,"held notes cleared");check(pedal==0,"pedal released");}
 }
 public static void main(String[] args){
  Fake f=new Fake();DiagnosticBenchmark b=new DiagnosticBenchmark(f,8,"test");b.start(0);long now=0;boolean sawIdle=false,sawResume=false;
  while(b.active&&now<800000){now+=125;b.tick(now);if(f.phase==6){sawIdle=true;f.clear();}if(f.phase==7)sawResume=true;if(b.waiting){f.clear();b.rate(0,now);}}
  check(!b.active&&sawIdle&&sawResume&&f.done==1&&f.saves>30,"full protocol");check(f.report.contains("Estado: Concluído")&&f.report.contains("vozes1;vozes2;vozes3"),"complete diagnostic");f.clear();
  f=new Fake();b=new DiagnosticBenchmark(f,8,"test");b.start(0);b.tick(1000);b.tick(10000);f.m.nativeAudio=false;f.m.xruns=4;b.tick(11000);f.m.xruns=0;b.tick(12000);b.cancel("focus lost");b.cancel("again");f.clear();check(f.done==1&&f.report.contains("MOTOR MUDOU")&&f.report.contains("CONTADOR REINICIOU")&&f.report.contains("9000"),"fallback/reset/gap/cancel logged");
  f=new Fake();b=new DiagnosticBenchmark(f,8,"test");b.start(0);b.tick(1000);b.tick(90000);check(b.waiting,"stall ends timed phase without catching up notes");b.cancel("stop");f.clear();check(f.report.contains("notas emitidas 6"),"no catch-up flood");
  System.out.println("PASS: eight stages, idle/resume, balanced notes/pedal, partial reports, fallback/reset/gaps, cancellation, no burst catch-up or automatic buffer recommendation.");
 }
}
