package com.luci.pianoroom;
import android.media.*;import java.io.*;import java.nio.*;
/** Encode the clean mixer WAV and combine it with the camera video, off the audio thread. */
public final class VideoMux {
 public static void combine(File video,File wave,File output,long audioOffsetUs)throws Exception {
  File audio=new File(output.getPath()+".aac.m4a");try{encode(wave,audio);mux(video,audio,output,audioOffsetUs);}finally{audio.delete();}
 }
 private static void encode(File wave,File target)throws Exception {
  MediaCodec codec=null;MediaMuxer mux=null;boolean started=false;
  try(RandomAccessFile pcm=new RandomAccessFile(wave,"r")){
   byte[] header=new byte[44];pcm.readFully(header);ByteBuffer h=ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN);int rate=h.getInt(24);if(rate<8000||rate>192000||h.getShort(22)!=2||h.getShort(34)!=16)throw new IOException("WAV incompatível");long total=pcm.length()-44,read=0;MediaFormat format=MediaFormat.createAudioFormat("audio/mp4a-latm",rate,2);format.setInteger(MediaFormat.KEY_AAC_PROFILE,MediaCodecInfo.CodecProfileLevel.AACObjectLC);format.setInteger(MediaFormat.KEY_BIT_RATE,256000);format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE,16384);
   codec=MediaCodec.createEncoderByType("audio/mp4a-latm");codec.configure(format,null,null,MediaCodec.CONFIGURE_FLAG_ENCODE);codec.start();mux=new MediaMuxer(target.getPath(),MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);int track=-1;boolean inputEnd=false,outputEnd=false;byte[] bytes=new byte[16384];MediaCodec.BufferInfo info=new MediaCodec.BufferInfo();long lastProgress=System.nanoTime();
   while(!outputEnd){if(!inputEnd){int index=codec.dequeueInputBuffer(10000);if(index>=0){ByteBuffer in=codec.getInputBuffer(index);int n=(int)Math.min(Math.min(bytes.length,in.remaining()),total-read);n-=n%4;if(n>0){pcm.readFully(bytes,0,n);in.put(bytes,0,n);codec.queueInputBuffer(index,0,n,(read/4)*1000000/rate,0);read+=n;}else{codec.queueInputBuffer(index,0,0,(read/4)*1000000/rate,MediaCodec.BUFFER_FLAG_END_OF_STREAM);inputEnd=true;}lastProgress=System.nanoTime();}}
    int index=codec.dequeueOutputBuffer(info,10000);if(index==MediaCodec.INFO_OUTPUT_FORMAT_CHANGED){track=mux.addTrack(codec.getOutputFormat());mux.start();started=true;lastProgress=System.nanoTime();}else if(index>=0){ByteBuffer out=codec.getOutputBuffer(index);if((info.flags&MediaCodec.BUFFER_FLAG_CODEC_CONFIG)==0&&info.size>0){if(!started)throw new IOException("Encoder sem formato");out.position(info.offset);out.limit(info.offset+info.size);mux.writeSampleData(track,out,info);}outputEnd=(info.flags&MediaCodec.BUFFER_FLAG_END_OF_STREAM)!=0;codec.releaseOutputBuffer(index,false);lastProgress=System.nanoTime();}if(System.nanoTime()-lastProgress>10000000000L)throw new IOException("Encoder não respondeu");
   }
  }finally{if(codec!=null){try{codec.stop();}catch(Exception ignored){}codec.release();}if(mux!=null){try{if(started)mux.stop();}finally{mux.release();}}}
 }
 private static int select(MediaExtractor e,String prefix){for(int i=0;i<e.getTrackCount();i++)if(e.getTrackFormat(i).getString(MediaFormat.KEY_MIME).startsWith(prefix)){e.selectTrack(i);return i;}return -1;}
 private static void mux(File video,File audio,File output,long offset)throws Exception {
  MediaExtractor v=new MediaExtractor(),a=new MediaExtractor();MediaMuxer mux=null;boolean started=false;
  try{v.setDataSource(video.getPath());a.setDataSource(audio.getPath());int vi=select(v,"video/"),ai=select(a,"audio/");if(vi<0||ai<0)throw new IOException("Faixa de vídeo/áudio ausente");mux=new MediaMuxer(output.getPath(),MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);MediaMetadataRetriever meta=new MediaMetadataRetriever();try{meta.setDataSource(video.getPath());String rotation=meta.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_ROTATION);if(rotation!=null)mux.setOrientationHint(Integer.parseInt(rotation));}finally{meta.release();}
   int vt=mux.addTrack(v.getTrackFormat(vi)),at=mux.addTrack(a.getTrackFormat(ai));mux.start();started=true;long vOrigin=Math.max(0,v.getSampleTime()),aOrigin=Math.max(0,a.getSampleTime());ByteBuffer buffer=ByteBuffer.allocateDirect(8*1024*1024);MediaCodec.BufferInfo info=new MediaCodec.BufferInfo();
   while(v.getSampleTime()>=0||a.getSampleTime()>=0){long vp=v.getSampleTime()<0?Long.MAX_VALUE:v.getSampleTime()-vOrigin,ap=a.getSampleTime()<0?Long.MAX_VALUE:a.getSampleTime()-aOrigin+offset;if(ap<0){a.advance();continue;}MediaExtractor source=vp<=ap?v:a;long pts=source==v?vp:ap;buffer.clear();int n=source.readSampleData(buffer,0);if(n<0){if(!source.advance()&&source.getSampleTime()<0)continue;throw new IOException("Amostra MP4 inválida");}if(n>buffer.capacity())throw new IOException("Quadro de vídeo excedeu o limite");info.set(0,n,pts,source.getSampleFlags());mux.writeSampleData(source==v?vt:at,buffer,info);source.advance();}
  }finally{v.release();a.release();if(mux!=null){try{if(started)mux.stop();}finally{mux.release();}}}
 }
}
