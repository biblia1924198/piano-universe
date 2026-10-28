package com.luci.pianoroom;
import java.io.*;
import java.nio.file.*;
import java.security.*;
import java.util.Arrays;
import org.jflac.FLACDecoder;
import org.jflac.PCMProcessor;
import org.jflac.FrameListener;
import org.jflac.frame.Frame;
import org.jflac.metadata.Metadata;
import org.jflac.metadata.StreamInfo;
import org.jflac.util.ByteData;
/** Lossless installer storage. Musical playback still opens the original SF2.
 * Restore/verification only run on the initial-load worker, never in render. */
final class BundledBanks {
 interface Source {InputStream open(String name)throws IOException;}
 private static final String[] NAMES={"GeneralUser-GS.sf2","FluidR3-GM.sf2","YDP-GrandPiano.sf2"};
 private static final long[] SIZES={32319396L,148398306L,118398836L};
 private static final String[] HASHES={"9575028c7a1f589f5770fccc8cff2734566af40cd26ed836944e9a5152688cfe","74594e8f4250680adf590507a306655a299935343583256f3b722c48a1bc1cb0","8757076aecf80abdad1e8f8f6168370399b06f94481796986e6e75ceca09ad21"};
 private static final class Failure extends RuntimeException {Failure(String text){super(text);}Failure(IOException cause){super(cause);}}
 static File ensure(String name,File dir,Source source)throws IOException {
  int index=Arrays.asList(NAMES).indexOf(name);if(index<0)throw new IOException("Banco integrado desconhecido");
  File target=new File(dir,name);byte[] expected=unhex(HASHES[index]);
  if(valid(target,SIZES[index],expected))return target;
  try(InputStream in=source.open(name+".prsf")){restore(in,target,SIZES[index],expected);}
  return target;
 }
 static boolean valid(File file,long size,byte[] expected)throws IOException {
  if(!file.isFile()||file.length()!=size)return false;
  MessageDigest hash=sha();try(InputStream in=new BufferedInputStream(new FileInputStream(file))){byte[] b=new byte[65536];int n;while((n=in.read(b))!=-1)hash.update(b,0,n);}
  return Arrays.equals(expected,hash.digest());
 }
 static void restore(InputStream stream,File target,long expectedSize,byte[] expectedHash)throws IOException {
  File temp=File.createTempFile("builtin-",".tmp",target.getParentFile());boolean published=false;
  try{try(DataInputStream in=new DataInputStream(new BufferedInputStream(stream));FileOutputStream file=new FileOutputStream(temp)){
   if(in.readInt()!=0x50525346||in.readInt()!=1)throw new IOException("Banco comprimido inválido");
   long size=in.readLong(),prefix=in.readLong(),samples=in.readLong(),packed=in.readLong(),suffix=in.readLong();byte[] digest=new byte[32];in.readFully(digest);
   if(size!=expectedSize||!Arrays.equals(digest,expectedHash)||prefix<20||suffix<0||prefix>size||suffix>size-prefix||samples!=size-prefix-suffix||samples<8||(samples&1)!=0||packed<42||packed>size*2)throw new IOException("Cabeçalho de banco incompatível");
   MessageDigest hash=sha();DigestOutputStream output=new DigestOutputStream(new BufferedOutputStream(file),hash);copy(in,output,prefix);
   Limit payload=new Limit(in,packed);FLACDecoder decoder=new FLACDecoder(payload);decoder.readMetadata();StreamInfo info=decoder.getStreamInfo();
   if(info==null||info.getChannels()!=1||info.getBitsPerSample()!=16||info.getSampleRate()!=48000||info.getTotalSamples()!=samples/2||info.getMaxBlockSize()>65535)throw new IOException("Formato de banco FLAC inválido");
   final long[] written={0};final long maximum=samples;
   decoder.addFrameListener(new FrameListener(){public void processMetadata(Metadata m){}public void processFrame(Frame f){if(f.header.channels!=1||f.header.bitsPerSample!=16||f.header.sampleRate!=48000)throw new Failure("Banco FLAC muda formato");}public void processError(String error){throw new Failure("Banco FLAC corrompido: "+error);}});
   decoder.addPCMProcessor(new PCMProcessor(){public void processStreamInfo(StreamInfo s){}public void processPCM(ByteData data){int n=data.getLen();if(n<=0||(n&1)!=0||n>maximum-written[0])throw new Failure("Amostras do banco excedem o limite");try{output.write(data.getData(),0,n);written[0]+=n;}catch(IOException e){throw new Failure(e);}}});
   decoder.decodeFrames();if(decoder.getBadFrames()!=0||written[0]!=samples||payload.left!=0)throw new IOException("Banco FLAC truncado");
   copy(in,output,suffix);if(in.read()!=-1)throw new IOException("Dados extras no banco");output.flush();file.getFD().sync();
   if(temp.length()!=size||!Arrays.equals(hash.digest(),expectedHash))throw new IOException("Banco falhou na verificação SHA-256");
  }
   // Close the descriptor before atomically publishing the verified file.
   Files.move(temp.toPath(),target.toPath(),StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);published=true;
  }catch(Failure e){throw new IOException(e.getMessage(),e);}finally{if(!published)temp.delete();}
 }
 private static void copy(InputStream in,OutputStream out,long size)throws IOException {byte[] b=new byte[65536];while(size>0){int n=in.read(b,0,(int)Math.min(b.length,size));if(n<=0)throw new EOFException("Banco incompleto");out.write(b,0,n);size-=n;}}
 private static MessageDigest sha()throws IOException {try{return MessageDigest.getInstance("SHA-256");}catch(NoSuchAlgorithmException e){throw new IOException(e);}}
 private static byte[] unhex(String text){byte[] b=new byte[text.length()/2];for(int i=0;i<b.length;i++)b[i]=(byte)Integer.parseInt(text.substring(i*2,i*2+2),16);return b;}
 private static final class Limit extends InputStream {
  final InputStream in;long left;Limit(InputStream in,long bytes){this.in=in;left=bytes;}
  public int read()throws IOException {if(left==0)return -1;int n=in.read();if(n<0)throw new EOFException("Banco FLAC incompleto");left--;return n;}
  public int read(byte[] b,int at,int size)throws IOException {if(size==0)return 0;if(left==0)return -1;int n=in.read(b,at,(int)Math.min(left,size));if(n<0)throw new EOFException("Banco FLAC incompleto");left-=n;return n;}
 }
}
