package com.luci.pianoroom;
import android.app.*;import android.content.*;import android.net.Uri;import android.os.Build;import android.widget.Toast;import java.io.File;
/** Retains the pending question across a paused/destroyed camera Activity. */
public final class GalleryPrompt {
 public interface LegacyExport{void export(File file);}
 private AlertDialog dialog;
 public void pause(){if(dialog!=null){dialog.dismiss();dialog=null;}}
 public void show(Activity activity,PianoService service,LegacyExport legacy){
  if(dialog!=null||service==null||service.closed||service.captureBusy()||activity.isFinishing())return;
  File file=service.pendingGallery();if(file==null)return;
  dialog=new AlertDialog.Builder(activity).setTitle("Salvar vídeo na galeria?")
   .setMessage("O vídeo com o áudio da mesa está pronto. Salvar uma cópia no álbum Piano Universe?")
   .setPositiveButton(Build.VERSION.SDK_INT>=29?"Salvar na galeria":"Escolher pasta",(d,w)->{service.resolveGallery(file);if(Build.VERSION.SDK_INT>=29)service.saveGallery(file,()->Toast.makeText(activity,service.captureStatus,Toast.LENGTH_LONG).show());else legacy.export(file);})
   .setNegativeButton("Só no aplicativo",(d,w)->service.resolveGallery(file))
   .setOnCancelListener(d->service.resolveGallery(file)).create();
  dialog.setOnDismissListener(d->dialog=null);dialog.show();
 }
}
