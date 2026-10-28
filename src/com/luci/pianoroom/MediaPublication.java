package com.luci.pianoroom;
import java.io.*;
/** A pending MediaStore entry is published only after its entire file is closed. */
public final class MediaPublication {
 public interface Destination {OutputStream open()throws IOException;void publish()throws IOException;void discard();}
 private MediaPublication(){}
 public static void copy(File source,Destination destination)throws IOException {
  boolean complete=false;
  try {
   try(InputStream in=new FileInputStream(source);OutputStream out=destination.open()){
    if(out==null)throw new IOException("Destino indisponível");
    byte[] bytes=new byte[65536];int n;while((n=in.read(bytes))!=-1)out.write(bytes,0,n);
   }
   destination.publish();complete=true;
  }finally{if(!complete)destination.discard();}
 }
}
