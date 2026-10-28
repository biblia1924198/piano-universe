package com.luci.pianoroom;
import android.media.*;import android.os.Build;import java.nio.*;import java.util.concurrent.atomic.AtomicBoolean;import java.util.concurrent.atomic.AtomicInteger;import java.util.concurrent.locks.LockSupport;
public final class Synth{
 static{System.loadLibrary("pianoroom");}
 public static native long load(String path);public static native long copy(long h,int voices);public static native void freeBank(long h);public static native String[] names(long h);public static native int[] presetInfo(long h);public static native void preset(long h,int p);private static native void voices(long h,int n);private static native void event(long h,int s,int a,int b);private static native long newFx();private static native void freeFx(long h);private static native void config(long h,float w,int r,float m,boolean mt,int t,int bt,float v);private static native void panic(long a,long b,long f);private static native void renderDirect(long a,long b,long c,long d,long f,float ga,float gb,float gc,float gd,ByteBuffer pcm,int frames);private static native void protection(long f,boolean enabled);private static native float[] levels(long f);private static native void tone(long f);
 public final Object lock=new Object();private final long fx=newFx();private final long[] layers=new long[4];private final int[] programs=new int[4];public final float[] gain={.8f,0,0,0};public float wet=.25f,master=.8f,metroVol=.6f,velocityScale=1;public int room=1,tempo=120,beats=4,transpose=0,midiChannel=0,voiceLimit=256;public boolean metro;
 private final AtomicBoolean running=new AtomicBoolean();private final AtomicInteger settings=new AtomicInteger();private Thread thread;private volatile AudioTrack track;private volatile AudioDeviceInfo preferred;public volatile String error="";private volatile int profile=0,manualMs=32,blockFrames=256,activeTarget=1424,underruns,recentUnderruns,actualBuffer,capacity,performance,deadlineMisses;private volatile boolean adaptive=true,protectedOutput=true,routeAccepted=true;private volatile long written,queuedFrames,renderMaxNanos,midiAgeNanos=-1,renderTotalNanos,renderBlocks,renderedFrames,stableSince;private volatile float peakBefore,peakAfter,limitedPercent,limiterGain=1,invalidSamples;
 public static native boolean mechanicalLoad(String path);public static native void bankRelease(long h,int pedalMs,int keyMs);
 public void libraryRelease(int channel,LibraryPedalSettings p){synchronized(lock){bankRelease(layers[channel],p.pedalCustom?p.pedalMs:0,p.keyCustom?p.keyMs:0);}}
 public Synth(){if(fx==0)throw new OutOfMemoryError("Efeito de áudio");}
 public ChannelSettings yamahaEffects=new ChannelSettings();
 private static native int yamahaInputOpen(int id);private static native void yamahaInputStop();private static native int[] yamahaInputStats();private static native void yamahaInputMix(long fx,boolean sendOnly,float gain);
 public int startYamahaInput(int id,boolean sendOnly,float gain){synchronized(lock){if(closed||activeBackend==0||!audioReady())return -1;yamahaChange();yamahaInputMix(fx,sendOnly,gain);int result=yamahaInputOpen(id);if(result==0)midiEvent(fx,layers[0],layers[1],layers[2],layers[3],0xB0,64,pedal,16);return result;}}
 public void stopYamahaInput(){synchronized(lock){yamahaInputStop();}}
 public int[] yamahaStats(){synchronized(lock){return yamahaInputStats();}}
 public void yamahaMix(boolean sendOnly,float gain){synchronized(lock){yamahaInputMix(fx,sendOnly,gain);}}
 public void yamahaChange(){synchronized(lock){ChannelSettings c=yamahaEffects;channelConfig(fx,4,c.pan/100f,c.chorus/100f,c.delay/100f,c.delayMs,c.feedback/100f,c.reverb/100f,c.resonance/100f,c.pedal,c.low,c.mid,c.high,0);pianoBody(fx,4,c.bodyEnabled,c.bodyAmount,c.boardAmount,c.bodySize,c.boardDamping,c.lid);soundSpace(fx,4,c.hybrid,c.spaceRoom,c.ambience,c.texture,c.ducking,c.spaceDecay,0,false);pianoControls(fx,4,0,0,false,c.hammer,c.keyNoise,c.pedalNoise);}}
 private static native void nativeConfigure(int backend);
 public static native long[] parallelStats();
 private static native int nativeOpen(long a,long b,long fx,float ga,float gb,int id,int ms);
 private static native void nativeStop();private static native int nativeBuffer(int ms);
 private static native int benchmarkCallbackMax();private static native int[] benchmarkPhaseMax();
 private static native int[] nativeStats();private static native long[] performancePoll();
 private volatile long[] performanceSnapshot=new long[33];
 private static native long[] bankMemory(long a,long b,long c,long d);
 public long[] bankMemory(){synchronized(lock){return bankMemory(layers[0],layers[1],layers[2],layers[3]);}}
 public long[] performanceSnapshot(){return performanceSnapshot.clone();}
 public String performanceDetails(){long[] p=performanceSnapshot;return "ADPF estado "+p[12]+" (2=ativo,1=aguardando,0=parado,-1=indisponível,-2=recusado) / relatórios "+p[13]+" / alvo ns "+p[14]+" / erros "+p[15]+" / thread áudio "+p[0];}
private static native void nativeLayers(long a,long b,long fx,float ga,float gb);
 private static native void nativeRate(long a,long b,long fx,int rate);
 private volatile int backend,activeBackend,activeRate=44100,nativeDevice=-1,nativeBurst,nativeSharing,nativeDrops,nativeProcessMax,nativeProcessAverage,nativeDeadlineMisses,nativeCpuPermille,nativeSampleMax,nativeEffectsMax,nativeMidiMax,nativeInputMax,nativeRecentMisses,nativeRecentDrops;
 private volatile AudioDeviceInfo[] available=new AudioDeviceInfo[0];public volatile String fallbackReason="";
 private static native void nativeExtra(long c,long d,float gc,float gd);
 public static native boolean retroEnable(boolean on);public static native long[] retroSnapshot();public static native int retroRead(long begin,int epoch,ByteBuffer buffer,int frames);
 public static native void recordStart(int rate);public static native void recordStop();public static native int recordRead(ByteBuffer buffer,int frames);public static native long recordFirstNanos();public static native int recordFailure();
 private static native float[] mixerLevels(long fx);public float[] mixerLevels(){synchronized(lock){return mixerLevels(fx);}}
 private static native int[] voiceStats(long a,long b,long c,long d);
 public int sampleRate(){return activeRate;}
 public boolean audioReady(){return running.get()&&audioReady;}
 public int[] voicesNow(){synchronized(lock){return voiceStats(layers[0],layers[1],layers[2],layers[3]);}}
 public String voiceDetails(){int[] v=voicesNow();return "Vozes: "+v[0]+" / "+v[1]+" / "+v[2]+" / "+v[3]+" · margem "+voiceLimit+" + 32 por canal\nCaudas suavizadas: "+v[4]+" · reciclagens no limite: "+v[5]+" · notas sem região/capacidade: "+v[6]+" · saídas suavizadas no limite: "+(v.length>7?v[7]:v[5]);}
 private static native void midiEvent(long fx,long a,long b,long c,long d,int type,int note,int value,int mask);
 private static native void midiFlush(long fx,long a,long b,long c,long d);
 private void flushMidi(){midiFlush(fx,layers[0],layers[1],layers[2],layers[3]);}private static native int[] midiQueueStats(long fx);
 private static native void sampleQuality(long a,long b,long c,long d,int mode);
 private static native void equalizer(long fx,boolean enabled,float low,float mid,float high);
 private static native int[] presetQuality(long bank,int preset);
 public String sampleDetails(int channel){synchronized(lock){if(channel<0||channel>=4||layers[channel]==0)return "Banco não carregado";int[] q=presetQuality(layers[channel],programs[channel]);return (q[0]==q[1]?q[0]+" Hz":q[0]+"–"+q[1]+" Hz")+" · "+q[3]+" zona(s) de intensidade em Dó4";}}
 private volatile int sampleMode=1;public boolean soundEq;public int eqLow,eqMid,eqHigh;
 public int sampleQuality(){return sampleMode;}
 public void sampleQuality(int mode){synchronized(lock){sampleMode=mode==1?1:0;sampleQuality(layers[0],layers[1],layers[2],layers[3],sampleMode);}}
 public void soundChange(){synchronized(lock){eqLow=Math.max(-6,Math.min(6,eqLow));eqMid=Math.max(-6,Math.min(6,eqMid));eqHigh=Math.max(-6,Math.min(6,eqHigh));equalizer(fx,soundEq,eqLow,eqMid,eqHigh);}}
 public String soundDetails(){return (sampleMode==1?"Alta qualidade · interpolação cúbica":"Original · interpolação linear")+" · "+(soundEq?"EQ "+eqLow+" / "+eqMid+" / "+eqHigh+" dB":"EQ desligado");}
 public final ChannelSettings[] channels={new ChannelSettings(),new ChannelSettings(),new ChannelSettings(),new ChannelSettings()};public boolean armSilent;public int armMask=3;public volatile int pedalValue,pedalIntermediate;private int pedal,pitch=8192;
 private static native boolean sceneSwap(long fx,long[] old,long[] next,float[] gains,int transpose,float master);private static native long tailDetach(long fx);private static native void tailFree(long garbage);private static native int[] tailStats(long fx);private static native void channelConfig(long fx,int channel,float pan,float chorus,float delay,float ms,float feedback,float reverb,float resonance,boolean pedal,float low,float mid,float high,int transpose);
 public static native float[] analyzePiano(long bank,int preset);
 private static native void pianoBody(long fx,int ch,boolean enabled,int body,int board,int size,int damping,int lid);
 private static native void pianoControls(long fx,int ch,int blend,int curve,boolean contextual,int hammer,int key,int pedal);
 private static native void pianoProfile(long fx,int ch,float[] profile,boolean harmonics);
 private static native void soundSpace(long fx,int ch,boolean enabled,int room,int amount,int texture,int duck,int decay,int repeat,boolean releaseSmooth);
 private final boolean[] appliedHarmonics=new boolean[4];
 private final float[][] pianoProfiles=new float[4][],appliedProfiles=new float[4][];private final long[] appliedProfileBank=new long[4];
 public void profile(int n,float[] p){synchronized(lock){pianoProfiles[n]=p;channelChange(n);}}
 public void channelChange(int n){synchronized(lock){ChannelSettings c=channels[n];channelConfig(fx,n,c.pan/100f,c.chorus/100f,c.delay/100f,c.delayMs,c.feedback/100f,c.reverb/100f,c.resonance/100f,c.pedal,c.low,c.mid,c.high,transpose);pianoBody(fx,n,c.bodyEnabled,c.bodyAmount,c.boardAmount,c.bodySize,c.boardDamping,c.lid);soundSpace(fx,n,c.hybrid,c.spaceRoom,c.ambience,c.texture,c.ducking,c.spaceDecay,c.repetition,c.releaseSmooth);pianoControls(fx,n,c.blend,c.blendCurve,c.contextual,c.hammer,c.keyNoise,c.pedalNoise);float[] selected=c.bankProfile?pianoProfiles[n]:null;if(appliedProfileBank[n]!=layers[n]||appliedProfiles[n]!=selected||appliedHarmonics[n]!=c.harmonics){pianoProfile(fx,n,selected,c.harmonics);appliedHarmonics[n]=c.harmonics;appliedProfileBank[n]=layers[n];appliedProfiles[n]=selected;}}}
 private static native boolean bankInUse(long fx,long base);public boolean bankInUse(long base){synchronized(lock){return !closed&&base!=0&&bankInUse(fx,base);}}
 public void collectTails(){if(Thread.holdsLock(lock))return;long garbage;synchronized(lock){if(closed)return;garbage=tailDetach(fx);}tailFree(garbage);}
 public String continuityDetails(){synchronized(lock){int[] a=tailStats(fx);return "Canais anteriores: "+a[0]+" · vozes antigas: "+a[1]+" · caudas suavizadas no limite: "+a[2];}}
 public String pedalDetails(){return "Pedal recebido: "+pedalValue+" / 127 · "+(pedalIntermediate>0?"valores intermediários observados":"ainda sem valores intermediários; teste o pedal");}
 public void replaceScene(long[] bases,int[] presets,float[] levels,ChannelSettings[] effects,int voices,int trans,float newMaster,float newWet,int newRoom){
  collectTails();long[] next=new long[4];try{for(int i=0;i<4;i++){next[i]=copy(bases[i],voices);if(next[i]==0)throw new OutOfMemoryError("Cena de áudio");preset(next[i],presets[i]);sampleQuality(next[i],0,0,0,sampleMode);}synchronized(lock){flushMidi();for(long h:next)restoreControllers(h);if(!sceneSwap(fx,layers,next,gain,trans,newMaster))throw new IllegalStateException("Aguarde as caudas terminarem antes de outra troca");System.arraycopy(next,0,layers,0,4);java.util.Arrays.fill(next,0);System.arraycopy(presets,0,programs,0,4);voiceLimit=voices;transpose=trans;master=newMaster;wet=newWet;room=newRoom;for(int i=0;i<4;i++){gain[i]=Math.max(0,Math.min(2,levels[i]));channels[i]=effects[i].copy();pianoProfiles[i]=null;channelChange(i);}change();syncLayers();}}finally{for(long h:next)if(h!=0)freeBank(h);}}

