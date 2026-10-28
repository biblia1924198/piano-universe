package com.luci.pianoroom;
import java.util.*;
/** Wall-clock diagnostic. No catch-up bursts; all sampling stays off the audio callback. */
public final class DiagnosticBenchmark extends PianoBenchmark {
 public interface DiagnosticDriver extends Driver {
  void phase(int index); String details(); void checkpoint(String report);
  default int voices(){return metrics().voices;}
 }
 private static final String[] NAMES={"Canal 1 sem efeitos", "Canal 2 sem efeitos", "Canal 3 sem efeitos", "Três canais sem efeitos", "Três canais com chorus 15%", "Três canais com seus efeitos", "Pausa de cinco minutos", "Retomada com seus efeitos"};
 private static final long[] LENGTHS={30000,30000,30000,60000,60000,120000,300000,30000};
 private final DiagnosticDriver d; private final String context; private final int buffer;
 private final StringBuilder timeline=new StringBuilder(), summaries=new StringBuilder(),performance=new StringBuilder();
 private final int[] held=new int[128]; private int phase,step,notes,samples,rows;
 private long began,phaseBegan,nextNote,nextSample,lastTick,lastSave,maxGap,skipped;
 private Metrics baseline,last; private int maxVoices,backendChanges,resets; private double maxProcess,maxPeak,maxLimited;
 private String outcome="Em andamento"; private boolean pedalDown,phaseClosed;
 public DiagnosticBenchmark(DiagnosticDriver d,int buffer,String context){super(d,new int[]{buffer},context);this.d=d;this.buffer=buffer;this.context=context;}
 @Override public void start(long now){active=true;waiting=false;recommended=-1;began=lastTick=lastSave=now;d.buffer(buffer);begin(now);}
 private void begin(long now){
  phaseClosed=false;waiting=false;phaseBegan=now;step=notes=samples=maxVoices=0;maxGap=skipped=0;maxProcess=maxPeak=maxLimited=0;
  d.phase(phase);baseline=d.metrics().copy();last=baseline;nextSample=now;nextNote=now+1000;
  summaries.append("\nINÍCIO ").append(phase+1).append(" — ").append(NAMES[phase]).append("\n").append(d.details()).append('\n');
  status=NAMES[phase]+" · preparando…";
 }
 private void release(){for(int k=0;k<128;k++)while(held[k]>0){d.midi(0x80,k,0);held[k]--;}d.midi(0xB0,64,0);pedalDown=false;}
 private int delta(int a,int b){return Math.max(0,a-b);}
 private void sample(long now){
  Metrics m=d.metrics(); samples++;maxVoices=Math.max(maxVoices,m.voices);maxProcess=Math.max(maxProcess,m.processMs);maxPeak=Math.max(maxPeak,m.peak);maxLimited=Math.max(maxLimited,m.limited);
  if(m.nativeAudio!=last.nativeAudio){backendChanges++;summaries.append("MOTOR MUDOU aos ").append(now-began).append(" ms\n");}
  if(m.xruns<last.xruns||m.conflicts<last.conflicts||m.late<last.late||m.midiOverflow<last.midiOverflow){resets++;summaries.append("CONTADOR REINICIOU aos ").append(now-began).append(" ms; deltas da etapa são parciais.\n");}
  if(rows++<7200)timeline.append(String.format(Locale.US,"%d;%d;%s;%d;%s;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%.5f;%.3f;%.0f;%d;%s\n",now-began,phase+1,m.engine,m.rate,m.ready,m.bufferMs,m.processMs,m.sampleMs,m.effectsMs,m.midiMs,m.inputMs,m.channelVoices[0],m.channelVoices[1],m.channelVoices[2],m.channelVoices[3],m.xruns,m.conflicts,m.late,m.hard,m.soft,m.midiPending,m.midiOverflow,m.peak,m.limited,m.invalid,maxGap,pedalDown));
  performance.append(now-began).append(";").append(phase+1).append(";").append(PerformanceReport.row(m.perf,last.perf)+";"+PerformanceReport.parallelRow(m.parallel)).append("\n");
  last=m;
 }
 private void closePhase(long now,String rating){
  if(phaseClosed)return;sample(now);phaseClosed=true;
  summaries.append(String.format(Locale.US,"FIM etapa %d: duração real %d ms; notas emitidas %d; amostragens %d; vozes máx %d; processo máx observado %.3f ms; pico %.5f; limitadas máx %.3f%%; falhas +%d; conflitos +%d; prazo +%d; excesso MIDI +%d; lacuna do controlador %d ms; passos não emitidos %d; ouvido: %s\n",phase+1,now-phaseBegan,notes,samples,maxVoices,maxProcess,maxPeak,maxLimited,delta(last.xruns,baseline.xruns),delta(last.conflicts,baseline.conflicts),delta(last.late,baseline.late),delta(last.midiOverflow,baseline.midiOverflow),maxGap,skipped,rating));
 }
 @Override public void tick(long now){
  if(!active)return;maxGap=Math.max(maxGap,Math.max(0,now-lastTick));lastTick=now;
  if(waiting)return;
  long elapsed=now-phaseBegan;
  if(elapsed>=LENGTHS[phase]){release();closePhase(now,"aguardando avaliação");if(phase==6){phase++;begin(now);}else{waiting=true;status=NAMES[phase]+" concluído. Avalie o som para continuar.";}d.checkpoint(report());return;}
  if(phase!=6&&elapsed>=LENGTHS[phase]-5000){if(pedalDown)release();}
  else if(phase!=6&&now>=nextNote){
   skipped+=Math.max(0,(now-nextNote)/125);nextNote=now+125;
   if(!pedalDown){d.midi(0xB0,64,127);pedalDown=true;}
   // Each key is released while CC64 stays down: repeated notes accumulate naturally.
   for(int k=0;k<128;k++)while(held[k]>0){d.midi(0x80,k,0);held[k]--;}
   if((step++&1)==0){int[] chord={48,55,60,64,67,72};for(int k:chord){int key=k+(step/16)%3;d.midi(0x90,key,40+(step*17)%80);held[key]++;notes++;}}
  }
  if(now>=nextSample){sample(now);nextSample=now+1000;}
  status=(phase+1)+"/8 · "+NAMES[phase]+" · "+elapsed/1000+"/"+LENGTHS[phase]/1000+" s\nVozes "+last.voices+" · falhas "+last.xruns+" · conflitos "+last.conflicts+(phase==6?"\nNão toque. Pode apagar a tela; a retomada será automática.":"\nPedal automático. Não toque durante esta etapa.");
  if(now-lastSave>=15000){lastSave=now;summaries.append("AMBIENTE aos ").append(now-began).append(" ms: ").append(d.details()).append("\n");d.checkpoint(report());}
 }
 @Override public void rate(int rating,long now){if(!active||!waiting||rating<0||rating>2)return;summaries.append("AVALIAÇÃO etapa ").append(phase+1).append(": ").append(new String[]{"Ótimo","Com estalos/ruído","Atraso perceptível"}[rating]).append('\n');if(++phase==NAMES.length){outcome="Concluído";active=waiting=false;status="Diagnóstico concluído. Ajustes restaurados; copie ou compartilhe o relatório.";d.finished(report(),-1);}else{lastTick=now;begin(now);}}
 @Override public void cancel(String reason){if(!active)return;release();closePhase(lastTick,"interrompida");outcome="Interrompido: "+reason;active=waiting=false;status=outcome+". Ajustes restaurados; relatório parcial disponível.";d.finished(report(),-1);}
 @Override public String report(){return "Piano Universe 0.27-preview1 / protocolo PR-DIAG-3\n"+context+"\nEstado: "+outcome+"\nBuffer fixo solicitado: "+buffer+" ms; nenhuma recomendação automática de buffer.\nDuração nominal 11 minutos + avaliações. Faders 28%, master limitado a 65%. Otimizações de cache/interpolação e ADPF ativas quando suportadas; consultar estado ADPF.\nNotas internas: não testa o cabo MIDI. Pausa mantém serviço/áudio ativos: não equivale a suspensão completa do Android.\nTrocas de motor: "+backendChanges+" / reinícios de contador: "+resets+"\n"+summaries+"\nSÉRIE TEMPORAL (1 leitura/s; lacunas preservadas; até 7200 linhas)\nms;etapa;motor;Hz;pronto;buffer_ms;processo_ms;amostras_ms;efeitos_ms;midi_ms;entrada_ms;vozes1;vozes2;vozes3;vozes4;xruns_total;conflitos_total;prazo_total;hard_total;soft_total;fila;overflow_total;pico_pre;limitadas_pct;invalidas_intervalo;maior_lacuna_ms;pedal\n"+timeline+"\nPRAZOS E CPU\nms;etapa;"+PerformanceReport.HEADER+";"+PerformanceReport.PARALLEL_HEADER+"\n"+performance+"\n"+PerformanceReport.LIMITS+"\nWorkers: tempos de trabalho são de relógio, não CPU total; incluem preempção. Espera máxima é cumulativa por sessão de saída. nice=0 indica prioridade normal; -16 indica pedido aceito. A CPU da tabela principal é apenas a thread do callback.\n"+"\nLimites: máximos da tabela de áudio são observados; percentis aproximados estão na tabela PRAZOS E CPU; não mede latência tecla→som. Etapas nativas amostradas 1/16 callbacks. Medidores podem perder transientes. Contadores absolutos podem reiniciar após troca de motor. Relatório parcial é salvo a cada 15 s e pode perder o último intervalo se o processo morrer.\nPara enviar: informe etapa/segundo do ruído, se apagou a tela/abriu outro app, rota/cabo usados e se o piano original continuou limpo.\n";}
}
