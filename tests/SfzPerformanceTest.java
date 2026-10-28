package com.luci.pianoroom;
import java.io.*;import java.nio.*;import java.nio.file.*;
public final class SfzPerformanceTest {
 public static void main(String[] args)throws Exception {
  File root=new File(args[0]);root.mkdirs();SfzCompatibilityTest.wav(root,"loop.wav",24,2,true);
  String source="<control> set_cc7=100 set_cc11=127\n<global> sustain_sw=off sostenuto_sw=off ampeg_dynamic=1 ampeg_attack=0.45 ampeg_release=0.45 ampeg_release_oncc64=1.15 amp_veltrack=65 amplitude_oncc7=100 amplitude_oncc11=100 note_polyphony=2 off_mode=normal off_time=0.04\n<region> sample=loop.wav key=60 loop_start=0 loop_end=8191 tune=-2.475946\n";
  ByteBuffer b=SfzCompatibilityTest.compile(root,source,"performance.prb");SfzCompatibilityTest.check(b.getInt(0)==0x35425250,"PRB5");SfzCompatibilityTest.check(b.capacity()==204+320+8192*2*4,"extended metadata length");SfzCompatibilityTest.check(b.getInt(204+120)==31,"authored controls");SfzCompatibilityTest.check(Math.abs(b.getFloat(204+156)+.475946f)<.000001f,"fractional cents preserved");
  SfzCompatibilityTest.fails(root,source.replace("sustain_sw=off","sustain_sw=bad"),"sustain_sw");SfzCompatibilityTest.fails(root,source.replace("sostenuto_sw=off","sostenuto_sw=on"),"sostenuto_sw");SfzCompatibilityTest.fails(root,source.replace("ampeg_dynamic=1","ampeg_dynamic=2"),"ampeg_dynamic");SfzCompatibilityTest.fails(root,source.replace("note_polyphony=2","note_polyphony=2.5"),"note_polyphony");SfzCompatibilityTest.fails(root,source.replace("amplitude_oncc7=100","amplitude_oncc7=-100.01"),"amplitude_oncc7");SfzCompatibilityTest.fails(root,source.replace("off_mode=normal","off_mode=bad"),"off_mode");SfzCompatibilityTest.fails(root,source+" cutoff=5000","cutoff");
  File big=new File(root,"index-4GiB.prb");try(RandomAccessFile r=new RandomAccessFile(big,"rw")){r.writeBytes("PRB5");r.writeInt(Integer.reverseBytes(1));r.writeInt(Integer.reverseBytes(1073741825));r.write(new byte[192]);r.setLength(364L+1073741825L*4);}SfzCompatibilityTest.check(InstrumentIndex.read(big,"Large",0).size()==1,"64-bit PRB5 catalog offsets");big.delete();
  System.out.println("PASS: authored SFZ pedal/CC/polyphony fields, exact fractional tuning, invalid switches/ranges and unsupported opcodes rejected, PRB5 catalog above 4GiB with bounded heap.");
 }
}
