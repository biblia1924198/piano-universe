package com.luci.pianoroom;
import java.util.Locale;
/** Bounded voice staircase. Stops a group after sustained overload, without
 * changing interpolation, reducing the synth voice limit, or concealing failures. */
public final class CapacityBenchmark extends PianoBenchmark {
 private static final int[] TARGETS={64,128,192,256,384,512};
 private final DiagnosticBenchmark.DiagnosticDriver d;private final int buffer;private final String context;
 private final StringBuilder log=new StringBuilder(),trace=new StringBuilder();private final int[] held=new int[128];
 private int group,level,step,notes,maxVoices,badIntervals,capacity;private long began,start,next,lastSample,lastSave,lastTick,maxGap;
 private Metrics baseline,previous;private boolean pedal,failed,closedStage,qualityFault,measuredStarted;private String outcome="Em andamento",verdict="";
 public CapacityBenchmark(DiagnosticBenchmark.DiagnosticDriver d,int buffer,String context){super(d,new int[]{buffer},context);this.d=d;this.buffer=buffer;this.context=context;}
 @Override public void start(long now){active=true;began=lastSave=lastTick=now;d.buffer(buffer);begin(now);}
 private String name(){return group<3?"Canal "+(group+1):"Três canais";}
 private void begin(long now){
  release();d.phase(group);baseline=d.metrics().copy();previous=baseline;start=now;lastTick=now;lastSample=now;next=now+1000;
  notes=step=maxVoices=badIntervals=0;maxGap=0;pedal=failed=waiting=closedStage=qualityFault=measuredStarted=false;verdict="";
  log.append("\nINÍCIO ").append(name()).append(" / alvo ").append(TARGETS[level]).append(" vozes / ").append(now-began).append(" ms\n");
  if(level==0)log.append(d.details()).append('\n');
 }
 private void off(){for(int k=0;k<128;k++)while(held[k]>0){d.midi(0x80,k,0);held[k]--;}}
 private void release(){off();if(pedal){d.midi(0xB0,64,0);pedal=false;}}
 private void sample(long now){
  Metrics m=d.metrics();maxVoices=Math.max(maxVoices,m.voices);
  qualityFault|=m.invalid>0||m.limited>0;
  boolean overloaded=m.xruns>previous.xruns||m.late>previous.late||m.conflicts>previous.conflicts||m.midiOverflow>previous.midiOverflow;
  if(now-start>=2000&&overloaded)badIntervals++;else badIntervals=0;
  if(m.nativeAudio!=baseline.nativeAudio||!m.ready){failed=true;verdict="motor mudou ou áudio indisponível";}
  if(badIntervals>=2){failed=true;verdict="sobrecarga em dois intervalos consecutivos";}
  trace.append(String.format(Locale.US,"%d;%d;%d;%d;%d;%d;%d;%d;%.3f;%.3f;%.3f;%d;",now-began,group+1,TARGETS[level],m.voices,m.xruns,m.late,m.conflicts,m.midiOverflow,m.processMs,m.sampleMs,m.effectsMs,maxGap)).append(PerformanceReport.row(m.perf,previous.perf)+";"+PerformanceReport.parallelRow(m.parallel)).append('\n');
  previous=m;lastSample=now;
 }
 private void finishStage(long now){
  if(closedStage)return;release();sample(now);closedStage=true;
  boolean measured=!qualityFault&&previous.xruns==baseline.xruns&&previous.late==baseline.late&&previous.conflicts==baseline.conflicts&&previous.midiOverflow==baseline.midiOverflow&&previous.nativeAudio==baseline.nativeAudio&&previous.ready;
  // Insufficient voices or delayed control cannot be advertised as stable capacity.
  boolean reached=maxVoices>=TARGETS[level]&&maxGap<=500;
  if(!failed&&measured&&reached)capacity=TARGETS[level];
  log.append(name()).append(" alvo ").append(TARGETS[level]).append(" / vozes observadas máx ").append(maxVoices).append(" / notas ").append(notes).append(" / duração ms ").append(now-start).append(" / ").append(failed?verdict:!reached?"alvo não demonstrado ou controle atrasado":measured?"sem falhas medidas neste degrau":"falhas medidas").append('\n');
  if(failed||!measured||level==TARGETS.length-1){waiting=true;status=name()+" concluído. Maior alvo sem falhas medidas: "+capacity+". Avalie o som.";d.checkpoint(report());}
  else{level++;begin(now);}
 }
 @Override public void tick(long now){
  if(!active||waiting)return;maxGap=Math.max(maxGap,Math.max(0,now-lastTick));lastTick=now;
  if(now-lastSample>=1000)sample(now);
  if(failed||now-start>=12000){finishStage(now);return;}
  if(now-start>=9000)release();
  else if(now>=next){
   next=now+125;
   if(!measuredStarted){Metrics warm=d.metrics().copy();log.append("Preparação: underruns +").append(Math.max(0,warm.xruns-baseline.xruns)).append(" / prazo +").append(Math.max(0,warm.late-baseline.late)).append("; contagem musical começa após preparação.\n");baseline=previous=warm;qualityFault=false;measuredStarted=true;}
   off();if(!pedal){d.midi(0xB0,64,127);pedal=true;}
   int live=d.voices();maxVoices=Math.max(maxVoices,live);
   if(live<TARGETS[level]){int count=Math.min(12,Math.max(1,(TARGETS[level]-live)/(group==3?6:2)));for(int i=0;i<count;i++){int key=48+(step++%32);d.midi(0x90,key,80+(step%4)*10);held[key]++;notes++;}}
  }
  status=name()+" · alvo "+TARGETS[level]+" vozes · "+(now-start)/1000+"/12 s\nVozes "+previous.voices+" · maior alvo sem falhas "+capacity+"\nNão toque durante a etapa automática.";
  if(now-lastSave>=15000){lastSave=now;log.append("AMBIENTE ").append(now-began).append(" ms: ").append(d.details()).append('\n');d.checkpoint(report());}
 }
 @Override public void rate(int rating,long now){
  if(!active||!waiting||rating<0||rating>2)return;
  log.append("AVALIAÇÃO ").append(name()).append(": ").append(new String[]{"Ótimo","Ruído/estalos","Atraso"}[rating]).append(" / maior alvo sem falhas medidas ").append(capacity).append(" (avaliação do grupo inteiro, não certificação por degrau)\n");
  if(++group>=4){active=waiting=false;outcome="Concluído";status="Capacidade concluída; ajustes restaurados. Compartilhe o relatório.";d.finished(report(),-1);}
  else{level=capacity=0;begin(now);}
 }
 @Override public void cancel(String reason){if(!active)return;release();if(!closedStage){sample(lastTick);log.append("Etapa parcial: ").append(name()).append(" alvo ").append(TARGETS[level]).append(" vozes observadas ").append(maxVoices).append('\n');}active=waiting=false;outcome="Interrompido: "+reason;status=outcome+". Ajustes restaurados.";d.finished(report(),-1);}
 @Override public String report(){return "Piano Universe 0.27-preview1 / PR-CAP-3\n"+context+"\nEstado: "+outcome+"\nBuffer fixo: "+buffer+" ms. Alvos de vozes reais 64/128/192/256/384/512; até 4 min 48 s + avaliações. Cada degrau: 1 s preparação, até 8 s emissão controlada, 3 s soltura. Interrompe grupo se sobrecarga persistente; não corta vozes para maquiar resultado. Timbres atuais, canais isolados e juntos, efeitos desligados. Samples/caches/ADPF otimizados; qualidade de interpolação atual preservada.\n"+log+"\nms;grupo;alvo;vozes;xruns_total;prazo_total;conflitos_total;overflow_total;processo_ms;amostras_ms;efeitos_ms;maior_lacuna_ms;"+PerformanceReport.HEADER+";"+PerformanceReport.PARALLEL_HEADER+"\n"+trace+"\n"+PerformanceReport.LIMITS+"\nWorkers: tempos de trabalho são de relógio, não CPU total; incluem preempção. Espera máxima é cumulativa por sessão de saída. nice=0 indica prioridade normal; -16 indica pedido aceito. A CPU da tabela principal é apenas a thread do callback.\n"+"\nNotas geradas no app: não testa cabo MIDI nem latência física. Capacidade medida por poucos segundos não certifica sessão longa; execute também diagnóstico de 11 minutos. Relatório parcial salvo a cada 15 s; nada enviado automaticamente.\n";}
}
