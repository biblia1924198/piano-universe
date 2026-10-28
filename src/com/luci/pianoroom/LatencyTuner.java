package com.luci.pianoroom;

/** User plays continuously; no inference about physical key-to-speaker latency. */
public final class LatencyTuner {
 public boolean active;public int target,best,step=4,minimum=8;public String status="";
 private int count,previousAccepted;private long since;
 public void start(int startMs,int underruns,int accepted,long now){active=true;target=Math.max(minimum,Math.min(120,startMs));best=target;count=underruns;previousAccepted=accepted;since=now;status="Toque acordes e use o pedal durante o teste.";}
 /** Return a new requested buffer, or -1 if unchanged. */
 public int observe(int underruns,int accepted,long now){
  if(!active)return -1;
  if(now-since<750)return -1;
  if(underruns>count){active=false;target=Math.min(120,best+Math.max(4,step));status="Houve falta de áudio. Salvo buffer de "+target+" ms, com margem de estabilidade.";return target;}
  if(now-since<6000)return -1;
  if(best!=target&&accepted>=previousAccepted-16){active=false;target=best;status="O Android não reduziu o buffer aceito. Mantido "+best+" ms.";return target;}
  best=target;previousAccepted=accepted;
  if(target<=minimum){active=false;status="Teste concluído em "+minimum+" ms de buffer. Confira a resposta ao tocar.";return target;}
  target=Math.max(minimum,target-Math.max(1,step));count=underruns;since=now;status="Testando "+target+" ms · continue tocando por 6 segundos.";return target;
 }
 public void cancel(){active=false;status="Teste cancelado; configuração anterior restaurada.";}
}
