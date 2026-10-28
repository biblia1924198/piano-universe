package com.luci.pianoroom;
import java.io.*;import java.nio.file.*;import java.util.*;
/** Optional end-to-end regression using the user's unmodified ZIP, never bundled. */
public final class NeblinaImportTest {
 static void check(boolean value,String why){if(!value)throw new AssertionError(why);}
 public static void main(String[] args)throws Exception {
  File root=new File(args[0]);root.mkdirs();File zip=new File(root,"received.zip");Files.copy(new File(args[1]).toPath(),zip.toPath(),StandardCopyOption.REPLACE_EXISTING);
  BankImportBatch.Report report=new BankImportBatch.Report();final File[] accepted={null};
  BankImportBatch.source(zip,new File(root,"unpacked"),root,"05-Orgao-Neblina.zip",new BankImportBatch.Sink(){
   public void progress(String value){System.out.println(value);}
   public void accept(File bank,String title,String detail)throws IOException {check(title.equals("05-Orgao-Neblina"),"instrument named from definition");check(InstrumentIndex.read(bank,title,3).size()==1,"PRB5 catalog");long h=Synth.load(bank.getPath());check(h!=0,"actual JNI loader accepts whole bank");Synth.freeBank(h);accepted[0]=bank;}
  },report);
  check(report.imported==1&&report.failed==0,"actual ZIP imported: "+report.detail());check(accepted[0]!=null,"published bank");File out=new File(root,"neblina.prb");Files.move(accepted[0].toPath(),out.toPath(),StandardCopyOption.REPLACE_EXISTING);
  try(RandomAccessFile r=new RandomAccessFile(out,"r")){check(r.readInt()==0x50524235,"PRB5");check(Integer.reverseBytes(r.readInt())==150,"75 stereo zones preserved");check(Integer.reverseBytes(r.readInt())==57600000,"75 full stereo PCM24 waveforms");check(r.length()==204+150*160L+57600000*4L,"exact float payload size");r.seek(76+7);check(r.readUnsignedByte()==101,"default CC7=100");r.seek(76+11);check(r.readUnsignedByte()==128,"default expression=127");
   File wave=new File(root,"unpacked/Samples/N024-V1.wav");try(RandomAccessFile w=new RandomAccessFile(wave,"r")){for(int channel=0;channel<2;channel++)for(int frame:new int[]{0,1,17,1000,190001,383999}){w.seek(44+frame*6L+channel*3);int v=w.readUnsignedByte()|(w.readUnsignedByte()<<8)|(w.readUnsignedByte()<<16);if((v&0x800000)!=0)v|=0xff000000;r.seek(204+150*160L+(channel*384000L+frame)*4);float actual=Float.intBitsToFloat(Integer.reverseBytes(r.readInt()));check(actual==v/8388608f,"exact 24-bit preservation at "+frame+" channel "+channel);}}
  }
  System.out.println("PASS: original 163MB ZIP -> batch compiler -> JNI and catalog; 75 WAV PCM24 stereo / 150 regions, 3 velocity layers, exact selected sample values, defaults and PRB5 size. "+report.detail());
 }
}
