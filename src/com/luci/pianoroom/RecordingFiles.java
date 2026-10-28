package com.luci.pianoroom;
import java.io.*;
/** Only completed recordings owned by this app may be published or deleted. */
public final class RecordingFiles {
 private RecordingFiles(){}
 public static File require(File root,File file)throws IOException {
  File canonical=file.getCanonicalFile();String name=canonical.getName();
  if(!root.getCanonicalFile().equals(canonical.getParentFile())||!canonical.isFile()||
   !(name.endsWith(".wav")||name.endsWith(".mid")||name.endsWith(".mp4"))||name.endsWith(".video.mp4"))throw new IOException("Gravação indisponível");
  return canonical;
 }
 public static void delete(File root,File file)throws IOException {
  File target=require(root,file);String base=target.getName().replaceFirst("\\.(wav|mid|mp4)$","");
  if(!target.delete())throw new IOException("Não foi possível excluir a gravação");
  // MIDI and WAV from the same take share a scene. Keep it while either survives.
  if(!new File(root,base+".mid").exists()&&!new File(root,base+".wav").exists()&&!new File(root,base+".mp4").exists())new File(root,base+".scene").delete();
 }
}
