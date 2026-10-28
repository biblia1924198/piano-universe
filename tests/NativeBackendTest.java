import com.luci.pianoroom.*;
public final class NativeBackendTest{
 static void check(boolean ok,String s){if(!ok)throw new AssertionError(s);}
 public static void main(String[] args)throws Exception{
  long bank=Synth.load(args[0]);check(bank!=0,"bank load");Synth synth=new Synth();synth.setLayer(0,bank,0);synth.setLayer(1,bank,89);synth.setGain(0,2);check(synth.gain[0]==2,"200% gain not clamped to 100%");synth.master=2;synth.wet=.4f;synth.change();synth.touch(new TouchResponse(1,1,1,127,100));synth.backend(1);synth.start();Thread.sleep(120);for(int n=48;n<76;n++)synth.midi(0x90,n,127);Thread.sleep(300);synth.manualBuffer(8);synth.latencyProfile(2);
  // Exercise font replacement, gain changes and MIDI while callback is running.
  synth.setLayer(1,bank,4);synth.setGain(1,1.6f);synth.midi(0xB0,64,127);synth.program(0,0);synth.midi(0x90,64,75);Thread.sleep(850);
  String report=synth.diagnostic();String scenario=args[1];if(scenario.equals("reject")||scenario.equals("disconnect"))check(report.contains("AudioTrack estável")&&report.contains("Retorno automático"),"native failure falls back safely: "+report);else{check(report.contains("AAudio nativo")&&report.contains("48000 Hz")&&report.contains("burst 192"),"actual accepted stream: "+report);check(report.contains(scenario.equals("shared")?"compartilhado":"exclusivo"),"sharing mode shown");check(synth.acceptedBufferFrames()==384,"8ms native-rate buffer");}
  if(scenario.equals("exclusive")||scenario.equals("shared")){synth.manualBuffer(2);Thread.sleep(130);check(synth.manualBuffer()==2&&synth.acceptedBufferFrames()==192,"2ms request accepted at 4ms hardware burst floor");check(synth.diagnostic().contains("excessos: 0"),"no MIDI queue overflow in physical-scale playing");}
  long stop=System.nanoTime();synth.stop();check(System.nanoTime()-stop<1000000000L,"stop completes off callback");check(synth.error.isEmpty(),synth.error);synth.close();synth.midi(0x90,60,127);synth.demo(60,true);Synth.freeBank(bank);
  System.out.println("PASS: native callback / "+scenario+", gains up to 200%, stereo limiter, 48kHz native rate, frame burst, live bank replacement, MIDI sustain, buffer changes, fallback and stop. Fake host route; no physical latency result.");
 }
}
