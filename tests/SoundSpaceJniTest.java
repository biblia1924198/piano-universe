import com.luci.pianoroom.*;
public final class SoundSpaceJniTest {
 public static void main(String[]args)throws Exception{long bank=Synth.load(args[0]);if(bank==0)throw new AssertionError("bank");Synth synth=new Synth();try{
  synth.setLayer(0,bank,0);synth.setGain(0,.4f);if(args.length>1)synth.backend(1);float[] profile=Synth.analyzePiano(bank,0);if(profile==null||profile.length!=792||PianoProfile.covered(profile)==0)throw new AssertionError("native spectral profile");PianoProfile.encode(profile);synth.profile(0,profile);
  synth.channels[0]=PianoSoundPresets.apply(synth.channels[0],5);synth.channelChange(0);synth.start();Thread.sleep(100);synth.midi(0xB0,64,127);
  for(int i=0;i<20;i++){synth.midi(0x90,60,75+i);Thread.sleep(4);synth.midi(0x80,60,0);}if(synth.voicesNow()[0]==0)throw new AssertionError("active voices");
  for(int id=1;id<=5;id++){synth.channels[0]=PianoSoundPresets.apply(synth.channels[0],id);synth.channelChange(0);Thread.sleep(15);}synth.midi(0xB0,64,0);synth.channels[0].hybrid=false;synth.channelChange(0);synth.panic();if(!synth.isRunning())throw new AssertionError("stream lost");System.out.println("PASS: actual Synth/JNI profile v2, five recipes, sustained repetitions/live effect changes/disable/panic on "+(args.length>1?"simulated AAudio":"simulated AudioTrack"));
 }finally{synth.close();Synth.freeBank(bank);}}
}
