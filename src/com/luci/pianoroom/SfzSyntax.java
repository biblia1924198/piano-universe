package com.luci.pianoroom;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.*;
import java.util.regex.*;
/** Bounded SFZ preprocessing and inheritance; unknown musical features fail explicitly. */
final class SfzSyntax {
 private static final Pattern VARIABLE=Pattern.compile("\\$[A-Za-z_][A-Za-z0-9_]*"), OP=Pattern.compile("([A-Za-z0-9_]+)\\s*="), TAG=Pattern.compile("<([^>]+)>");
 private static final Set<String> OPS=new HashSet<>(Arrays.asList("group","off_by","polyphony","group_volume","sustain_cc","lorand","hirand","curve_index","sustain_sw","sostenuto_sw","ampeg_dynamic","ampeg_release_oncc64","amp_veltrack","amplitude_oncc7","amplitude_oncc11","note_polyphony","note_selfmask","off_mode","off_time","xfin_lovel","xfin_hivel","xfout_lovel","xfout_hivel","xf_velcurve","rt_decay","sample","default_path","key","lokey","hikey","lovel","hivel","pitch_keycenter","pitch_keytrack","transpose","tune","volume","amplitude","pan","ampeg_attack","ampeg_decay","ampeg_sustain","ampeg_release","loop_mode","loop_start","loop_end","offset","end","seq_length","seq_position","trigger","label","global_label","master_label","group_label","region_label"));
 private final Map<String,String> macros=new HashMap<>();private final Set<String> stack=new HashSet<>();private final StringBuilder source=new StringBuilder();
 static final class Document {
  final List<Map<String,String>> regions;final Map<String,String> control;final List<Map<String,String>> curves;
  Document(List<Map<String,String>> regions,Map<String,String> control,List<Map<String,String>> curves){this.regions=regions;this.control=control;this.curves=curves;}
 }
 private final Set<String> included=new HashSet<>();
 static List<Map<String,String>> read(File file,File root)throws IOException {return document(file,root).regions;}
 static Document document(File file,File root)throws IOException {SfzSyntax syntax=new SfzSyntax();syntax.include(file,root,0);return regions(syntax.source.toString());}
 static Set<String> dependencies(File file,File root)throws IOException {SfzSyntax syntax=new SfzSyntax();syntax.include(file,root,0);syntax.included.remove(file.getCanonicalPath());return syntax.included;}
 private static int controller(String opcode,String prefix)throws IOException {
  try{int n=Integer.parseInt(opcode.substring(prefix.length()));if(n<0||n>127)throw new NumberFormatException();return n;}catch(NumberFormatException e){throw new IOException("Controlador SFZ fora de 0–127: "+opcode);}
 }
 private static boolean controlOpcode(String opcode)throws IOException {
  if(opcode.startsWith("set_cc")){controller(opcode,"set_cc");return true;}
  if(opcode.startsWith("label_cc")){controller(opcode,"label_cc");return true;}
  return false;
 }
 private static String velocityOpcode(String opcode)throws IOException {
  for(String prefix:new String[]{"amp_veltrack_oncc","amp_veltrack_curvecc"})if(opcode.startsWith(prefix))return prefix+controller(opcode,prefix);
  return null;
 }
 private void include(File file,File root,int depth)throws IOException {
  if(depth>12||!stack.add(file.getCanonicalPath()))throw new IOException("Include SFZ recursivo: "+file.getName());
  try {included.add(file.getCanonicalPath());if(!file.isFile())throw new IOException("Include ausente: "+file.getName());if(file.length()>4*1024*1024)throw new IOException("SFZ excessivo");String text=new String(Files.readAllBytes(file.toPath()),StandardCharsets.UTF_8);if(text.startsWith("\uFEFF"))text=text.substring(1);text=comments(text);
   for(String line:text.split("\\r?\\n",-1)){String value=line.trim();if(value.startsWith("#define")){Matcher def=Pattern.compile("#define\\s+(\\$[A-Za-z_][A-Za-z0-9_]*)\\s+(.+)").matcher(value);if(!def.matches())throw new IOException("#define inválido em "+file.getName());macros.put(def.group(1),def.group(2).trim());}
    else if(value.startsWith("#include")){String path=clean(expand(value.substring(8).trim()));if(path.isEmpty())throw new IOException("#include sem arquivo");File child=SfzBank.safe(root,relative(root,file.getParentFile())+path);include(child,root,depth+1);}
    else if(value.startsWith("#"))throw new IOException("Diretiva SFZ não suportada: "+value);
    else {String expanded=expand(line);if(source.length()+expanded.length()+1>8*1024*1024)throw new IOException("SFZ expandido excessivo");source.append(expanded).append('\n');}
   }
  }finally{stack.remove(file.getCanonicalPath());}
 }
 private String expand(String value)throws IOException {for(int round=0;round<16;round++){Matcher matcher=VARIABLE.matcher(value);StringBuffer out=new StringBuffer();boolean changed=false;while(matcher.find()){String replacement=macros.get(matcher.group());if(replacement==null)throw new IOException("Variável SFZ não definida: "+matcher.group());matcher.appendReplacement(out,Matcher.quoteReplacement(replacement));changed=true;if(out.length()>8*1024*1024)throw new IOException("Macro SFZ excessiva");}matcher.appendTail(out);if(!changed)return value;value=out.toString();if(value.length()>8*1024*1024)throw new IOException("Macro SFZ excessiva");}throw new IOException("Macro SFZ recursiva ou profunda demais");}
 private static String comments(String text)throws IOException {StringBuilder out=new StringBuilder(text.length());boolean quote=false,block=false,line=false;for(int i=0;i<text.length();i++){char c=text.charAt(i),next=i+1<text.length()?text.charAt(i+1):0;if(line){if(c=='\n'){line=false;out.append(c);}else out.append(' ');continue;}if(block){if(c=='*'&&next=='/'){out.append("  ");i++;block=false;}else out.append(c=='\n'?'\n':' ');continue;}if(c=='"')quote=!quote;if(!quote&&c=='/'&&next=='/'){line=true;out.append("  ");i++;}else if(!quote&&c=='/'&&next=='*'){block=true;out.append("  ");i++;}else out.append(c);}if(block)throw new IOException("Comentário SFZ sem fechamento");if(quote)throw new IOException("Aspas SFZ sem fechamento");return out.toString();}
 private static String mask(String text)throws IOException {char[] chars=text.toCharArray();boolean quote=false;for(int i=0;i<chars.length;i++){if(chars[i]=='"'){quote=!quote;chars[i]=' ';}else if(quote&&chars[i]!='\n')chars[i]=' ';}if(quote)throw new IOException("Aspas SFZ sem fechamento");return new String(chars);}
 private static String clean(String value){value=value.trim();return value.length()>1&&value.startsWith("\"")&&value.endsWith("\"")?value.substring(1,value.length()-1):value;}
 private static String relative(File root,File dir)throws IOException {String r=root.getCanonicalPath(),d=dir.getCanonicalPath();if(!d.equals(r)&&!d.startsWith(r+File.separator))throw new IOException("Include fora do banco");return d.equals(r)?"":d.substring(r.length()+1)+"/";}
 private static Map<String,String> parse(String value)throws IOException {Map<String,String> map=new LinkedHashMap<>();Matcher matcher=OP.matcher(mask(value));String last=null;int at=0;while(matcher.find()){if(last!=null)map.put(last,clean(value.substring(at,matcher.start())));else if(!value.substring(0,matcher.start()).trim().isEmpty())throw new IOException("Texto SFZ não reconhecido");last=matcher.group(1);String velocity=SfzAutomation.canonical(last);if(velocity!=null)last=velocity;if(!OPS.contains(last)&&velocity==null&&!last.matches("v[0-9]{3}")&&!controlOpcode(last))throw new IOException("Opcode SFZ não suportado: "+last);at=matcher.end();}if(last!=null)map.put(last,clean(value.substring(at)));else if(!value.trim().isEmpty())throw new IOException("Texto SFZ não reconhecido");return map;}
 private static Document regions(String source)throws IOException {
  List<Map<String,String>> result=new ArrayList<>(),curves=new ArrayList<>();Map<String,String> global=new HashMap<>(),master=new HashMap<>(),group=new HashMap<>(),control=new HashMap<>();Matcher tags=TAG.matcher(mask(source));String tag=null;int at=0;
  List<String> names=new ArrayList<>();List<Map<String,String>> values=new ArrayList<>();while(tags.find()){if(tag!=null){names.add(tag);values.add(parse(source.substring(at,tags.start())));}else if(!source.substring(0,tags.start()).trim().isEmpty())throw new IOException("SFZ precisa de cabeçalhos");tag=tags.group(1);if(!Arrays.asList("control","global","master","group","region","curve").contains(tag))throw new IOException("Cabeçalho SFZ não suportado: "+tag);at=tags.end();}if(tag!=null){names.add(tag);values.add(parse(source.substring(at)));}
  for(int n=0;n<names.size();n++){Map<String,String> ops=values.get(n);switch(names.get(n)){case "curve":curves.add(ops);break;case "control":for(String key:ops.keySet()){
   if(!key.equals("default_path")&&!key.endsWith("label")&&!controlOpcode(key))throw new IOException("Opcode não suportado em <control>: "+key);
   if(key.startsWith("set_cc"))try{int value=Integer.parseInt(ops.get(key));if(value<0||value>127)throw new NumberFormatException();}catch(NumberFormatException e){throw new IOException("Valor SFZ inválido (0–127): "+key);}
  }control.putAll(ops);break;case "global":global=new HashMap<>(ops);master=new HashMap<>(global);group=new HashMap<>(master);break;case "master":master=new HashMap<>(global);master.putAll(ops);group=new HashMap<>(master);break;case "group":group=new HashMap<>(master);group.putAll(ops);break;case "region":Map<String,String> region=new HashMap<>(group);region.putAll(ops);for(String key:region.keySet())if(controlOpcode(key))throw new IOException("Use "+key+" em <control>, não em <region>/<group>");if(control.containsKey("default_path"))region.put("default_path",control.get("default_path"));result.add(region);if(result.size()>16000)throw new IOException("SFZ excede 16 mil regiões");break;default:throw new IOException("Cabeçalho inválido");}}
  if(result.isEmpty())throw new IOException("SFZ sem regiões de áudio");SfzAutomation.curves(curves);for(Map<String,String> r:result){for(String key:r.keySet())if(key.equals("curve_index")||key.matches("v[0-9]{3}"))throw new IOException("Use "+key+" em <curve>");SfzAutomation.validateCurves(r,curves);}return new Document(result,control,curves);
 }
}
