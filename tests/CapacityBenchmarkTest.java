package com.luci.pianoroom;
public class CapacityBenchmarkTest {
 static void check(boolean b,String s){if(!b)throw new AssertionError(s);}
 static class Fake implements DiagnosticBenchmark.DiagnosticDriver {
  PianoBenchmark.Metrics m=new PianoBenchmark.Metrics();int[] held=new int[128];int pedal,phase,done,saves;boolean overload;String report;
  public void buffer(int ms){m.bufferMs=ms;m.nativeAudio=true;m.ready=true;m.rate=48000;}
  public void phase(int n){phase=n;m.voices=0;}
  public void midi(int t,int k,int v){if(t==0x90){held[k]++;m.voices+=phase==3?6:2;}if(t==0x80){check(held[k]>0,"balanced off");held[k]--;}if(t==0xB0){pedal=v;if(v==0)m.voices=0;}}
  public int voices(){return m.voices;}
  public PianoBenchmark.Metrics metrics(){if(overload&&m.voices>=190){m.xruns+=20;m.late+=20;}return m.copy();}
  public String details(){return "test memory";}public void checkpoint(String s){saves++;report=s;}public void finished(String s,int rec){done++;report=s;check(rec==-1,"no buffer recommendation");}
  void clear(){check(pedal==0,"pedal released");for(int n:held)check(n==0,"notes released");}
 }
 public static void main(String[]args){
  for(boolean bad:new boolean[]{false,true}){Fake f=new Fake();f.overload=bad;CapacityBenchmark b=new CapacityBenchmark(f,8,"test");b.start(0);long now=0;int ratings=0;
   while(b.active&&now<400000){now+=125;b.tick(now);if(b.waiting){f.clear();ratings++;b.rate(bad?1:0,now);}}
   check(!b.active&&f.done==1&&ratings==4,"four groups complete");f.clear();check(f.report.contains("Estado: Concluído")&&f.report.contains("prazo_p99"),"telemetry report");
   if(bad)check(f.report.contains("sobrecarga em dois intervalos")&&!f.report.contains("INÍCIO Canal 1 / alvo 384"),"stops overloaded group before higher targets");
   else check(f.report.contains("maior alvo sem falhas medidas 512"),"reached target recorded");
  }
  Fake f=new Fake();CapacityBenchmark b=new CapacityBenchmark(f,8,"test");b.start(0);b.tick(1000);b.tick(40000);b.cancel("exit");b.cancel("again");f.clear();check(f.done==1&&!f.report.contains("maior alvo sem falhas medidas 64"),"gap cannot certify capacity and cancel is idempotent");
  long[] a=new long[33],z=new long[33];a[1]=100;a[16]=50;a[17]=45;a[32]=5;check(PerformanceReport.percentile(a,z,50).equals("25")&&PerformanceReport.percentile(a,z,95).equals("50")&&PerformanceReport.percentile(a,z,99).equals(">=400"),"histogram bounds");
  check(PerformanceReport.row(z,a).startsWith("NA"),"counter reset is unavailable");
  System.out.println("PASS: capacity staircase, measured targets, overload stop, balanced MIDI, no burst catch-up, cancellation, percentile bounds and counter reset.");
 }
}
