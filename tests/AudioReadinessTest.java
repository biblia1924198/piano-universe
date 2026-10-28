import com.luci.pianoroom.*;import android.media.AudioTrack;import java.io.*;
/** Exercise actual Synth/JNI startup through slow AudioTrack and AAudio adapters. */
public final class AudioReadinessTest {
 static void check(boolean b,String s){if(!b)throw new AssertionError(s);}
 public static void main(String[] args)throws Exception {
  boolean nativeEngine=args.length>1;AudioTrack.openDelayMs=350;
  long font=Synth.load(args[0]);check(font!=0,"bank");Synth synth=new Synth();synth.setLayer(0,font,0);
  if(nativeEngine)synth.backend(1);long start=System.nanoTime();synth.start();
  Thread.sleep(100);check(synth.isRunning()&&!synth.audioReady(),"opening is not ready for recording");
  AudioStartGate gate=new AudioStartGate(0);int state=0;
  while(state==0){long now=(System.nanoTime()-start)/1000000;state=gate.observe(now,synth.isRunning(),synth.audioReady(),synth.sampleRate(),synth.activeBackend());if(state==0)Thread.sleep(25);}
  check(state==AudioStartGate.READY,"format settled");check((System.nanoTime()-start)/1000000>=450,"waited for actual stream + stable format");
  check(synth.sampleRate()==(nativeEngine?48000:44100),"actual format rather than old default");
  WavRecorder recorder=new WavRecorder();File wave=File.createTempFile("piano-start", ".wav");
  recorder.start(wave,synth.sampleRate());synth.midi(0x90,60,100);Thread.sleep(150);recorder.finish();
  check(recorder.error.isEmpty()&&recorder.bytes>4096,"recording survives delayed stream startup");
  byte[] pcm=java.nio.file.Files.readAllBytes(wave.toPath());boolean sound=false;for(int i=44;i<pcm.length;i++)sound|=pcm[i]!=0;check(sound,"recorded PCM is audible");
  synth.stop();check(!synth.audioReady(),"stopped route cannot be considered ready");synth.close();Synth.freeBank(font);wave.delete();
  System.out.println("PASS: actual Synth delayed "+(nativeEngine?"AAudio 48kHz":"AudioTrack 44.1kHz")+" startup; no premature recording; audible WAV; stop clears readiness. Simulated host route.");
 }
}
