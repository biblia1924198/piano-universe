package com.luci.pianoroom;
import java.util.*;
/** Read before write and retain originals; only parameters answered by this piano. */
public final class YamahaRemote {
 public interface Wire{boolean send(byte[] message);}public interface Listener{void changed();}
 private final Wire wire;private final Listener listener;private final Map<String,byte[]> known=new LinkedHashMap<>(),original=new LinkedHashMap<>(),pending=new LinkedHashMap<>();private final Set<String> queried=new HashSet<>(),modified=new HashSet<>();
 public String model="",firmware="",status="Controle Yamaha ainda não consultado";public int sent,replies,confirmed;private final ArrayDeque<String> log=new ArrayDeque<>();
 public YamahaRemote(Wire w,Listener l){wire=w;listener=l;}
 private String key(int p,int i){return p+":"+i;}private void note(String s){log.addLast(s);while(log.size()>20)log.removeFirst();}
 private boolean tx(byte[] b){boolean ok=wire.send(b);if(ok)sent++;else status="Porta de envio indisponível. Feche Smart Pianist e reconecte.";note((ok?"TX ":"FALHA TX ")+hex(b));return ok;}
 public void reset(){known.clear();original.clear();pending.clear();queried.clear();modified.clear();model=firmware="";sent=replies=confirmed=0;log.clear();status="Controle Yamaha ainda não consultado";}
 public void probe(){status="Consultando parâmetros locais Yamaha…";tx(YamahaProtocol.identity());get(YamahaProtocol.MODEL,0);get(YamahaProtocol.FIRMWARE,0);for(int i=0;i<2;i++){get(YamahaProtocol.VOLUME,i);get(YamahaProtocol.REVERB,i);}get(YamahaProtocol.VOICE,0);listener.changed();}
 public void get(int p,int i){queried.add(key(p,i));tx(YamahaProtocol.get(p,i));}
 public boolean has(int p,int i){return known.containsKey(key(p,i));}public int number(int p,int i,int fallback){byte[] v=known.get(key(p,i));return v!=null&&v.length==1?v[0]&127:fallback;}
 public boolean ready(){return has(YamahaProtocol.VOLUME,0);}public boolean waiting(){return !pending.isEmpty();}
 public void receive(byte[] raw){YamahaProtocol.Reply r=YamahaProtocol.reply(raw);if(r==null){note((raw.length>=6&&raw[1]==0x7E&&raw[3]==6&&raw[4]==2?"Identidade MIDI: ":"RX SysEx não reconhecido: ")+hex(raw));return;}String k=key(r.property,r.index);note("RX "+hex(raw));if(!queried.contains(k)&&!pending.containsKey(k))return;replies++;if(r.property==YamahaProtocol.MODEL)model=YamahaProtocol.text(r.data);else if(r.property==YamahaProtocol.FIRMWARE)firmware=YamahaProtocol.text(r.data);else if(((r.property==YamahaProtocol.VOLUME||r.property==YamahaProtocol.REVERB)&&r.data.length==1||r.property==YamahaProtocol.VOICE&&r.data.length==4)&&r.index<2){known.put(k,r.data.clone());if(!original.containsKey(k))original.put(k,r.data.clone());byte[] want=pending.get(k);if(want!=null&&Arrays.equals(want,r.data)){pending.remove(k);confirmed++;status=r.property==YamahaProtocol.VOICE?"Timbre confirmado pelo Yamaha":"Valor confirmado pelo Yamaha: "+(r.property==YamahaProtocol.VOLUME?"volume ":"reverb ")+(r.data[0]&127);}else if(want==null)status="Yamaha respondeu ao controle local · volume principal "+number(YamahaProtocol.VOLUME,0,127);}listener.changed();}
 public void set(int p,int i,int v){setRaw(p,i,YamahaProtocol.value(v,1));}
 public void setRaw(int p,int i,byte[] b){String k=key(p,i);if(!has(p,i)){status="O Yamaha não respondeu à leitura deste parâmetro. Use o modo MIDI de compatibilidade para testar.";listener.changed();return;}pending.put(k,b);modified.add(k);if(tx(YamahaProtocol.set(p,i,b)))status="Comando enviado; aguardando confirmação do Yamaha…";get(p,i);listener.changed();}
 public void volume(int v){set(YamahaProtocol.VOLUME,0,v);if(has(YamahaProtocol.VOLUME,1))set(YamahaProtocol.VOLUME,1,v);}
 public void reverb(int v){set(YamahaProtocol.REVERB,0,v);if(has(YamahaProtocol.REVERB,1))set(YamahaProtocol.REVERB,1,v);}
 public void voice(int v){setRaw(YamahaProtocol.VOICE,0,YamahaProtocol.value(v,4));}
 public void restore(){restore(false);}
 public void restore(boolean audible){for(String k:new ArrayList<>(modified)){byte[] v=original.get(k);if(v==null)continue;String[] parts=k.split(":");int p=Integer.parseInt(parts[0]),i=Integer.parseInt(parts[1]);if(tx(YamahaProtocol.set(p,i,v))){known.put(k,v.clone());pending.remove(k);modified.remove(k);}}status="Restauração dos parâmetros originais solicitada";if(audible&&ready()&&number(YamahaProtocol.VOLUME,0,127)==0){String k=key(YamahaProtocol.VOLUME,0);byte[] v={100};if(tx(YamahaProtocol.set(YamahaProtocol.VOLUME,0,v))){known.put(k,v.clone());original.put(k,v.clone());pending.put(k,v.clone());get(YamahaProtocol.VOLUME,0);status="Recuperação do volume local enviada: 100; aguardando confirmação.";}}listener.changed();}
 public String diagnostic(){return "Modelo SysEx: "+model+" / firmware: "+firmware+"\n"+status+"\nSysEx enviados "+sent+", respostas "+replies+", valores confirmados "+confirmed+"\nPendências: "+pending.size()+"\n"+String.join("\n",log);}
 private static String hex(byte[] b){StringBuilder s=new StringBuilder();for(byte v:b)s.append(String.format(java.util.Locale.US,"%02X ",v&255));return s.toString().trim();}
}
