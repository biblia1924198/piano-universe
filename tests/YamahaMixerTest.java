package com.luci.pianoroom;
import java.io.IOException;
public final class YamahaMixerTest {
 static void check(boolean ok,String what){if(!ok)throw new AssertionError(what);}
 public static void main(String[] args)throws Exception{
  int desired=93;
  check(YamahaMixer.effective(desired,-1)==93,"normal mix retains desired Yamaha level");
  for(int app=0;app<4;app++)check(YamahaMixer.effective(desired,app)==0,"app solo mutes hardware");
  check(YamahaMixer.effective(desired,4)==93,"Yamaha solo retains desired level");
  check(YamahaMixer.effective(desired,-1)==93,"leaving solo restores volume");
  check(YamahaMixer.effective(0,4)==0,"solo does not unmute intentionally muted piano");
  check(YamahaMixer.fromPercent(0)==0&&YamahaMixer.fromPercent(100)==127&&YamahaMixer.fromPercent(200)==127,"hardware endpoints and cap");
  for(int p=0;p<=100;p++)check(Math.abs(YamahaMixer.percent(YamahaMixer.fromPercent(p))-p)<=1,"percent round trip");
  Scene s=new Scene();s.solo=4;s.pianoVolume=93;Scene restored=Scene.decode(s.encode());
  check(restored.solo==4&&restored.pianoVolume==93,"Yamaha solo scene persists");
  s.solo=5;try{Scene.decode(s.encode());throw new AssertionError("invalid channel accepted");}catch(IOException expected){}
  System.out.println("PASS Yamaha mixer: app/hardware solo, mute, volume restoration, 0-100% and scenes");
 }
}
