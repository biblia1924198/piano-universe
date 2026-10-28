package com.luci.pianoroom;
import android.graphics.*;import android.media.MediaMetadataRetriever;import android.os.*;import android.util.LruCache;import android.widget.ImageView;import java.io.*;import java.lang.ref.WeakReference;import java.util.concurrent.*;
/** Small video thumbnails are decoded off the UI and musical threads. */
public final class RecordingPreview {
 private static final ExecutorService worker=Executors.newSingleThreadExecutor(r->{Thread t=new Thread(()->{android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_BACKGROUND);r.run();},"PianoThumbnails");t.setDaemon(true);return t;});
 private static final Handler ui=new Handler(Looper.getMainLooper());
 private static final LruCache<String,Bitmap> images=new LruCache<String,Bitmap>(2048){protected int sizeOf(String key,Bitmap image){return Math.max(1,image.getAllocationByteCount()/1024);}};
 private RecordingPreview(){}
 public static void bind(ImageView target,File file){String key=file.getPath()+":"+file.length()+":"+file.lastModified();target.setTag(key);Bitmap cached=images.get(key);if(cached!=null){target.setImageBitmap(cached);return;}
  WeakReference<ImageView> reference=new WeakReference<>(target);
  worker.execute(()->{Bitmap bitmap=null;MediaMetadataRetriever retriever=new MediaMetadataRetriever();try{
   retriever.setDataSource(file.getPath());if(Build.VERSION.SDK_INT>=27)bitmap=retriever.getScaledFrameAtTime(0,MediaMetadataRetriever.OPTION_CLOSEST_SYNC,320,180);
   else{Bitmap full=retriever.getFrameAtTime(0,MediaMetadataRetriever.OPTION_CLOSEST_SYNC);if(full!=null){bitmap=Bitmap.createScaledBitmap(full,320,180,true);if(bitmap!=full)full.recycle();}}
  }catch(Exception ignored){}finally{try{retriever.release();}catch(Exception ignored){}}
   if(bitmap==null)return;final Bitmap result=bitmap;images.put(key,result);ui.post(()->{ImageView view=reference.get();if(view!=null&&key.equals(view.getTag()))view.setImageBitmap(result);});
  });
 }
}
