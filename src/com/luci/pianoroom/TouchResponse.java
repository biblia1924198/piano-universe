package com.luci.pianoroom;

/** Immutable lookup: no allocations or power operations in the MIDI callback. */
public final class TouchResponse {
 public static final String[] NAMES={"Linear · mantém o toque recebido","Leve · notas fortes com menos esforço","Firme · mais espaço para tocar suave","Personalizada","Fixa · mesma intensidade"};
 private final int[] values=new int[128];
 public final int mode,low,high,fixed; public final float gamma;
 public TouchResponse(int mode,float gamma,int low,int high,int fixed){
  this.mode=Math.max(0,Math.min(4,mode));this.gamma=Math.max(.4f,Math.min(2.5f,gamma));
  this.low=Math.max(1,Math.min(126,low));this.high=Math.max(this.low+1,Math.min(127,high));this.fixed=Math.max(1,Math.min(127,fixed));
  double power=this.mode==1?.7:this.mode==2?1.45:this.mode==3?this.gamma:1;
  for(int v=1;v<=127;v++){double x=Math.max(0,Math.min(1,(v-this.low)/(double)(this.high-this.low)));values[v]=this.mode==4?this.fixed:1+(int)Math.round(126*Math.pow(x,power));}
 }
 public int map(int velocity){return values[Math.max(0,Math.min(127,velocity))];}
}
