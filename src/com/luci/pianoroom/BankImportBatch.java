package com.luci.pianoroom;

import java.io.*;
import java.util.*;

/** Each instrument is validated independently; completed banks survive a later failure. */
final class BankImportBatch {
 interface Sink {
  void progress(String text);
  void accept(File file,String name,String detail)throws IOException;
 }
 static final class Report {
  int imported,failed;
  final List<String> lines=new ArrayList<>();
  void failure(String label,Throwable error){failed++;lines.add("Falha · "+label+": "+(error.getMessage()==null?error.getClass().getSimpleName():error.getMessage()));}
  String summary(){return imported+" instrumentos importados"+(failed>0?" · "+failed+" falhas (ver resultado)":"");}
  String detail(){StringBuilder out=new StringBuilder(summary());for(String line:lines)out.append('\n').append(line);return out.toString();}
 }
 static void source(File input,File stage,File destination,String label,Sink sink,Report report)throws IOException {
  String reason=BankImportHelp.directReason(label);if(reason!=null)throw new IOException(reason);
  try(RandomAccessFile file=new RandomAccessFile(input,"r")){
   if(file.length()<12)throw new IOException("Arquivo incompleto");int magic=file.readInt();
   if(magic==0x504b0304){List<File> instruments=SfzBank.unpackAll(new FileInputStream(input),stage);input.delete();instruments(instruments,stage,destination,sink,report);}
   else if(magic==0x52494646){file.seek(8);if(file.readInt()!=0x7366626b)throw new IOException("Arquivo não é SoundFont SF2");one(input,null,destination,clean(label),sink,report);}
   else throw new IOException("Use SF2, ZIP SFZ ou o botão Importar pasta SFZ");
  }
 }
 static void instruments(List<File> files,File root,File destination,Sink sink,Report report){
  for(int n=0;n<files.size();n++){
   File source=files.get(n);String name=clean(source.getName());
   if((name.equalsIgnoreCase("main")||name.equalsIgnoreCase("instrument"))&&!source.getParentFile().equals(root))name=source.getParentFile().getName();
   sink.progress("Importando "+(n+1)+"/"+files.size()+" · "+name);
   one(source,root,destination,name,sink,report);
  }
 }
 private static void one(File source,File root,File destination,String name,Sink sink,Report report){
  File target=null;
  try{
   boolean sfz=source.getName().toLowerCase(Locale.ROOT).endsWith(".sfz");
   target=File.createTempFile("import-",sfz?".prb":".sf2",destination);
   String detail=sfz?SfzBank.compile(source,root,target,name):"SoundFont SF2";
   if(!sfz){SfzBank.space(destination,source.length());try(InputStream in=new FileInputStream(source);OutputStream out=new FileOutputStream(target)){byte[] b=new byte[65536];int n;while((n=in.read(b))!=-1)out.write(b,0,n);}}
   sink.accept(target,name,detail);report.imported++;report.lines.add("OK · "+name+" · "+detail);target=null;
  }catch(Exception e){report.failure(name,e);}finally{if(target!=null){new File(target.getPath()+".partial").delete();target.delete();}}
 }
 static List<File> select(File root,List<String> selected,Report report)throws IOException {
  List<File> files=SfzBank.discover(root);if(selected==null)return files;
  List<File> matches=new ArrayList<>();Set<String> missing=new LinkedHashSet<>(selected);
  for(File file:files)if(selected.contains(file.getName())){matches.add(file);missing.remove(file.getName());}
  for(String name:missing)report.failure(name,new IOException("SFZ não encontrado na pasta selecionada (ou é apenas um #include)"));return matches;
 }
 static String clean(String name){return name.replaceAll("(?i)\\.(sf2|sfz|zip)$","");}
}
