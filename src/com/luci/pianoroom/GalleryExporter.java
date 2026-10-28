package com.luci.pianoroom;
import android.content.*;import android.net.Uri;import android.os.Build;import android.provider.MediaStore;import java.io.*;
public final class GalleryExporter {
 private GalleryExporter(){}
 public static Uri save(ContentResolver resolver,File file)throws IOException {
  if(Build.VERSION.SDK_INT<29)throw new IOException("Use Exportar para escolher uma pasta neste Android");
  ContentValues values=new ContentValues();values.put(MediaStore.Video.Media.DISPLAY_NAME,file.getName());
  values.put(MediaStore.Video.Media.MIME_TYPE,"video/mp4");values.put(MediaStore.Video.Media.RELATIVE_PATH,"Movies/Piano Room");values.put(MediaStore.Video.Media.IS_PENDING,1);
  final Uri uri=resolver.insert(MediaStore.Video.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY),values);
  if(uri==null)throw new IOException("A galeria não aceitou o vídeo");
  MediaPublication.copy(file,new MediaPublication.Destination(){
   public OutputStream open()throws IOException{return resolver.openOutputStream(uri,"w");}
   public void publish()throws IOException {ContentValues done=new ContentValues();done.put(MediaStore.Video.Media.IS_PENDING,0);if(resolver.update(uri,done,null,null)!=1)throw new IOException("Não foi possível concluir o vídeo na galeria");}
   public void discard(){try{resolver.delete(uri,null,null);}catch(Exception ignored){}}
  });return uri;
 }
 public static boolean exists(ContentResolver resolver,String uri){if(uri.isEmpty())return false;try(android.database.Cursor c=resolver.query(Uri.parse(uri),new String[]{MediaStore.Video.Media._ID},null,null,null)){return c!=null&&c.moveToFirst();}catch(Exception e){return false;}}
}
