package com.luci.pianoroom;
import java.util.Locale;
/** Histogram percentiles are conservative upper bucket edges, not exact timings. */
public final class PerformanceReport {
 public static final String PARALLEL_HEADER="workers;lotes_paralelos;espera_max_ns;espera_total_ns;tarefas_worker1;trabalho_worker1_ns;nice_worker1;tarefas_worker2;trabalho_worker2_ns;nice_worker2";
 public static String parallelRow(long[] values){StringBuilder b=new StringBuilder();for(int i=0;i<10;i++){if(i>0)b.append(';');b.append(values!=null&&i<values.length?values[i]:0);}return b.toString();}

 public static final String HEADER="callbacks;frames_min;frames_max;prazo_p50_pct_limite;prazo_p95_pct_limite;prazo_p99_pct_limite;prazo_max_pct_sessao;cpu_pct_amostrada;pagefault_minor_total;pagefault_major_total;adpf;adpf_reports";
 private static long delta(long[] a,long[] b,int n){return Math.max(0,a[n]-b[n]);}
 static String percentile(long[] now,long[] before,int pct){
  long total=0;for(int i=16;i<33;i++)total+=delta(now,before,i);if(total==0)return "NA";
  long target=(total*pct+99)/100,sum=0;for(int i=0;i<17;i++){sum+=delta(now,before,16+i);if(sum>=target)return i==16?">=400":Integer.toString((i+1)*25);}return "NA";
 }
 public static String row(long[] now,long[] before){
  if(now==null||now.length<33||before==null||before.length<33||now[1]<before[1])return "NA;NA;NA;NA;NA;NA;NA;NA;NA;NA;NA;NA";
  long period=delta(now,before,8);String cpu=period>0?String.format(Locale.US,"%.2f",100.0*delta(now,before,7)/period):"NA";
  return delta(now,before,1)+";"+now[4]+";"+now[5]+";"+percentile(now,before,50)+";"+percentile(now,before,95)+";"+percentile(now,before,99)+";"+String.format(Locale.US,"%.1f",now[6]/10.0)+";"+cpu+";"+now[10]+";"+now[11]+";"+now[12]+";"+now[13];
 }
 public static final String LIMITS="Percentis em faixas de 25 pontos percentuais do prazo real de cada callback; >=400 indica faixa aberta. Min/max frames e pico de prazo são acumulados da sessão. CPU do thread amostrada 1/32 callbacks; não é CPU total do aparelho. Page faults referem-se ao processo inteiro. ADPF: 2 ativo, 1 aguardando, 0 parado, -1 indisponível, -2 recusado/erro. Sessão aceita não comprova aumento de frequência. Memória/PSS coletada fora do callback a cada 15 s ou mudança de etapa.";
}
