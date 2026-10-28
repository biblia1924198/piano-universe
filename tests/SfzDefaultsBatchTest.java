package com.luci.pianoroom;
import java.io.*;import java.nio.*;import java.nio.file.*;import java.util.*;import java.util.zip.*;

public final class SfzDefaultsBatchTest {
 static void check(boolean b,String message){if(!b)throw new AssertionError(message);}
 static void write(File root,String name,String value)throws Exception{Files.write(new File(root,name).toPath(),value.getBytes("UTF-8"));}
 static void rejects(File root,String value,String contains)throws Exception{
  write(root,"bad-control.sfz",value+"\n<region> sample=note.wav");
  try{SfzBank.compile(new File(root,"bad-control.sfz"),root,new File(root,"rejected.prb"),"bad");throw new AssertionError("accepted "+value);}catch(IOException e){check(e.getMessage().contains(contains),e.toString());}
 }
 public static void main(String[] args)throws Exception {
  File root=new File(args[0]);SfzBank.erase(root);check(root.mkdirs(),"root");
  SfzBankTest.wave(new File(root,"note.wav"),1,10000);
  write(root,"defaults.sfz","<control> set_cc7=80 set_cc10=32 set_cc11=100 label_cc7=Volume inicial\n<region> key=60 sample=note.wav ampeg_release=0.1");
  File target=new File(root,"defaults.prb");SfzBank.compile(new File(root,"defaults.sfz"),root,target,"Defaults");
  ByteBuffer b=ByteBuffer.wrap(Files.readAllBytes(target.toPath())).order(ByteOrder.LITTLE_ENDIAN);
  check(b.getInt(0)==0x34425250&&b.get(76+7)==81&&b.get(76+10)==33&&b.get(76+11)==101,"PRB4 default controller table");
  check(b.getInt(204)==60&&b.getInt(204+8)==1,"unaltered region data");check(InstrumentIndex.read(target,"Defaults",0).size()==1,"PRB4 catalog");
  rejects(root,"<control> set_cc7=128","set_cc7");rejects(root,"<control> set_cc128=10","0–127");rejects(root,"<control> set_cc7=-1","set_cc7");rejects(root,"<group> set_cc7=80","<control>");
  write(root,"label-only.sfz","<control> label_cc7=Volume\n<region> sample=note.wav");SfzBank.compile(new File(root,"label-only.sfz"),root,new File(root,"label.prb"),"Labels");
  check(Files.readAllBytes(new File(root,"label.prb").toPath())[3]=='1',"UI-only label does not alter sound or legacy format");
  File fixture=new File(root,"source");check(fixture.mkdir(),"fixture");SfzBankTest.wave(new File(fixture,"note.wav"),1,10000);
  write(fixture,"zones.sfz","<region> sample=note.wav key=60");write(fixture,"Grand.sfz","#include \"zones.sfz\"");
  write(fixture,"Soft.sfz","<control> set_cc7=90\n<region> sample=note.wav key=60 volume=-6");
  write(fixture,"Unsupported.sfz","<region> sample=note.wav cutoff=1000");
  check(SfzBank.discover(fixture).size()==3,"include SFZ excluded as duplicate instrument");
  BankImportBatch.Report selection=new BankImportBatch.Report();List<File> selected=BankImportBatch.select(fixture,Arrays.asList("Grand.sfz","Soft.sfz","Missing.sfz"),selection);
  check(selected.size()==2&&selection.failed==1&&selection.detail().contains("Missing.sfz"),"loose multi-SFZ selection retains only chosen instruments and identifies missing files");
  File zip=new File(root,"collection.zip");try(ZipOutputStream out=new ZipOutputStream(new FileOutputStream(zip))){for(File f:fixture.listFiles()){out.putNextEntry(new ZipEntry("collection/"+f.getName()));out.write(Files.readAllBytes(f.toPath()));out.closeEntry();}}
  File destination=new File(root,"imported");check(destination.mkdir(),"destination");ArrayList<File> accepted=new ArrayList<>();ArrayList<String> names=new ArrayList<>();
  BankImportBatch.Sink sink=new BankImportBatch.Sink(){public void progress(String text){}public void accept(File file,String name,String detail)throws IOException{check(InstrumentIndex.read(file,name,0).size()==1,"valid independent import");accepted.add(file);names.add(name);}};
  BankImportBatch.Report report=new BankImportBatch.Report();BankImportBatch.source(zip,new File(root,"stage"),destination,"collection.zip",sink,report);
  check(report.imported==2&&report.failed==1&&names.equals(Arrays.asList("Grand","Soft")),"two instruments succeed despite unsupported third");
  check(report.detail().contains("cutoff")&&report.detail().contains("Grand"),"per-instrument report");check(destination.listFiles().length==2,"failed import leaves no incomplete bank");
  BankImportBatch.Report refused=new BankImportBatch.Report();BankImportBatch.instruments(Collections.singletonList(new File(fixture,"Soft.sfz")),fixture,destination,new BankImportBatch.Sink(){public void progress(String t){}public void accept(File f,String n,String d)throws IOException{throw new IOException("native validation failed");}},refused);
  check(refused.failed==1&&destination.listFiles().length==2,"native-load failure rolls back only that instrument");
  System.out.println("PASS: set_cc7/10/11 + labels, bounds/header validation, PRB4 catalog, multi-SFZ ZIP/includes, partial success and rollback");
 }
}
