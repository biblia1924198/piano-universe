package com.luci.pianoroom;
import java.io.IOException;
import java.util.*;
/** Stable file identities survive display-name edits; selection indices are remapped once. */
public final class LibraryEdits {
 private LibraryEdits(){}
 public static String name(String raw)throws IOException {String n=raw==null?"":raw.trim();if(n.isEmpty()||n.length()>100||n.matches("(?s).*[\\p{Cntrl}].*"))throw new IOException("Use um nome de 1 a 100 caracteres, sem quebras de linha.");return n;}
 public static int[] mapping(int count,Set<Integer> removed){int[] map=new int[count];int next=0;for(int i=0;i<count;i++)map[i]=removed.contains(i)?-1:next++;return map;}
 public static int[] selections(int[] current,int[] mapping,int fallback){int[] out=current.clone();for(int i=0;i<out.length;i++){int n=current[i];out[i]=n>=0&&n<mapping.length&&mapping[n]>=0?mapping[n]:fallback;}return out;}
 public static boolean imported(String file){return file.startsWith("import-")&&(file.endsWith(".prb")||file.endsWith(".sf2"));}
 public static boolean deletable(String file){return imported(file)||file.equals("FluidR3-GM.sf2")||file.equals("YDP-GrandPiano.sf2");}
}
