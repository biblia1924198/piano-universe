package com.luci.pianoroom;
import java.io.*;
import java.nio.*;
import java.nio.file.*;
import java.util.*;
import java.util.zip.*;

/** Reproduce the reported opcode through the real compiler, catalog and JNI. */
public final class SfzVelocityControllerTest {
 private static void check(boolean ok,String why){SfzCompatibilityTest.check(ok,why);}
 public static void main(String[] args)throws Exception {
  File root=new File(args[0]);check(root.mkdirs(),"fresh fixture directory");
  SfzCompatibilityTest.wav(root,"note.wav",24,2,true);
  String source="#define $DYNAMICS 99\n<control> set_cc$DYNAMICS=63 label_cc$DYNAMICS=Intensidade\n<global> amp_veltrack=0 amp_veltrack_oncc$DYNAMICS=100 amp_veltrack_curvecc99=0 ampeg_release=0.3\n<region> sample=note.wav key=60 loop_start=0 loop_end=8191\n";
  ByteBuffer b=SfzCompatibilityTest.compile(root,source,"velocity.prb");
  check(b.getInt(0)==0x36425250&&b.getInt(4)==2,"PRB6 stereo regions");
  check(b.capacity()==204+2*672+8192*2*4,"bounded fixed controller metadata");
  check(b.get(76+99)==64&&b.getFloat(204+160+99*4)==100,"CC99 default and coefficient");
  check(b.getFloat(204+672+160+99*4)==100,"same map in both stereo halves");
  check(InstrumentIndex.read(new File(root,"velocity.prb"),"Dynamics",0).size()==1,"PRB6 catalog");
  long h=Synth.load(new File(root,"velocity.prb").getPath());check(h!=0,"actual JNI loader accepts PRB6");Synth.freeBank(h);
  for(String invalid:new String[]{"NaN","Infinity","100.01","-100.01"})SfzCompatibilityTest.fails(root,source.replace("oncc$DYNAMICS=100","oncc$DYNAMICS="+invalid),"amp_veltrack_oncc99");
  SfzCompatibilityTest.fails(root,source.replace("oncc$DYNAMICS=100","oncc128=100"),"0–127");
  SfzCompatibilityTest.fails(root,source.replace("oncc$DYNAMICS=100","onccX=100"),"0–127");
  SfzCompatibilityTest.fails(root,source.replace("curvecc99=0","curvecc99=255"),"Curva SFZ");
  SfzCompatibilityTest.fails(root,source.replace("<control>","<control> amp_veltrack_oncc99=100"),"<control>");
  String endpoints="<global> amp_veltrack=0 amp_veltrack_oncc0=25 amp_veltrack_oncc099=-50 amp_veltrack_oncc127=100\n<region> sample=note.wav key=60\n";
  b=SfzCompatibilityTest.compile(root,endpoints,"endpoints.prb");check(b.getFloat(204+160)==25&&b.getFloat(204+160+99*4)==-50&&b.getFloat(204+160+127*4)==100,"controller bounds and canonical leading zero");
  // Original values remain in inherited groups, overridden by individual zones.
  b=SfzCompatibilityTest.compile(root,"<global> amp_veltrack_oncc99=100\n<group> amp_veltrack_oncc99=50\n<region> sample=note.wav amp_veltrack_oncc99=25\n<region> sample=note.wav\n","inheritance.prb");
  check(b.getFloat(204+160+99*4)==25&&b.getFloat(204+2*672+160+99*4)==50,"SFZ inheritance");
  File sourceDir=new File(root,"collection");check(sourceDir.mkdir(),"collection");
  Files.copy(new File(root,"note.wav").toPath(),new File(sourceDir,"note.wav").toPath());
  Files.write(new File(sourceDir,"Piano-Referencia-fixture.sfz").toPath(),source.getBytes("UTF-8"));
  Files.write(new File(sourceDir,"Dynamics-2.sfz").toPath(),endpoints.getBytes("UTF-8"));
  File zip=new File(root,"received.zip");try(ZipOutputStream out=new ZipOutputStream(new FileOutputStream(zip))){for(File f:sourceDir.listFiles()){out.putNextEntry(new ZipEntry("Bank/"+f.getName()));Files.copy(f.toPath(),out);out.closeEntry();}}
  File dest=new File(root,"imported");check(dest.mkdir(),"destination");BankImportBatch.Report report=new BankImportBatch.Report();
  BankImportBatch.source(zip,new File(root,"stage"),dest,"received.zip",new BankImportBatch.Sink(){public void progress(String s){}public void accept(File bank,String title,String detail)throws IOException{check(InstrumentIndex.read(bank,title,0).size()==1,"batch catalog");long font=Synth.load(bank.getPath());check(font!=0,"batch JNI validation");Synth.freeBank(font);}},report);
  check(report.imported==2&&report.failed==0,"reported opcode works in a ZIP with multiple definitions: "+report.detail());
  // Reject malformed payload without native crashes or losing a valid import.
  byte[] valid=Files.readAllBytes(new File(root,"velocity.prb").toPath());
  ByteBuffer corrupt=ByteBuffer.wrap(valid.clone()).order(ByteOrder.LITTLE_ENDIAN);corrupt.putFloat(204+160+99*4,Float.NaN);File bad=new File(root,"corrupt.prb");Files.write(bad.toPath(),corrupt.array());check(Synth.load(bad.getPath())==0,"invalid coefficient rejected");
  Files.write(bad.toPath(),Arrays.copyOf(valid,valid.length-1));check(Synth.load(bad.getPath())==0,"truncated sample rejected");
  File big=new File(root,"large-index.prb");try(RandomAccessFile r=new RandomAccessFile(big,"rw")){r.writeBytes("PRB6");r.writeInt(Integer.reverseBytes(1));r.writeInt(Integer.reverseBytes(1073741825));r.write(new byte[192]);r.setLength(876L+1073741825L*4);}check(InstrumentIndex.read(big,"Large",0).size()==1,"64-bit PRB6 catalog without sample allocation");big.delete();
  System.out.println("PASS: amp_veltrack_oncc0–127, CC99 defaults/macros/inheritance, stereo PRB6, strict coefficient/curve bounds, actual ZIP multi-import -> catalog -> JNI, corrupt/truncated payload and 4GiB catalog.");
 }
}
