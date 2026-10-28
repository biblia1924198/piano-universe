package com.luci.pianoroom;
/** Hardware volume remains separate from sampled audio and the app master. */
public final class YamahaMixer {
 private YamahaMixer(){}
 public static final int CHANNEL=4;
 public static int effective(int desired,int solo){int v=Math.max(0,Math.min(127,desired));return solo>=0&&solo<CHANNEL?0:v;}
 public static int fromPercent(int percent){return Math.round(Math.max(0,Math.min(100,percent))*127f/100f);}
 public static int percent(int volume){return Math.round(Math.max(0,Math.min(127,volume))*100f/127f);}
}
