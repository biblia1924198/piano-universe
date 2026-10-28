package com.luci.pianoroom;
import java.io.File;
/** Library policy is independent of scene settings and source sample data. */
public final class LibraryPedalSettings {
 public boolean pedalCustom,keyCustom;public int pedalMs=500,keyMs=350;
 public static String key(File bank){return "libraryPedal.v1."+bank.getName();}
 public String encode(){return (pedalCustom?1:0)+","+pedalMs+","+(keyCustom?1:0)+","+keyMs;}
 public static LibraryPedalSettings decode(String value){LibraryPedalSettings p=new LibraryPedalSettings();try{String[] v=value.split(",",-1);if(v.length!=4)throw new IllegalArgumentException();int a=Integer.parseInt(v[0]),b=Integer.parseInt(v[1]),c=Integer.parseInt(v[2]),d=Integer.parseInt(v[3]);if(a<0||a>1||c<0||c>1||b<20||b>8000||d<20||d>8000)throw new IllegalArgumentException();p.pedalCustom=a==1;p.pedalMs=b;p.keyCustom=c==1;p.keyMs=d;}catch(RuntimeException ignored){}return p;}
}
