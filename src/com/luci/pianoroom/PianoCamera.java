package com.luci.pianoroom;
/** Column-major perspective/view matrix and picking use the same transforms as GLSL. */
public final class PianoCamera {
 public static final float HINGE_Y=.244f,HINGE_Z=1.215f,ORIGINAL_ANGLE=.50119f;
 public static final class State {
  public final float yaw,pitch,distance,angle,scale;public final int width,height,selected;public final boolean interior;public final float[] matrix;
  public State(float yaw,float pitch,float distance,int width,int height,int lid,int size,int selected,boolean interior){
   this.yaw=Float.isFinite(yaw)?yaw:1.1f;this.pitch=clamp(pitch,.12f,1.25f);this.distance=clamp(distance,4.4f,10);this.width=Math.max(1,width);this.height=Math.max(1,height);this.angle=clamp(lid,0,100)/100f*1.04719755f-ORIGINAL_ANGLE;this.scale=1+(clamp(size,0,100)-50)*.0024f;this.selected=Math.max(0,Math.min(5,selected));this.interior=interior;matrix=matrix(this.yaw,this.pitch,this.distance,this.width/(float)this.height);
  }
 }
 public static float dragYaw(float yaw,float deltaDp){return yaw-deltaDp*.009f;}
 public static float dragPitch(float pitch,float deltaDp){return clamp(pitch+deltaDp*.006f,.12f,1.25f);}
 private static float clamp(float x,float lo,float hi){return !Float.isFinite(x)?(lo+hi)*.5f:Math.max(lo,Math.min(hi,x));}
 private static float[] matrix(float yaw,float pitch,float r,float aspect){
  float x=(float)(Math.sin(yaw)*Math.cos(pitch)),y=(float)Math.sin(pitch),z=(float)(Math.cos(yaw)*Math.cos(pitch));float length=(float)Math.sqrt(x*x+z*z),sx=z/length,sz=-x/length,ux=y*sz,uy=z*sx-x*sz,uz=-y*sx;
  float tx=-.8f,ty=-.05f,tz=0,ex=tx+x*r,ey=ty+y*r,ez=tz+z*r;
  float[] view={sx,ux,x,0,0,uy,y,0,sz,uz,z,0,-(sx*ex+sz*ez),-(ux*ex+uy*ey+uz*ez),-(x*ex+y*ey+z*ez),1};
  float f=(float)(1/Math.tan(Math.toRadians(38)/2)),near=.2f,far=30;float[] p={f/aspect,0,0,0,0,f,0,0,0,0,(far+near)/(near-far),-1,0,0,2*far*near/(near-far),0},out=new float[16];
  for(int col=0;col<4;col++)for(int row=0;row<4;row++)for(int k=0;k<4;k++)out[col*4+row]+=p[k*4+row]*view[col*4+k];return out;
 }
 public static boolean project(State s,float[] v,int offset,int tag,float[] out){
  float x=v[offset],y=v[offset+1],z=v[offset+2],co=(float)Math.cos(s.angle),si=(float)Math.sin(s.angle);
  if(tag==1){float dy=y-HINGE_Y,dz=z-HINGE_Z;y=HINGE_Y+co*dy-si*dz;z=HINGE_Z+si*dy+co*dz;}
  else if(tag==6){float weight=Math.max(0,Math.min(1,(y-.02371633f)/1.09925467f)),dy=1.122971f-HINGE_Y,dz=-.4336694f-HINGE_Z;y+=(HINGE_Y+co*dy-si*dz-1.122971f)*weight;z+=(HINGE_Z+si*dy+co*dz+.4336694f)*weight;}
  x*=s.scale;z*=s.scale;float[] m=s.matrix;float w=m[3]*x+m[7]*y+m[11]*z+m[15];if(w<=.2f)return false;out[0]=(m[0]*x+m[4]*y+m[8]*z+m[12])/w*s.width*.5f+s.width*.5f;out[1]=s.height*.5f-(m[1]*x+m[5]*y+m[9]*z+m[13])/w*s.height*.5f;out[2]=(m[2]*x+m[6]*y+m[10]*z+m[14])/w;return Float.isFinite(out[0])&&Float.isFinite(out[1])&&out[2]>=-1&&out[2]<=1;
 }
}
