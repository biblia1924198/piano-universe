import com.luci.pianoroom.Synth;import android.media.AudioTrack;
public class SynthLoopTest{
 static void check(boolean ok,String m){if(!ok)throw new AssertionError(m);}
 public static void main(String[] args)throws Exception{
  long bank=Synth.load(args[0]);check(bank!=0,"load bank through JNI");Synth synth=new Synth();synth.setLayer(0,bank,0);synth.setLayer(1,bank,89);
  AudioTrack.freezeHead=true;synth.start();Thread.sleep(180);synth.midi(0x90,60,100);Thread.sleep(250);synth.midi(0x80,60,0);AudioTrack t=AudioTrack.latest;long stop=System.nanoTime();synth.stop();check(System.nanoTime()-stop<1000000000L,"stop interrupts blocking write");check(t.nonzero>0&&t.writeCalls>40,"frozen/quantized playback head does not starve writes");check(t.blockingUsed&&t.directUsed,"blocking writes and direct PCM");check(t.maxQueue<=t.getBufferSizeInFrames(),"honor accepted operation buffer");check(synth.error.isEmpty(),synth.error);
  AudioTrack.freezeHead=false;AudioTrack.requiredFrames=2048;synth.start();Thread.sleep(200);synth.midi(0x90,64,100);Thread.sleep(2300);t=AudioTrack.latest;synth.stop();check(t.getBufferSizeInFrames()>=2048,"adaptive buffer escapes 16ms ceiling and meets simulated route requirement");check(t.nonzero>0&&synth.error.isEmpty(),"adapted stream remains audible");System.out.println("PASS: actual Synth + JNI, direct synchronized PCM, delayed/frozen head reports, stop during blocking write, adaptive buffer "+t.getBufferSizeInFrames()+" frames; simulated USB only.");
  AudioTrack.requiredFrames=0;synth.latencyProfile(2);synth.manualBuffer(16);synth.automaticBuffer(false);synth.blockSize(128);synth.start();Thread.sleep(180);synth.cleanTone();Thread.sleep(220);t=AudioTrack.latest;synth.stop();check(t.nonzero>0&&t.getBufferSizeInFrames()<800,"manual buffer and clean tone");
  android.os.Build.VERSION.SDK_INT=30;AudioTrack.rejectShrink=true;synth.start();Thread.sleep(150);synth.midi(0x90,64,90);Thread.sleep(250);t=AudioTrack.latest;synth.stop();check(!t.thresholdSet&&t.nonzero>0&&synth.error.isEmpty(),"pre-31 and buffer refusal still play, without priming deadlock");
  synth.close();Synth.freeBank(bank);System.out.println("PASS: manual mode, live block settings, clean test tone, Android30, rejected buffer shrink and lifecycle. Not a physical latency measurement.");
 }
}
