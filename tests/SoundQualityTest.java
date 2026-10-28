import com.luci.pianoroom.*;
public final class SoundQualityTest {
 static void check(boolean value,String message){if(!value)throw new AssertionError(message);}
 public static void main(String[] args)throws Exception{
  long bank=Synth.load(args[0]);check(bank!=0,"load font");Synth synth=new Synth();synth.backend(args.length>1?1:0);synth.manualBuffer(2);synth.latencyProfile(2);synth.sampleQuality(1);
  for(int i=0;i<4;i++){synth.setLayer(i,bank,i==0?0:i==1?4:i==2?48:89);synth.setGain(i,.12f);check(synth.sampleDetails(i).contains("Hz"),"preset metadata");}
  synth.start();Thread.sleep(200);synth.midi(0xB0,64,127);for(int n=0;n<12;n++){synth.midi(0x90,48+n,100);synth.midi(0x80,48+n,0);}Thread.sleep(50);int[] before=synth.voicesNow();synth.sampleQuality(0);synth.soundEq=true;synth.eqLow=-1;synth.eqMid=1;synth.eqHigh=2;synth.soundChange();Thread.sleep(50);int[] after=synth.voicesNow();for(int i=0;i<4;i++)check(before[i]>0&&after[i]>0,"held notes preserved");check(synth.isRunning()&&synth.sampleQuality()==0,"audio not restarted");
  synth.sampleQuality(1);synth.setLayer(2,bank,49);synth.midi(0x90,70,100);Thread.sleep(120);check(synth.voicesNow()[2]>0,"bank change adopts HQ");check(synth.diagnostic().contains("interpolação cúbica")&&synth.soundDetails().contains("-1 / 1 / 2"),"visible config");if(args.length>1){check(synth.manualBuffer()==2&&synth.acceptedBufferFrames()==192,"2ms request preserved; fake burst honored");check(synth.diagnostic().contains("Processamento nativo médio"),"native timing");}
  synth.panic();check(synth.soundEq&&synth.eqHigh==2,"tone retained");synth.close();Synth.freeBank(bank);System.out.println("PASS: JNI HQ/EQ, bank metadata, four-channel held notes, bank changes, timing/buffer; simulated route.");
 }
}
