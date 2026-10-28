package com.luci.pianoroom;
/** Gives the fader the remaining viewport; small windows may scroll. */
public final class MixerLayout {
 private MixerLayout(){}
 public static int faderHeight(int viewportDp){return Math.max(120,Math.min(300,viewportDp-128));}
 public static int channelWidth(int viewportDp){return channelWidth(viewportDp,4);}
 public static int channelWidth(int viewportDp,int channels){return Math.max(132,(viewportDp-channels*6-Math.max(0,channels-1)*6-(channels==5?42:0))/Math.max(1,channels));}
}
