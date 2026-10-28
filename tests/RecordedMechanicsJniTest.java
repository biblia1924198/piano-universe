import com.luci.pianoroom.*;
public final class RecordedMechanicsJniTest {
 public static void main(String[]args)throws Exception{
  if(Synth.mechanicalLoad(args[0]+".missing"))throw new AssertionError("missing capture accepted");
  if(!Synth.mechanicalLoad(args[0]))throw new AssertionError("pack JNI load");
  long bank=Synth.load(args[1]);if(bank==0)throw new AssertionError("sampler");Synth synth=new Synth();
  try{
   synth.setLayer(0,bank,0);synth.setGain(0,.4f);if(args.length>2)synth.backend(1);
   ChannelSettings c=synth.channels[0];c.hammer=40;c.keyNoise=40;c.pedalNoise=20;synth.channelChange(0);synth.start();Thread.sleep(100);
   synth.midi(0xB0,64,127);for(int i=0;i<80;i++){int key=21+i%88;synth.midi(0x90,key,30+i%98);synth.midi(0x80,key,0);if(i%10==0)Thread.sleep(8);}
   if(!Synth.mechanicalLoad(args[0]))throw new AssertionError("immutable repeat load");
   synth.midi(0xB0,64,0);synth.panic();if(!synth.isRunning())throw new AssertionError("stream stopped");
   System.out.println("PASS: actual JNI recorded-pack load/reload, configured mechanics, rapid notes/pedal and panic on simulated "+(args.length>2?"AAudio":"AudioTrack"));
  }finally{synth.close();Synth.freeBank(bank);}
 }
}
