package com.luci.pianoroom;
import java.io.*;
import java.security.*;
import java.util.Arrays;
import org.jflac.FLACDecoder;
import org.jflac.PCMProcessor;
import org.jflac.FrameListener;
import org.jflac.frame.Frame;
import org.jflac.metadata.Metadata;
import org.jflac.metadata.StreamInfo;
import org.jflac.util.ByteData;
/** Lossless offline decoding; no decoder or disk work in the audio callback. */
final class FlacWave {
 private static final class Failure extends RuntimeException {Failure(String message){super(message);}Failure(IOException cause){super(cause);}}
 static File decode(File source,File root,long remainingSamples)throws IOException {
  byte[] expected=new byte[16];
  try(DataInputStream in=new DataInputStream(new FileInputStream(source))){if(in.readInt()!=0x664c6143)throw new IOException("FLAC inválido: "+source.getName());int type=in.readUnsignedByte(),size=(in.readUnsignedByte()<<16)|(in.readUnsignedByte()<<8)|in.readUnsignedByte();if((type&127)!=0||size!=34)throw new IOException("FLAC sem STREAMINFO válido");byte[] info=new byte[34];in.readFully(info);System.arraycopy(info,18,expected,0,16);}
  File result=File.createTempFile("pr-flac-",".wav",root);boolean success=false;
  try(InputStream input=new BufferedInputStream(new FileInputStream(source));RandomAccessFile out=new RandomAccessFile(result,"rw")){
   FLACDecoder decoder=new FLACDecoder(input);decoder.readMetadata();StreamInfo info=decoder.getStreamInfo();
   int channels=info.getChannels(),bits=info.getBitsPerSample(),rate=info.getSampleRate(),align=channels*bits/8;
   if(channels<1||channels>2||(bits!=16&&bits!=24)||rate<8000||rate>192000||info.getMaxBlockSize()>65535||info.getTotalSamples()*channels>remainingSamples)throw new IOException("FLAC aceita 16/24 bits, mono/estéreo, 8–192 kHz; banco convertido até cerca de 8 GiB");
   out.setLength(44);out.seek(44);long[] bytes={0};MessageDigest digest=MessageDigest.getInstance("MD5");
   decoder.addFrameListener(new FrameListener(){public void processMetadata(Metadata m){}public void processFrame(Frame f){if(f.header.bitsPerSample!=bits||f.header.channels!=channels||f.header.sampleRate!=rate)throw new Failure("FLAC muda formato durante o áudio");}public void processError(String message){throw new Failure("FLAC corrompido: "+message);}});
   decoder.addPCMProcessor(new PCMProcessor(){public void processStreamInfo(StreamInfo s){}public void processPCM(ByteData data){int length=data.getLen();if(length==0||length%align!=0)throw new Failure("Bloco FLAC inválido");bytes[0]+=length;if(bytes[0]/(bits/8)>remainingSamples||bytes[0]>0xffffffffL-36)throw new Failure("FLAC descompactado excede limite do banco");try{out.write(data.getData(),0,length);digest.update(data.getData(),0,length);}catch(IOException e){throw new Failure(e);}}});
   decoder.decodeFrames();long frames=bytes[0]/align;if(frames<4||decoder.getBadFrames()!=0||(info.getTotalSamples()>0&&frames!=info.getTotalSamples()))throw new IOException("FLAC truncado ou com quadros perdidos: "+source.getName());
   if(!Arrays.equals(expected,new byte[16])&&!Arrays.equals(expected,digest.digest()))throw new IOException("FLAC falhou na verificação de integridade MD5");
   out.seek(0);out.writeBytes("RIFF");le(out,(int)bytes[0]+36);out.writeBytes("WAVEfmt ");le(out,16);out.writeShort(Short.reverseBytes((short)1));out.writeShort(Short.reverseBytes((short)channels));le(out,rate);le(out,rate*align);out.writeShort(Short.reverseBytes((short)align));out.writeShort(Short.reverseBytes((short)bits));out.writeBytes("data");le(out,(int)bytes[0]);success=true;return result;
  }catch(Failure e){throw new IOException(e.getMessage(),e);}catch(NoSuchAlgorithmException e){throw new IOException(e);}finally{if(!success)result.delete();}
 }
 private static void le(RandomAccessFile out,int value)throws IOException{out.writeInt(Integer.reverseBytes(value));}
}
