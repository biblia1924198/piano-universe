package com.luci.pianoroom;
import java.io.*;import java.util.*;
public final class InstrumentIndexTest {
 static void check(boolean b,String message){if(!b)throw new AssertionError(message);}
 static void le(RandomAccessFile r,int n)throws IOException{r.writeInt(Integer.reverseBytes(n));}
 public static void main(String[] args)throws Exception {
  int total=0;for(int n=0;n<args.length;n++){
   File file=new File(args[n]);List<InstrumentIndex.Entry> all=InstrumentIndex.read(file,"Banco "+n,n);
   long nativeBank=Synth.load(file.getAbsolutePath());check(nativeBank!=0,"native bank");String[] names=Synth.names(nativeBank);int[] info=Synth.presetInfo(nativeBank);
   check(names.length==all.size(),"same preset count");Set<String> keys=new HashSet<>();
   for(int i=0;i<names.length;i++){InstrumentIndex.Entry e=all.get(i);check(e.preset==i&&e.original.equals(names[i].trim())&&e.bank==info[i*2]&&(file.getName().endsWith(".prb")||e.program==info[i*2+1]),"exact native order and metadata "+file+" preset "+i+" "+e.original+" vs "+names[i]);check(keys.add(e.key),"unique favorite id");}
   total+=all.size();Synth.freeBank(nativeBank);
   List<InstrumentIndex.Entry> reopened=InstrumentIndex.read(file,"Renamed bank",42);check(reopened.get(0).key.equals(all.get(0).key),"favorite survives display name, restart and source reordering");
  }
  InstrumentIndex.Entry e=new InstrumentIndex.Entry(2,0,0,0,"import-123.sf2","São Paulo emocional","Órgão Suave");Set<String> favorites=new HashSet<>();check(e.name.equals("Órgão Suave"),"custom labels preserved");check(e.matches("orgao sao",-1,-1,false,favorites),"accent-insensitive multi-term search");check(!e.matches("orgao",-1,-1,true,favorites),"favorites only excludes ordinary entries");favorites.add(e.key);check(e.matches("orgao",2,0,true,favorites),"favorite + bank + family + query intersection");check(!e.matches("orgao",1,0,true,favorites),"bank filter");check(!e.matches("orgao",2,11,true,favorites),"family filter");favorites.remove(e.key);check(!e.matches("",-1,-1,true,favorites),"unfavorite");
  // Sparse 1-GiB sample payload: only headers may be read, independent of sample RAM.
  File sparse=File.createTempFile("catalog-sparse", ".sf2");try(RandomAccessFile r=new RandomAccessFile(sparse,"rw")){
   int skip=1024*1024*1024; r.writeBytes("RIFF");le(r,4+8+skip+12+8+38*3);r.writeBytes("sfbkJUNK");le(r,skip);r.seek(r.getFilePointer()+skip);r.writeBytes("LIST");le(r,4+8+38*3);r.writeBytes("pdtaphdr");le(r,38*3);
   // Deliberately unsorted: selection index must agree with TinySoundFont.
   for(int i=0;i<3;i++){byte[] title=new byte[20];byte[] t=(i==0?"Second":i==1?"First":"EOP").getBytes("US-ASCII");System.arraycopy(t,0,title,0,t.length);r.write(title);r.writeShort(Short.reverseBytes((short)(i==0?12:0)));r.writeShort(0);r.write(new byte[14]);}
  }
  List<InstrumentIndex.Entry> entries=InstrumentIndex.read(sparse,"Sparse",0);check(entries.get(0).original.equals("First")&&entries.get(1).original.equals("Second"),"stable native sorting");
  try(RandomAccessFile r=new RandomAccessFile(sparse,"rw")){r.setLength(15);}try{InstrumentIndex.read(sparse,"Broken",0);throw new AssertionError("truncated SF2 accepted");}catch(IOException expected){}sparse.delete();
  System.out.println("PASS: "+total+" native presets matched; SF2/PRB headers, sparse 1-GiB skipping, malformed data, stable favorites, imported names, combined global filters.");
 }
}
