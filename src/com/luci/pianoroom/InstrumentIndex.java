package com.luci.pianoroom;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.text.Normalizer;
import java.util.*;

/** Metadata-only catalog. Never opens a native sampler or reads sample payloads. */
public final class InstrumentIndex {
 public static final class Entry {
  public final int source,preset,bank,program,family,art;
  public final String key,name,original,sourceName,search;
  Entry(int source,int preset,int bank,int program,String file,String sourceName,String original){
   this.source=source;this.preset=preset;this.bank=bank;this.program=program;this.sourceName=sourceName;this.original=original;
   boolean packed=file.endsWith(".prb"),ydp=file.equals("YDP-GrandPiano.sf2");
   boolean factory=file.equals("GeneralUser-GS.sf2")||file.equals("FluidR3-GM.sf2");
   name=packed?sourceName:factory||ydp?InstrumentCatalog.name(original,bank,program,ydp):original;
   family=InstrumentCatalog.family(bank,program);art=InstrumentCatalog.art(bank,program,ydp);
   // Filename is persistent across app updates; bank/preset distinguishes variants.
   key=file+"/"+bank+"/"+program+"/"+preset;
   search=normalize(name+" "+original+" "+sourceName+" "+InstrumentCatalog.FAMILIES[family]+" "+InstrumentCatalog.name(original,bank,program,ydp));
  }
  public boolean matches(String query,int source,int family,boolean onlyFavorites,Set<String> favorites){
   return matchesTerms(normalize(query).split("\\s+"),source,family,onlyFavorites,favorites);
  }
  public boolean matchesTerms(String[] terms,int source,int family,boolean onlyFavorites,Set<String> favorites){
   if(source>=0&&this.source!=source||family>=0&&this.family!=family||onlyFavorites&&!favorites.contains(key))return false;
   for(String term:terms)if(!search.contains(term))return false;return true;
  }
 }
 public static String normalize(String s){return Normalizer.normalize(s,Normalizer.Form.NFD).replaceAll("\\p{M}+","").toLowerCase(Locale.ROOT).trim();}
 private static long uint(RandomAccessFile r)throws IOException{return Integer.toUnsignedLong(Integer.reverseBytes(r.readInt()));}
 private static String label(byte[] b,int max){int n=0;while(n<max&&b[n]!=0)n++;return new String(b,0,n,StandardCharsets.ISO_8859_1).trim();}
 public static List<Entry> read(File file,String name,int source)throws IOException {return read(file,name,source,name);}
 public static List<Entry> read(File file,String name,int source,String classification)throws IOException {
  try(RandomAccessFile r=new RandomAccessFile(file,"r")){
   int magic=r.readInt();
   if(magic==0x50524231||magic==0x50524232||magic==0x50524233||magic==0x50524234||magic==0x50524235||magic==0x50524236||magic==0x50524237){long regions=uint(r),samples=uint(r);byte[] title=new byte[64];r.readFully(title);
    if(regions<1||regions>32000||samples<4||samples>(magic>=0x50524233?SfzBank.MAX_SAMPLES:268435456L)||r.length()<(magic==0x50524237?131276:magic>=0x50524234?204:76)+regions*(magic==0x50524237?196:magic==0x50524236?672:magic==0x50524235?160:magic>=0x50524232?120:96)+samples*4)throw new IOException("Índice SFZ incompleto");
    if(magic>=0x50524234)for(int cc=0;cc<128;cc++)if(r.readUnsignedByte()>128)throw new IOException("Controlador SFZ inválido no índice");
    return Collections.singletonList(new Entry(source,0,0,InstrumentCatalog.guessProgram(classification),file.getName(),name,label(title,19)));
   }
   if(magic!=0x52494646)throw new IOException("Índice não é SF2/SFZ");long size=uint(r),end=8+size;
   if(size<4||end>r.length()||r.readInt()!=0x7366626b)throw new IOException("Cabeçalho SF2 inválido");
   while(r.getFilePointer()+8<=end){int id=r.readInt();long length=uint(r),start=r.getFilePointer(),next=start+length+(length&1);
    if(next>end)throw new IOException("Chunk SF2 truncado");
    if(id==0x4c495354&&length>=4&&r.readInt()==0x70647461){
     long pdtaEnd=start+length;
     while(r.getFilePointer()+8<=pdtaEnd){int chunk=r.readInt();long n=uint(r),at=r.getFilePointer(),after=at+n+(n&1);
      if(after>pdtaEnd)throw new IOException("Presets SF2 truncados");
      if(chunk==0x70686472){
       if(n<76||n%38!=0||n/38>65537)throw new IOException("Tabela de presets inválida");
       ArrayList<Entry> raw=new ArrayList<>();byte[] b=new byte[20];
       for(int i=0;i<n/38-1;i++){r.readFully(b);int program=Short.toUnsignedInt(Short.reverseBytes(r.readShort())),bank=Short.toUnsignedInt(Short.reverseBytes(r.readShort()));r.skipBytes(14);raw.add(new Entry(source,i,bank,program,file.getName(),name,label(b,19)));}
       // TinySoundFont uses stable bank/program order, NOT physical phdr order.
       Collections.sort(raw,Comparator.comparingInt((Entry e)->e.bank).thenComparingInt(e->e.program));
       ArrayList<Entry> sorted=new ArrayList<>();for(int i=0;i<raw.size();i++){Entry e=raw.get(i);sorted.add(new Entry(source,i,e.bank,e.program,file.getName(),name,e.original));}return Collections.unmodifiableList(sorted);
      }r.seek(after);
     }
    }r.seek(next);
   }throw new IOException("Banco sem tabela de presets");
  }
 }
}
