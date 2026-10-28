package com.luci.pianoroom;
import java.util.*;import java.io.*;
public final class SoundPresetsTest {
 static void check(boolean b,String s){if(!b)throw new AssertionError(s);}
 public static void main(String[]args)throws Exception{
  Scene s=new Scene();s.name="Minha mesa";s.banks[0]="Salamander.prb";s.gains[0]=1.25f;s.master=.74f;s.yamaha=true;s.pianoVolume=83;s.effects[0].pan=17;s.effects[0].high=2;
  for(int id=1;id<=5;id++){ChannelSettings before=s.effects[0];s.effects[0]=PianoSoundPresets.apply(before,id);Scene next=Scene.decode(s.encode());check(next.effects[0].hybrid&&next.effects[0].releaseSmooth&&next.effects[0].harmonics&&next.effects[0].repetition==35&&next.effects[0].soundPreset==id,"preset scene roundtrip");check(next.banks[0].equals("Salamander.prb")&&next.gains[0]==1.25f&&next.master==.74f&&next.yamaha&&next.pianoVolume==83&&next.effects[0].pan==17&&next.effects[0].high==2&&!next.effects[1].hybrid,"scope isolated");check(before!=next.effects[0]&&s.effects[0].copy().texture==s.effects[0].texture,"copy owned");}
  byte[] data=Base64.getDecoder().decode(s.encode());data=Arrays.copyOf(data,data.length-124);data[3]=5;Scene old=Scene.decode(Base64.getEncoder().encodeToString(data));check(!old.effects[0].hybrid&&!old.effects[0].releaseSmooth&&!old.effects[0].harmonics&&old.effects[0].repetition==0&&old.gains[0]==1.25f&&old.effects[0].bodyEnabled,"v5 upgrade bypass");
  s.effects[0].texture=101;try{Scene.decode(s.encode());throw new AssertionError("invalid texture");}catch(IOException expected){}
  float[] p=new float[792];for(int n=0;n<88;n++){p[n]=(float)(440*Math.pow(2,(n+21-69)/12.0));p[176+n]=.9f;for(int h=0;h<3;h++){p[264+n*3+h]=p[n]*(h+1)*(h==0?1:1.002f);p[528+n*3+h]=1f/(h+1);}}
  check(PianoProfile.decode(PianoProfile.encode(p)).length==792&&PianoProfile.covered(p)==88,"spectral profile cache v2");p[264]=Float.NaN;try{PianoProfile.encode(p);throw new AssertionError("corrupt spectral coefficient");}catch(IOException expected){}
  System.out.println("PASS: five sound recipes, isolated bank/fader/Yamaha/master/EQ, deep copies, scene v6/v5 migration, invalid settings, profile v2.");
 }
}
