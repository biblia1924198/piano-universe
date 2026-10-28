package com.luci.pianoroom;
import java.io.*;import java.nio.*;
/** Small trusted mesh, parsed once outside audio. Original asset is CC-BY3.0. */
public final class PianoMesh {
 public static final String[] PARTS={"Corpo","Tampa","Tábua harmônica","Cordas","Teclas / martelos","Pedal"};
 public static final float[][] ANCHORS={{-.9f,-.22f,1.18f},{-1.1f,.976f,-.12f},{-1,.045f,.06f},{-.9f,.14f,-.32f},{.75f,.08f,0},{.65f,-1.1f,0}};
 public final float[] vertices;public final FloatBuffer buffer;public final int triangles;
 private PianoMesh(float[] v,int n){vertices=v;triangles=n;buffer=ByteBuffer.allocateDirect(v.length*4).order(ByteOrder.nativeOrder()).asFloatBuffer();buffer.put(v).position(0);}
 public static PianoMesh read(InputStream input)throws IOException{
  try(DataInputStream in=new DataInputStream(new BufferedInputStream(input))){if(in.readInt()!=0x50523344||Integer.reverseBytes(in.readInt())!=1)throw new IOException("Modelo 3D incompatível");int n=Integer.reverseBytes(in.readInt());if(n<1||n>4096)throw new IOException("Modelo 3D excede limite");float[] v=new float[n*30];for(int i=0;i<v.length;i++){float f=Float.intBitsToFloat(Integer.reverseBytes(in.readInt()));if(!Float.isFinite(f)||Math.abs(f)>10)throw new IOException("Geometria inválida");v[i]=f;}for(int i=9;i<v.length;i+=10)if(v[i]<0||v[i]>6||v[i]!=(int)v[i])throw new IOException("Parte inválida");if(in.read()!=-1)throw new IOException("Dados extras no modelo");return new PianoMesh(v,n);}
 }
 public int pick(PianoCamera.State s,float x,float y){
  float closest=Float.MAX_VALUE;int found=-1;float[] a=new float[3],b=new float[3],c=new float[3];
  for(int i=0;i<vertices.length;i+=30){int tag=(int)vertices[i+9];if(s.interior&&(tag==0||tag==1||tag==6))continue;if(!PianoCamera.project(s,vertices,i,tag,a)||!PianoCamera.project(s,vertices,i+10,tag,b)||!PianoCamera.project(s,vertices,i+20,tag,c))continue;
   float den=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1]);if(Math.abs(den)<.001f)continue;float u=((b[1]-c[1])*(x-c[0])+(c[0]-b[0])*(y-c[1]))/den,v=((c[1]-a[1])*(x-c[0])+(a[0]-c[0])*(y-c[1]))/den,w=1-u-v;if(u<0||v<0||w<0)continue;float z=u*a[2]+v*b[2]+w*c[2];if(z<closest){closest=z;found=tag==6?1:tag;}
  }return found;
 }
}
