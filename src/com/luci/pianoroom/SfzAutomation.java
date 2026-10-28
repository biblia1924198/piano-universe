package com.luci.pianoroom;
import java.io.*;
import java.util.*;
/** PRB7: sparse controller mappings, immutable lookup curves and event conditions. */
final class SfzAutomation {
 static final String[] PREFIX={"amplitude","volume","pan","ampeg_release","amp_veltrack"};
 static String canonical(String op)throws IOException {
  // SFZ1 releaseccN is an alias of the SFZ2 spelling.
  if(op.startsWith("ampeg_releasecc"))op="ampeg_release_oncc"+op.substring(15);
  for(String p:PREFIX)for(String suffix:new String[]{"_oncc","_curvecc"})if(op.startsWith(p+suffix))return p+suffix+cc(op,p+suffix);
  for(String p:new String[]{"on_locc","on_hicc","locc","hicc"})if(op.startsWith(p))return p+cc(op,p);
  return null;
 }
 private static int cc(String op,String prefix)throws IOException {try{int n=Integer.parseInt(op.substring(prefix.length()));if(n<0||n>127)throw new NumberFormatException();return n;}catch(NumberFormatException e){throw new IOException("Controlador SFZ fora de 0–127: "+op);}}
 static int integer(Map<String,String> r,String op,int def,int lo,int hi)throws IOException {try{int n=r.containsKey(op)?Integer.parseInt(r.get(op)):def;if(n<lo||n>hi)throw new NumberFormatException();return n;}catch(NumberFormatException e){throw new IOException("Valor SFZ inválido: "+op);}}
 static float number(Map<String,String> r,String op,float def,float lo,float hi)throws IOException {try{float n=r.containsKey(op)?Float.parseFloat(r.get(op)):def;if(!Float.isFinite(n)||n<lo||n>hi)throw new NumberFormatException();return n;}catch(NumberFormatException e){throw new IOException("Valor SFZ inválido: "+op);}}
 static boolean needed(Map<String,String> r){for(String op:r.keySet()){
  if(op.equals("amplitude_oncc7")||op.equals("amplitude_oncc11"))try{if(Float.parseFloat(r.get(op))<0)return true;}catch(NumberFormatException ignored){}
  if(op.equals("group")||op.equals("off_by")||op.equals("polyphony")||op.equals("group_volume")||op.equals("sustain_cc")||op.equals("lorand")||op.equals("hirand")||op.startsWith("locc")||op.startsWith("hicc")||op.startsWith("on_locc")||op.startsWith("on_hicc"))return true;
  for(String p:PREFIX)if(op.startsWith(p+"_curvecc")){try{if(Integer.parseInt(r.get(op))!=0||!p.equals("amp_veltrack"))return true;}catch(NumberFormatException e){return true;}}
  for(String p:new String[]{"volume","pan","ampeg_release"})if(op.startsWith(p+"_oncc")&&!op.equals("ampeg_release_oncc64"))return true;
  if(op.startsWith("amplitude_oncc")&&!op.equals("amplitude_oncc7")&&!op.equals("amplitude_oncc11"))return true;
 }return false;}
 static float[][] curves(List<Map<String,String>> definitions)throws IOException {
  float[][] curves=new float[256][128];boolean[] seen=new boolean[256];
  for(int k=0;k<256;k++)for(int i=0;i<128;i++){float x=i/127f;curves[k][i]=k==1?2*x-1:k==2?1-x:k==3?1-2*x:k==4?x*x:k==5?(float)Math.sqrt(x):k==6?(float)Math.sqrt(1-x):x;}
  for(Map<String,String> def:definitions){int index=integer(def,"curve_index",-1,7,255);if(seen[index])throw new IOException("Curva SFZ duplicada: "+index);seen[index]=true;float[] c=curves[index];boolean[] points=new boolean[128];points[0]=points[127]=true;
   for(String key:def.keySet()){if(key.equals("curve_index"))continue;if(!key.matches("v[0-9]{3}"))throw new IOException("Opcode inválido em <curve>: "+key);int i=Integer.parseInt(key.substring(1));if(i>127)throw new IOException("Ponto SFZ fora de v000–v127");c[i]=number(def,key,0,-1,1);points[i]=true;}
   int left=0;for(int right=1;right<128;right++)if(points[right]){for(int i=left+1;i<right;i++)c[i]=c[left]+(c[right]-c[left])*(i-left)/(right-left);left=right;}
  }
  return curves;
 }
 static void validateCurves(Map<String,String> r,List<Map<String,String>> definitions)throws IOException {for(String op:r.keySet())if(op.contains("_curvecc")){int index=integer(r,op,0,0,255);if(index>6){boolean found=false;for(Map<String,String> def:definitions)if(integer(def,"curve_index",-1,7,255)==index)found=true;if(!found)throw new IOException("Curva SFZ não definida: "+op+"="+index);}}}
 private static void i(DataOutputStream out,int v)throws IOException{out.writeInt(Integer.reverseBytes(v));}
 private static void f(DataOutputStream out,float v)throws IOException{i(out,Float.floatToIntBits(v));}
 static boolean controllerTrigger(Map<String,String> r){for(String k:r.keySet())if(k.startsWith("on_locc")||k.startsWith("on_hicc"))return true;return false;}
 static void write(DataOutputStream out,Map<String,String> r,int stereoSide)throws IOException {
  List<int[]> mods=new ArrayList<>(),conditions=new ArrayList<>();List<Float> amounts=new ArrayList<>();
  for(int kind=0;kind<PREFIX.length;kind++)for(int cc=0;cc<128;cc++){String op=PREFIX[kind]+"_oncc"+cc;if(r.containsKey(op)){float max=kind==1?144:100;amounts.add(number(r,op,0,-max,max));mods.add(new int[]{kind+1,cc,integer(r,PREFIX[kind]+"_curvecc"+cc,0,0,255)});}}
  for(int trigger=0;trigger<2;trigger++)for(int cc=0;cc<128;cc++){String lo=(trigger==1?"on_":"")+"locc"+cc,hi=(trigger==1?"on_":"")+"hicc"+cc;if(r.containsKey(lo)||r.containsKey(hi)){int l=integer(r,lo,0,0,127),h=integer(r,hi,127,0,127);if(l>h)throw new IOException("Faixa de controlador SFZ invertida: "+lo);conditions.add(new int[]{cc,l,h,trigger});}}
  if(mods.size()>64||conditions.size()>32)throw new IOException("Região SFZ excede 64 modulações ou 32 condições");
  i(out,integer(r,"group",0,0,Integer.MAX_VALUE));i(out,integer(r,"off_by",0,0,Integer.MAX_VALUE));i(out,integer(r,"polyphony",0,0,64));i(out,integer(r,"sustain_cc",64,0,127));i(out,stereoSide);i(out,mods.size());i(out,conditions.size());
  float lo=number(r,"lorand",0,0,1),hi=number(r,"hirand",1,0,1);if(lo>hi)throw new IOException("Faixa aleatória SFZ invertida");f(out,lo);f(out,hi);
  for(int n=0;n<mods.size();n++){for(int v:mods.get(n))i(out,v);f(out,amounts.get(n));}for(int[] condition:conditions)for(int v:condition)i(out,v);
 }
}