 public void auditionEvent(int layer,int type,int note,int value){if(layer<0||layer>3)return;synchronized(lock){if(!closed)midiEvent(fx,layers[0],layers[1],layers[2],layers[3],type,note,value,1<<layer);}}
 private void dispatch(int type,int note,int value){int mask=0;for(int i=0;i<4;i++)if(type!=0x90||value==0||gain[i]>0||(armSilent&&(armMask&(1<<i))!=0))mask|=1<<i;midiEvent(fx,layers[0],layers[1],layers[2],layers[3],type,note,value,mask);}
 private void syncLayers(){nativeLayers(layers[0],layers[1],fx,gain[0],gain[1]);nativeExtra(layers[2],layers[3],gain[2],gain[3]);}
 private void restoreControllers(long h){event(h,0xB0,64,pedal);event(h,0xE0,pitch&127,pitch>>7);}
 private volatile boolean closed,audioReady;
 public static String backendName(int mode){return mode==3?"Oboe 1.10.0 + canais paralelos":mode==2?"Oboe 1.10.0 serial":mode==1?"AAudio nativo":"AudioTrack";}
 public void backend(int mode){backend=Math.max(0,Math.min(3,mode));}public int backend(){return backend;}public int activeBackend(){return activeBackend;}
 public void availableDevices(AudioDeviceInfo[] devices){available=devices;}
 private double ms(long frames){return frames*1000.0/activeRate;}
 public void setLayer(int n,long bank,int p){collectTails();long next=copy(bank,voiceLimit);if(next==0)throw new OutOfMemoryError("Camada de áudio");preset(next,p);sampleQuality(next,0,0,0,sampleMode);synchronized(lock){try{flushMidi();restoreControllers(next);long[] desired=layers.clone();desired[n]=next;if(!sceneSwap(fx,layers,desired,gain,transpose,master))throw new IllegalStateException("Aguarde as caudas anteriores");layers[n]=next;pianoProfiles[n]=null;programs[n]=p;next=0;channelChange(n);syncLayers();}finally{if(next!=0)freeBank(next);}}}
 public void program(int n,int p){synchronized(lock){if(programs[n]==p)return;setLayer(n,layers[n],p);}}
 private static native void nativeGains(long fx,float a,float b,float c,float d);
 public void setGains(float[] values){synchronized(lock){for(int i=0;i<4;i++)gain[i]=Math.max(0,Math.min(2,values[i]));nativeGains(fx,gain[0],gain[1],gain[2],gain[3]);}}
 public void setGain(int n,float value){synchronized(lock){gain[n]=Math.max(0,Math.min(2,value));syncLayers();}}
 private volatile TouchResponse touch=new TouchResponse(0,1,1,127,100);
 public void touch(TouchResponse curve){touch=curve;}
 public int mappedVelocity(int raw){return touch.map(raw);}
 public boolean isRunning(){return running.get();}
 public int underrunCount(){return underruns;}
 public int acceptedBufferFrames(){return actualBuffer;}
 public String brief(){return "Buffer "+fmt(ms(actualBuffer))+" ms · falhas "+underruns+" · limitador "+fmt(limiterGain*100)+"%";}
 public double acceptedBufferMs(){return ms(actualBuffer);}public int tuningStepMs(){return activeBackend>0?Math.max(1,(int)Math.ceil(ms(nativeBurst))):4;}
 public void quality(int n){synchronized(lock){flushMidi();voiceLimit=n==512?512:n==768?768:256;for(int i=0;i<4;i++){voices(layers[i],voiceLimit);preset(layers[i],programs[i]);restoreControllers(layers[i]);}}}
 public void latencyProfile(int p){profile=Math.max(0,Math.min(2,p));settings.incrementAndGet();}public int latencyProfile(){return profile;}
 public void manualBuffer(int ms){manualMs=Math.max(backend>0?2:8,Math.min(120,ms));settings.incrementAndGet();}public int manualBuffer(){return manualMs;}
 public void automaticBuffer(boolean on){adaptive=on;settings.incrementAndGet();}public boolean automaticBuffer(){return adaptive;}
 public void blockSize(int n){blockFrames=n==128?128:256;settings.incrementAndGet();}public int blockSize(){return blockFrames;}
 public void recalibrate(){settings.incrementAndGet();}
 public void protectedOutput(boolean on){synchronized(lock){protectedOutput=on;protection(fx,on);}}public boolean protectedOutput(){return protectedOutput;}
 public void cleanTone(){panic();synchronized(lock){tone(fx);}}
 private static native void tailMaster(long fx,float value);public void master(float value){synchronized(lock){master=Math.max(0,Math.min(2,value));tailMaster(fx,master);change();}}
 public void change(){synchronized(lock){config(fx,wet,room,master,metro,tempo,beats,metroVol);for(int i=0;i<4;i++)channelChange(i);yamahaChange();}}
 public void panic(){synchronized(lock){if(closed)return;pedal=0;pitch=8192;panic(layers[0],layers[1],fx);for(int i=0;i<4;i++)preset(layers[i],programs[i]);}}
 public void receivedAt(long stamp){long now=System.nanoTime();midiAgeNanos=stamp>0&&stamp<=now&&now-stamp<1000000000L?now-stamp:-1;}
 public void midi(int s,int a,int b){if(!running.get()||(s&15)!=midiChannel)return;int k=s&0xF0;if(k==0x90&&b>0)b=touch.map(b);if((k==0x90||k==0x80)&&(a<0||a>127))return;synchronized(lock){if(closed)return;if(k==0xB0&&a==64){pedal=b;pedalValue=b;if(b>0&&b<127)pedalIntermediate++;}if(k==0xE0)pitch=a|(b<<7);dispatch(k,a,b);}}
 public void demo(int n,boolean on){synchronized(lock){if(closed)return;dispatch(on?0x90:0x80,n,touch.map(100));}}
 public void preferred(AudioDeviceInfo d){preferred=d;AudioTrack t=track;if(t!=null)routeAccepted=t.setPreferredDevice(d);}
 public AudioDeviceInfo routed(){if(activeBackend>0){for(AudioDeviceInfo d:available)if(d.getId()==nativeDevice)return d;return null;}AudioTrack t=track;try{return t==null?null:t.getRoutedDevice();}catch(IllegalStateException e){return null;}}
 private String fmt(double v){return String.format(java.util.Locale.US,"%.1f",v);}
 public String audioDetails(){long age=stableSince==0?0:(System.nanoTime()-stableSince)/1000000000L;String health=recentUnderruns>0?"Instável: aumente o buffer; faltou áudio no último segundo.":age<5?"Observando estabilidade…":"Sem novas falhas há "+age+" s";
  return (activeBackend>0?backendName(activeBackend)+" · "+(nativeSharing==0?"exclusivo":"compartilhado")+" · burst "+nativeBurst+" frames · conflitos "+nativeDrops:"AudioTrack estável")+(fallbackReason.isEmpty()?"":"\nRetorno automático: "+fallbackReason)+"\n"+health+"\nBuffer solicitado: "+fmt(ms(activeTarget))+" ms · aceito: "+fmt(ms(actualBuffer))+" ms\n"+(activeBackend>0?"Fila de reprodução não medida; buffer: ":"Fila estimada: ")+fmt(ms(queuedFrames))+" ms · capacidade: "+fmt(ms(capacity))+" ms\nFalhas: "+recentUnderruns+" no último intervalo de ~1 s · "+underruns+" nesta saída\nPico antes da proteção: "+fmt(peakBefore*100)+"% · saída: "+fmt(peakAfter*100)+"% · amostras limitadas: "+fmt(limitedPercent)+"%\nProteção de volume: "+(protectedOutput?"ativada":"desativada")+" · ganho do limitador: "+fmt(limiterGain*100)+"%\n"+(activeBackend>0?"Callback variável · burst "+nativeBurst+" frames · processamento máx.: "+fmt(nativeProcessMax/1000000.0)+" ms":"Blocos de "+blockFrames+" frames ("+fmt(ms(blockFrames))+" ms) · processamento máx.: "+fmt(renderMaxNanos/1000000.0)+" ms")+"\nBaixa latência Android: "+((activeBackend>0?performance==12:performance==AudioTrack.PERFORMANCE_MODE_LOW_LATENCY)?"aceita":"não confirmada pela rota")+"\nBuffer/fila não medem o atraso total entre tecla e alto-falante."+(actualBuffer>activeTarget+256?"\nO Android manteve um buffer maior que o solicitado.":"")+(routeAccepted?"":"\nO Android recusou a saída solicitada.");}
 public String crackleAdvice(){return AudioHealth.explain(recentUnderruns,activeBackend>0?nativeRecentDrops:0,activeBackend>0?nativeRecentMisses:deadlineMisses,peakBefore,limitedPercent);}
 public PianoBenchmark.Metrics benchmarkMetrics(){PianoBenchmark.Metrics m=new PianoBenchmark.Metrics();m.nativeAudio=activeBackend>0;if(m.nativeAudio){int[] phases=benchmarkPhaseMax();m.sampleMs=phases[0]/1000000.0;m.effectsMs=phases[1]/1000000.0;m.midiMs=phases[2]/1000000.0;m.inputMs=phases[3]/1000000.0;}int[] queue;synchronized(lock){queue=midiQueueStats(fx);}m.midiPending=queue[1];m.midiOverflow=queue[0];m.bufferMs=acceptedBufferMs();m.processMs=m.nativeAudio?benchmarkCallbackMax()/1000000.0:renderMaxNanos/1000000.0;m.peak=peakBefore;m.limited=limitedPercent;m.xruns=underruns;m.conflicts=m.nativeAudio?nativeDrops:0;m.late=m.nativeAudio?nativeDeadlineMisses:deadlineMisses;int[] v=voicesNow();for(int i=0;i<4;i++)m.voices+=v[i];m.perf=performanceSnapshot();m.engine=backendName(activeBackend);m.parallel=parallelStats();m.rate=activeRate;m.ready=audioReady();m.invalid=invalidSamples;System.arraycopy(v,0,m.channelVoices,0,4);m.soft=v[4];m.hard=v.length>7?v[7]:v[5];return m;}
 public String diagnostic(){int[] queue; synchronized(lock){queue=midiQueueStats(fx);}double cpu=renderedFrames==0?0:renderTotalNanos*100.0/(renderedFrames*1000000000.0/activeRate);return audioDetails()+"\nProcessamento paralelo: "+java.util.Arrays.toString(parallelStats())+" (workers, lotes, esperaMaxNs, esperaTotalNs, tarefas1, trabalho1Ns, prioridade1, tarefas2, trabalho2Ns, prioridade2)"+"\nEstalos: "+crackleAdvice()+"\n"+continuityDetails()+"\n"+pedalDetails()+"\n"+soundDetails()+"\nMIDI timestamp→recepção: "+(midiAgeNanos<0?"indisponível":fmt(midiAgeNanos/1000000.0)+" ms")+"\n"+activeRate+" Hz / estéreo / "+(activeBackend>0?backendName(activeBackend)+" callback nativo, float32 sem conversão PCM16 na reprodução":"PCM16, memória direta, escrita sincronizada")+"\nPerfil "+profile+" / manual "+manualMs+" ms / adaptação "+adaptive+(activeBackend>0?"\nProcessamento nativo médio: "+fmt(nativeProcessAverage/1000000.0)+" ms / carga média "+fmt(nativeCpuPermille/10.0)+"% / callbacks acima do prazo "+nativeDeadlineMisses:"\nProcessamento médio: "+fmt(renderBlocks==0?0:renderTotalNanos/1000000.0/renderBlocks)+" ms / carga estimada "+fmt(cpu)+"% / blocos acima do prazo "+deadlineMisses)+(activeBackend>0?"\nPicos por etapa (amostragem 1/16 callbacks): amostras "+fmt(nativeSampleMax/1000000.0)+" ms / efeitos "+fmt(nativeEffectsMax/1000000.0)+" ms / eventos MIDI "+fmt(nativeMidiMax/1000000.0)+" ms / entrada USB "+fmt(nativeInputMax/1000000.0)+" ms\nReverb com trabalho distribuído / ressonância SIMD / medidores sem bloqueio do render":"")+"\nFila MIDI: "+queue[1]+" pendentes / excessos: "+queue[0]+"\nAmostras inválidas no último intervalo: "+invalidSamples;}
 public synchronized void start(){if(closed)return;if(thread!=null&&thread.isAlive())return;if(running.getAndSet(true))return;audioReady=false;error="";thread=new Thread(()->{
  android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO);AudioTrack t=null;
  try{
   fallbackReason="";if(backend>0&&tryAAudio())return;activeBackend=0;activeRate=44100;
   synchronized(lock){nativeRate(layers[0],layers[1],fx,44100);nativeRate(layers[2],layers[3],fx,44100);}
   int min=AudioTrack.getMinBufferSize(44100,AudioFormat.CHANNEL_OUT_STEREO,AudioFormat.ENCODING_PCM_16BIT);if(min<=0)throw new IllegalStateException("Formato de áudio não disponível");
   t=new AudioTrack.Builder().setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build()).setAudioFormat(new AudioFormat.Builder().setSampleRate(44100).setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).setEncoding(AudioFormat.ENCODING_PCM_16BIT).build()).setTransferMode(AudioTrack.MODE_STREAM).setBufferSizeInBytes(Math.max(min,AudioBufferPolicy.framesForMs(200)*4)).setPerformanceMode(AudioTrack.PERFORMANCE_MODE_LOW_LATENCY).build();
   track=t;routeAccepted=t.setPreferredDevice(preferred);capacity=t.getBufferCapacityInFrames();AudioBufferPolicy policy=new AudioBufferPolicy();int selected=settings.get();policy.configure(profile,manualMs,adaptive,capacity);applyBuffer(t,policy.frames());
   written=queuedFrames=renderMaxNanos=renderTotalNanos=renderBlocks=renderedFrames=0;underruns=recentUnderruns=deadlineMisses=0;stableSince=System.nanoTime();peakBefore=peakAfter=limitedPercent=invalidSamples=0;
   ByteBuffer pcm=ByteBuffer.allocateDirect(256*4).order(ByteOrder.nativeOrder());change();protectedOutput(protectedOutput);t.play();audioReady=true;long epoch=0,lastRaw=0,lastStats=0,lastWindow=System.nanoTime(),lastRouteId=-2;int windowUnderruns=0;
   while(running.get()){
    long now=System.nanoTime();long raw=Integer.toUnsignedLong(t.getPlaybackHeadPosition());if(raw<lastRaw){if(lastRaw>0xf0000000L&&raw<0x10000000L)epoch+=1L<<32;else{epoch=0;written=raw;}}lastRaw=raw;queuedFrames=AudioBufferPolicy.queued(written,epoch+raw);
    if(selected!=settings.get()){selected=settings.get();policy.configure(profile,manualMs,adaptive,capacity);applyBuffer(t,policy.frames());}
    if(now-lastStats>=50000000L){lastStats=now;int count=t.getUnderrunCount();if(count>underruns)stableSince=now;underruns=count;
     if(policy.observe(count,now))applyBuffer(t,policy.frames());AudioDeviceInfo route=t.getRoutedDevice();long id=route==null?-1:route.getId();if(id!=lastRouteId){lastRouteId=id;capacity=t.getBufferCapacityInFrames();policy.configure(profile,manualMs,adaptive,capacity);applyBuffer(t,policy.frames());}
     actualBuffer=t.getBufferSizeInFrames();capacity=t.getBufferCapacityInFrames();performance=t.getPerformanceMode();
     if(now-lastWindow>=1000000000L){recentUnderruns=Math.max(0,count-windowUnderruns);windowUnderruns=count;lastWindow=now;synchronized(lock){float[] meter=levels(fx);peakBefore=meter[0];peakAfter=meter[1];limitedPercent=meter[2];limiterGain=meter[3];invalidSamples=meter[4];}}
    }
    int frames=blockFrames;long begin=System.nanoTime();synchronized(lock){renderDirect(layers[0],layers[1],layers[2],layers[3],fx,gain[0],gain[1],gain[2],gain[3],pcm,frames);}long elapsed=System.nanoTime()-begin;renderMaxNanos=Math.max(renderMaxNanos,elapsed);renderTotalNanos+=elapsed;renderBlocks++;renderedFrames+=frames;if(elapsed>frames*1000000000L/44100)deadlineMisses++;
    pcm.position(0);pcm.limit(frames*4);
    // AudioTrack supplies the playback clock and waits for real writable space.
    // The playback-head estimate above is diagnostic only; it never gates writes.
    while(pcm.hasRemaining()&&running.get()){int n=t.write(pcm,pcm.remaining(),AudioTrack.WRITE_BLOCKING);if(n<0){if(!running.get())break;throw new IllegalStateException("Saída de áudio "+n);}if(n==0){LockSupport.parkNanos(1000000L);continue;}if((n&3)!=0)throw new IllegalStateException("Quadro PCM incompleto");written+=n/4;}
   }
  }catch(Throwable ex){error=ex.toString();running.set(false);}finally{audioReady=false;track=null;if(t!=null){try{t.pause();t.flush();}catch(Exception ignored){}t.release();}}
 },"PianoAudio");thread.start();}

 private boolean tryAAudio(){
  int initial=profile==0?32:profile==1?12:manualMs;int result;
  synchronized(lock){panic();nativeConfigure(backend);result=nativeOpen(layers[0],layers[1],fx,gain[0],gain[1],preferred==null?-1:preferred.getId(),initial);if(result>=0)nativeExtra(layers[2],layers[3],gain[2],gain[3]);}
  if(result<0){fallbackReason=backendName(backend)+" indisponível ("+result+")";nativeStop();return false;}
  activeBackend=backend;stableSince=System.nanoTime();int selected=settings.get(),count=0,drops=0,lastLate=0;long window=0;boolean success=true;
  try{while(running.get()){
   int[] data=nativeStats();performanceSnapshot=performancePoll();activeRate=data[0]>0?data[0]:44100;actualBuffer=data[1];capacity=data[2];nativeBurst=data[3];nativeSharing=data[4];performance=data[5];underruns=data[6];nativeDevice=data[7];nativeDrops=data[9];nativeProcessMax=data[10];nativeProcessAverage=data[11];nativeDeadlineMisses=data[12];nativeCpuPermille=data[13];if(data.length>=18){nativeSampleMax=data[14];nativeEffectsMax=data[15];nativeMidiMax=data[16];nativeInputMax=data[17];}
   if(data[8]!=0){audioReady=false;fallbackReason=backendName(backend)+" interrompido ("+data[8]+")";success=false;break;}
   if(selected!=settings.get()){selected=settings.get();initial=profile==0?32:profile==1?12:manualMs;if(nativeBuffer(initial)<0){fallbackReason=backendName(backend)+" recusou o buffer";success=false;break;}}
   audioReady=actualBuffer>0&&data[0]>0;activeTarget=(activeRate*initial+999)/1000;queuedFrames=actualBuffer;
   long now=System.nanoTime();if(window==0||now-window>=1000000000L){recentUnderruns=Math.max(0,underruns-count);nativeRecentMisses=Math.max(0,nativeDeadlineMisses-lastLate);lastLate=nativeDeadlineMisses;nativeRecentDrops=Math.max(0,nativeDrops-drops);if(recentUnderruns>0){stableSince=now;if(adaptive){initial=AudioHealth.nextBuffer(initial,ms(actualBuffer),recentUnderruns);nativeBuffer(initial);}}
    if(data[9]-drops>8){fallbackReason="Muitos conflitos no callback nativo; preservada a saída estável";success=false;break;}drops=data[9];count=underruns;window=now;
    synchronized(lock){float[] meter=levels(fx);peakBefore=meter[0];peakAfter=meter[1];limitedPercent=meter[2];limiterGain=meter[3];invalidSamples=meter[4];}}
   LockSupport.parkNanos(50000000L);
  }}finally{audioReady=false;synchronized(lock){nativeStop();}performanceSnapshot=new long[33];activeBackend=0;activeRate=44100;synchronized(lock){nativeRate(layers[0],layers[1],fx,44100);nativeRate(layers[2],layers[3],fx,44100);panic();}}
  return success||!running.get();
 }
 private void applyBuffer(AudioTrack t,int frames){activeTarget=frames;int accepted=t.setBufferSizeInFrames(frames);if(accepted<=0)throw new IllegalStateException("Buffer de áudio recusado: "+accepted);if(Build.VERSION.SDK_INT>=31)t.setStartThresholdInFrames(Math.min(frames,t.getBufferSizeInFrames()));actualBuffer=t.getBufferSizeInFrames();capacity=t.getBufferCapacityInFrames();}
 public synchronized void stop(){audioReady=false;running.set(false);AudioTrack t=track;if(t!=null)try{t.pause();t.flush();}catch(Exception ignored){}if(thread!=null)try{thread.join();}catch(InterruptedException e){Thread.currentThread().interrupt();}thread=null;panic();}
 public synchronized void close(){if(closed)return;stop();synchronized(lock){if(closed)return;closed=true;for(int i=0;i<4;i++){if(layers[i]!=0)freeBank(layers[i]);layers[i]=0;}freeFx(fx);}}
}
