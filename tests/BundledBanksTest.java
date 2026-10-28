package com.luci.pianoroom;
import java.io.*;import java.nio.file.*;import java.security.*;import java.util.*;
public final class BundledBanksTest {
 static void check(boolean yes,String text){if(!yes)throw new AssertionError(text);}
 static byte[] sha(File file)throws Exception{MessageDigest h=MessageDigest.getInstance("SHA-256");try(InputStream in=new FileInputStream(file)){byte[] b=new byte[65536];int n;while((n=in.read(b))!=-1)h.update(b,0,n);}return h.digest();}
 public static void main(String[]args)throws Exception{
  File assets=new File(args[0]),cache=new File(args[1]);cache.mkdirs();int[] opened={0};
  BundledBanks.Source source=name->{opened[0]++;return new FileInputStream(new File(assets,name));};
  for(String name:new String[]{"GeneralUser-GS.sf2","FluidR3-GM.sf2","YDP-GrandPiano.sf2"}){
   File original=new File(assets,name),restored=BundledBanks.ensure(name,cache,source);check(restored.length()==original.length()&&Arrays.equals(sha(original),sha(restored)),"Every original byte restored: "+name);
  }
  check(opened[0]==3,"all three packed banks used");for(String name:new String[]{"GeneralUser-GS.sf2","FluidR3-GM.sf2","YDP-GrandPiano.sf2"})BundledBanks.ensure(name,cache,source);check(opened[0]==3,"valid cached banks reused without decoding");
  File gs=new File(cache,"GeneralUser-GS.sf2"),original=new File(assets,gs.getName());try(RandomAccessFile f=new RandomAccessFile(gs,"rw")){f.seek(4096);int old=f.read();f.seek(4096);f.write(old^127);}
  BundledBanks.ensure(gs.getName(),cache,source);check(opened[0]==4&&Arrays.equals(sha(gs),sha(original)),"same-length corrupt cached bank repaired");
  byte[] good=sha(gs);long length=gs.length();byte[] header=new byte[80];try(InputStream in=new FileInputStream(new File(assets,gs.getName()+".prsf"))){check(in.read(header)==80,"header");}
  header[0]=0;try{BundledBanks.restore(new ByteArrayInputStream(header),gs,length,good);throw new AssertionError("bad header accepted");}catch(IOException expected){}check(Arrays.equals(sha(gs),good),"failed restore preserves previous bank");
  try(InputStream input=new FileInputStream(new File(assets,gs.getName()+".prsf"))){try{BundledBanks.restore(new FilterInputStream(input){long left=65536;public int read(byte[] b,int at,int size)throws IOException{if(left==0)return -1;int n=super.read(b,at,(int)Math.min(size,left));if(n>0)left-=n;return n;}public int read()throws IOException{if(left==0)return -1;int n=super.read();if(n>=0)left--;return n;}},gs,length,good);throw new AssertionError("truncated FLAC accepted");}catch(IOException expected){}}
  check(Arrays.equals(sha(gs),good),"truncation cannot replace original");File[] temps=cache.listFiles((d,n)->n.startsWith("builtin-")&&n.endsWith(".tmp"));check(temps!=null&&temps.length==0,"failed restores leave no staging file");
  Files.write(gs.toPath(),new byte[]{1,2,3});BundledBanks.ensure(gs.getName(),cache,source);check(Arrays.equals(sha(gs),good),"interrupted old direct copy repaired");
  System.out.println("PASS: all three real banks restore byte-for-byte; cached update preserves files; same-size corruption and partial old copy repaired; bad/truncated packs preserve previous data; staging cleaned.");
 }
}
