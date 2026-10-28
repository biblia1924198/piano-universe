package com.luci.pianoroom;
/** Camera frame durations and codec limits are checked before a quality option is offered. */
public final class VideoModePolicy {
 private VideoModePolicy(){}
 public static boolean frameRate(int fps,long videoMinNs,long previewMinNs,int lower,int upper){return fps>0&&fps<=60&&lower<=fps&&upper==fps&&Math.max(videoMinNs,previewMinNs)<=1_000_000_000.0/fps+1000;}
 public static int orientation(int sensor,int displayDegrees,boolean front){return Math.floorMod(sensor+(front?displayDegrees:-displayDegrees),360);}
 public static int relativeRotation(int sensor,int displayDegrees,boolean front){return Math.floorMod(sensor-displayDegrees*(front?1:-1),360);}
 public static float[] previewScale(int width,int height,int bufferWidth,int bufferHeight,int sensor,int displayDegrees){boolean naturalSwap=Math.floorMod(sensor,180)!=0,displaySwap=Math.floorMod(displayDegrees,180)!=0;float nw=naturalSwap?bufferHeight:bufferWidth,nh=naturalSwap?bufferWidth:bufferHeight;float rw=displaySwap?nh:nw,rh=displaySwap?nw:nh;float scale=Math.max(width/rw,height/rh);return new float[]{scale*nw/Math.max(1,width),scale*nh/Math.max(1,height)};}
 public static int bitrate(int width,int height,int fps,int min,int max){long desired=Math.round(width*(double)height*fps*.12);return (int)Math.max(min,Math.min(max,Math.max(2_000_000,Math.min(80_000_000,desired))));}
 public static String label(int width,int height,int fps,boolean fixed){int h=Math.min(width,height);String quality=h>=2160?"4K":h>=1440?"QHD":h>=1080?"Full HD":h>=720?"HD":height+"p";return quality+" · "+width+" × "+height+" · "+(fixed?"":"até ")+fps+" FPS";}
}
