package com.luci.pianoroom;
import java.io.*;import java.util.*;import java.util.zip.*;
/** A deliberately compressible ZIP64 fixture; does not claim a tested commercial bank. */
public class LargeSfzImportTest {
 static void check(boolean v){if(!v)throw new AssertionError();}
 static void le(RandomAccessFile r,int v)throws Exception{r.writeInt(Integer.reverseBytes(v));}
 public static void main(String[]args)throws Exception{
  File root=new File(args[0]);root.mkdirs();File zip=new File(args[1]);File source=SfzBank.unpack(new FileInputStream(zip),new File(root,"unpacked"));check(new File(source.getParentFile(),"padding.dat").length()>Integer.MAX_VALUE);File result=new File(root,"small.prb");SfzBank.compile(source,source.getParentFile(),result,"ZIP64 piano");check(result.length()>100);
  // PRB3 catalog consumes only metadata even when file payload crosses 4 GiB.
  File big=new File(root,"index-4GiB.prb");try(RandomAccessFile r=new RandomAccessFile(big,"rw")){r.writeBytes("PRB3");le(r,1);le(r,1073741825);r.write(new byte[64]);r.setLength(196L+1073741825L*4);}
  check(InstrumentIndex.read(big,"Grande",0).size()==1);big.delete();
  // Parse a WAV data chunk >2 GiB without reading/allocating the payload.
  File wave=new File(root,"large.wav");try(RandomAccessFile r=new RandomAccessFile(wave,"rw")){r.writeBytes("RIFF");le(r,(int)(2147483650L+36));r.writeBytes("WAVEfmt ");le(r,16);r.writeShort(Short.reverseBytes((short)1));r.writeShort(Short.reverseBytes((short)1));le(r,48000);le(r,96000);r.writeShort(Short.reverseBytes((short)2));r.writeShort(Short.reverseBytes((short)16));r.writeBytes("data");le(r,(int)2147483650L);r.setLength(44L+2147483650L);}
  Class<?> cls=Class.forName("com.luci.pianoroom.SfzBank$Wave");java.lang.reflect.Constructor<?> constructor=cls.getDeclaredConstructor(File.class);constructor.setAccessible(true);Object meta=constructor.newInstance(wave);java.lang.reflect.Field frames=cls.getDeclaredField("frames");frames.setAccessible(true);check(frames.getInt(meta)==1073741825);wave.delete();SfzBank.erase(root);
  System.out.println("PASS: real ZIP64 extraction beyond 2 GiB with bounded Java heap, valid SFZ compile, 4-GiB catalog metadata, and >2-GiB WAV header parsing without large arrays.");
 }
}
