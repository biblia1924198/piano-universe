package com.luci.pianoroom;
import android.app.Activity;import android.os.Bundle;import android.widget.*;import java.io.File;
/** Own private recording, displayed in-app. */
public final class VideoPlaybackActivity extends Activity {
 private VideoView video;
 public void onCreate(Bundle b){super.onCreate(b);String path=getIntent().getStringExtra("path");File root=new File(getFilesDir(),"recordings");try{File file=new File(path);if(!file.getCanonicalPath().startsWith(root.getCanonicalPath()+File.separator)||!file.getName().endsWith(".mp4"))throw new Exception();video=new VideoView(this);setContentView(video);MediaController controls=new MediaController(this);controls.setAnchorView(video);video.setMediaController(controls);video.setVideoPath(file.getPath());video.setOnPreparedListener(p->video.start());video.setOnErrorListener((p,a,c)->{Toast.makeText(this,"Vídeo indisponível",Toast.LENGTH_LONG).show();return true;});}catch(Exception e){finish();}}
 protected void onPause(){if(video!=null)video.pause();super.onPause();}protected void onDestroy(){if(video!=null)video.stopPlayback();super.onDestroy();}
}
