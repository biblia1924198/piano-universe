import com.luci.pianoroom.*;import java.io.*;import java.nio.file.*;import java.util.*;
public final class RecordingStorageTest {
 static void check(boolean ok,String text){if(!ok)throw new AssertionError(text);}
 static final class Destination implements MediaPublication.Destination {
  final ByteArrayOutputStream data=new ByteArrayOutputStream();final ArrayList<String> events=new ArrayList<>();boolean failWrite,failPublish;
  public OutputStream open(){events.add("open");return new FilterOutputStream(data){public void write(byte[] bytes,int off,int len)throws IOException {if(failWrite)throw new IOException("disk full");out.write(bytes,off,len);}public void close()throws IOException{events.add("close");super.close();}};}
  public void publish()throws IOException {events.add("publish");if(failPublish)throw new IOException("provider failed");}
  public void discard(){events.add("discard");}
 }
 public static void main(String[] args)throws Exception {
  File root=Files.createTempDirectory("piano-recordings").toFile();File video=new File(root,"take.mp4");byte[] source=new byte[160000];new Random(42).nextBytes(source);Files.write(video.toPath(),source);
  Destination success=new Destination();MediaPublication.copy(video,success);check(Arrays.equals(source,success.data.toByteArray()),"entire video copied");check(success.events.equals(Arrays.asList("open","close","publish")),"publish only after close");
  for(int scenario=0;scenario<2;scenario++){Destination failed=new Destination();failed.failWrite=scenario==0;failed.failPublish=scenario==1;try{MediaPublication.copy(video,failed);throw new AssertionError("expected failure");}catch(IOException expected){}check(failed.events.get(failed.events.size()-1).equals("discard"),"failed pending item removed");check(video.exists()&&Arrays.equals(source,Files.readAllBytes(video.toPath())),"private original preserved");}
  File outside=Files.createTempFile("outside",".mp4").toFile();try{RecordingFiles.delete(root,outside);throw new AssertionError("outside accepted");}catch(IOException expected){}check(outside.exists(),"outside file survives");
  File unfinished=new File(root,"take.mp4.video.mp4");Files.write(unfinished.toPath(),source);try{RecordingFiles.require(root,unfinished);throw new AssertionError("unfinished accepted");}catch(IOException expected){}
  File link=new File(root,"linked.mp4");Files.createSymbolicLink(link.toPath(),outside.toPath());try{RecordingFiles.delete(root,link);throw new AssertionError("symlink escape accepted");}catch(IOException expected){}check(outside.exists(),"symlink target survives");
  File midi=new File(root,"take.mid"),wave=new File(root,"take.wav"),scene=new File(root,"take.scene");Files.write(midi.toPath(),source);Files.write(wave.toPath(),source);Files.write(scene.toPath(),source);
  RecordingFiles.delete(root,midi);check(scene.exists()&&wave.exists()&&video.exists(),"one delete preserves companion recordings and scene");RecordingFiles.delete(root,wave);check(scene.exists(),"video keeps take scene");RecordingFiles.delete(root,video);check(!scene.exists(),"orphan scene removed after last recording");
  for(File file:root.listFiles())file.delete();root.delete();outside.delete();System.out.println("PASS: complete publication after close, cleanup on disk/provider failure, original preservation, deletion containment/symlink checks and companion scene lifetime.");
 }
}
