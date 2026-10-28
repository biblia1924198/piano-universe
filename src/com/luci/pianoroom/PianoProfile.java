package com.luci.pianoroom;
import java.io.*;import java.util.Base64;
/** Versioned sample-derived profile; invalid cached data falls back to A440. */
public final class PianoProfile {
 public static String key(File file,int preset){return "pianoProfile-v2/"+file.getName()+"/"+file.length()+"/"+file.lastModified()+"/"+preset;}
 public static String legacyKey(File file,int preset){return key(file,preset).replace("-v2/","-v1/");}
 public static int covered(float[] p){if(p==null||(p.length!=264&&p.length!=792))return 0;int n=0;for(int i=0;i<88;i++)if(p[i]>0&&p[i+176]>=.5f)n++;return n;}
 public static String encode(float[] p)throws IOException{if(!valid(p))throw new IOException("Perfil inválido");ByteArrayOutputStream b=new ByteArrayOutputStream();try(DataOutputStream o=new DataOutputStream(b)){o.writeInt(p.length==792?2:1);for(float f:p)o.writeFloat(f);}return Base64.getEncoder().encodeToString(b.toByteArray());}
 public static float[] decode(String value){try{byte[] b=Base64.getDecoder().decode(value);if(b.length!=1060&&b.length!=3172)return null;DataInputStream i=new DataInputStream(new ByteArrayInputStream(b));int version=i.readInt(),count=version==1?264:version==2?792:0;if(count==0||b.length!=4+count*4)return null;float[] p=new float[count];for(int n=0;n<count;n++)p[n]=i.readFloat();return valid(p)?p:null;}catch(Exception e){return null;}}
 private static boolean valid(float[] p){if(p==null||(p.length!=264&&p.length!=792))return false;for(int n=0;n<88;n++){double hz=440*Math.pow(2,(n+21-69)/12.0);if(!Float.isFinite(p[n])||p[n]!=0&&(p[n]<hz*.97||p[n]>hz*1.03)||!Float.isFinite(p[n+88])||p[n+88]<0||p[n+88]>12||!Float.isFinite(p[n+176])||p[n+176]<0||p[n+176]>1)return false;}if(p.length==792)for(int n=0;n<264;n++){float f=p[264+n],amp=p[528+n];if(!Float.isFinite(f)||!Float.isFinite(amp)||amp<0||amp>1.5f||f<0||f>15000||f!=0&&(p[n/3]<=0||f<p[n/3]*(n%3+1)*.9995||f>p[n/3]*(n%3+1)*1.012))return false;}return true;}
}
