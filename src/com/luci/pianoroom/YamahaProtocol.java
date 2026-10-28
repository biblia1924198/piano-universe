package com.luci.pianoroom;
import java.util.Arrays;
/** Original codec implementing the published CSP wire format; no upstream code. */
public final class YamahaProtocol {
 private static final byte[] PREFIX={(byte)0xF0,0x43,0x73,1,0x52,0x25,0x26};
 public static final int MODEL=0x0f011801, FIRMWARE=0x0f010b01, VOLUME=0x0c000001, REVERB=0x0c000401, VOICE=0x02000101;
 public static byte[] get(int property,int index){return message(1,0,property,index,null);}
 public static byte[] set(int property,int index,byte[] value){return message(1,1,property,index,value);}
 public static byte[] value(int n,int length){byte[] b=new byte[length];for(int i=length-1;i>=0;i--){b[i]=(byte)(n&127);n>>>=7;}return b;}
 private static byte[] message(int a,int b,int p,int index,byte[] v){if(index<0||index>127)throw new IllegalArgumentException("Index");int n=17+(v==null?0:2+v.length);byte[] m=new byte[n];System.arraycopy(PREFIX,0,m,0,7);m[7]=(byte)a;m[8]=(byte)b;for(int i=0;i<4;i++)m[9+i]=(byte)((p>>>(24-i*8))&127);m[13]=(byte)index;m[14]=1;if(v!=null){m[16]=(byte)(v.length>>>7);m[17]=(byte)(v.length&127);for(int i=0;i<v.length;i++){if((v[i]&128)!=0)throw new IllegalArgumentException("Non-MIDI data");m[18+i]=v[i];}}m[n-1]=(byte)0xF7;return m;}
 public static final class Reply{public final int property,index;public final byte[] data;public final boolean ack;Reply(int p,int i,byte[] d,boolean a){property=p;index=i;data=d;ack=a;}}
 public static Reply reply(byte[] m){if(m.length<19)return null;for(int i=0;i<7;i++)if(m[i]!=PREFIX[i])return null;if(m[7]!=0||(m[8]!=0&&m[8]!=1)||m[14]!=1||m[15]!=0||m[m.length-1]!=(byte)0xF7)return null;for(int i=1;i<m.length-1;i++)if((m[i]&128)!=0)return null;int at=m[8]==1?18:16;if(m.length<at+3)return null;if(m[8]==1&&(m[16]!=0||m[17]!=0))return null;int n=(m[at]<<7)|m[at+1];if(at+2+n!=m.length-1)return null;int p=0;for(int i=9;i<13;i++)p=(p<<8)|(m[i]&127);return new Reply(p,m[13]&127,Arrays.copyOfRange(m,at+2,at+2+n),m[8]==1);}
 /** Reply.data already excludes the two-byte length; decode each 7-bit group. */
 public static String text(byte[] v){StringBuilder s=new StringBuilder();for(int at=0;at<v.length;){int high=v[at++]&127;int count=Math.min(7,v.length-at);for(int i=0;i<count;i++){int c=(v[at+i]&127)|(((high>>(count-1-i))&1)<<7);if(c!=0)s.append((char)c);}at+=count;}return s.toString();}
 public static byte[] identity(){return new byte[]{(byte)0xF0,0x7E,0x7F,6,1,(byte)0xF7};}
 public static byte[] masterVolume(int v){int n=Math.max(0,Math.min(127,v))*129;return new byte[]{(byte)0xF0,0x7F,0x7F,4,1,(byte)(n&127),(byte)(n>>7),(byte)0xF7};}
}
