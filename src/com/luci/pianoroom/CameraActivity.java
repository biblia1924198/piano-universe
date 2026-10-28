package com.luci.pianoroom;
import android.Manifest;import android.app.*;import android.content.*;import android.content.pm.PackageManager;import android.content.pm.ActivityInfo;import android.content.res.Configuration;import android.graphics.*;import android.hardware.camera2.*;import android.hardware.camera2.params.StreamConfigurationMap;import android.media.MediaRecorder;import android.os.*;import android.util.*;import android.view.*;import android.widget.*;import java.io.*;import java.util.*;import java.util.concurrent.*;
/** Visible-only camera. The musical foreground service continues independently. */
public final class CameraActivity extends Activity {
 private final GalleryPrompt galleryPrompt=new GalleryPrompt();private File legacyGalleryExport;
 private final Handler ui=new Handler(Looper.getMainLooper());private HandlerThread cameraThread;private Handler handler;private PianoService service;private boolean bound;private volatile boolean visible,front,recording,preparing,finalizing,destroyed,changingCamera;private TextureView preview;private TextView status,zoomText;private Button record,flip,quality,rotate;private LinearLayout workArea;private ScrollView controlScroll;private FrameLayout previewFrame;private volatile CameraVideoModes.Mode mode;private volatile List<CameraVideoModes.Mode> videoModes=Collections.emptyList();private int previousOrientation;private int lastDisplayRotation=-1;private SeekBar zoomBar;private final ArrayList<Button> lensButtons=new ArrayList<>();private long lastCameraLog;private CameraManager manager;private volatile CameraDevice camera;private CameraCaptureSession session;private CaptureRequest.Builder request;private CameraCharacteristics chars;private Surface previewSurface;private MediaRecorder recorder;private Size size;private String cameraId="";private float minZoom=1,maxZoom=1,zoom=1;private volatile long videoFirst;private File rawVideo,finalVideo;private final CameraEpoch epoch=new CameraEpoch();private volatile PianoService takeOwner;
 private final ServiceConnection binding=new ServiceConnection(){public void onServiceConnected(ComponentName n,IBinder b){service=((PianoService.LocalBinder)b).service();}public void onServiceDisconnected(ComponentName n){stopRecording();service=null;}};
 public void onCreate(Bundle state){super.onCreate(state);if(state!=null){String path=state.getString("legacyGalleryExport","");if(!path.isEmpty())legacyGalleryExport=new File(path);}getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);getWindow().setStatusBarColor(0xFF0C141C);getWindow().setNavigationBarColor(0xFF0C141C);setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_FULL_SENSOR);manager=(CameraManager)getSystemService(CAMERA_SERVICE);cameraThread=new HandlerThread("PianoCamera");cameraThread.start();handler=new Handler(cameraThread.getLooper());bound=bindService(new Intent(this,PianoService.class),binding,BIND_AUTO_CREATE);build();if(checkSelfPermission(Manifest.permission.CAMERA)!=PackageManager.PERMISSION_GRANTED)requestPermissions(new String[]{Manifest.permission.CAMERA},72);}
 private int dp(int n){return (int)(n*getResources().getDisplayMetrics().density+.5f);}private TextView text(String s,int n){TextView t=new TextView(this);t.setText(s);t.setTextSize(n);t.setTextColor(0xFFEAF1F4);return t;}
 private Button button(String s,Runnable action){Button b=new Button(this);b.setText(s);b.setAllCaps(false);b.setTextSize(13);b.setTextColor(0xFFEAF1F4);android.graphics.drawable.GradientDrawable bg=new android.graphics.drawable.GradientDrawable();bg.setColor(0xFF253A43);bg.setCornerRadius(dp(12));b.setBackground(bg);b.setMinimumHeight(dp(48));b.setMinHeight(dp(48));b.setMinWidth(dp(48));b.setPadding(dp(8),dp(6),dp(8),dp(6));b.setMaxLines(2);b.setOnClickListener(v->action.run());return b;}
 private LinearLayout row(){LinearLayout r=new LinearLayout(this);r.setOrientation(0);r.setGravity(Gravity.CENTER_VERTICAL);android.graphics.drawable.GradientDrawable gap=new android.graphics.drawable.GradientDrawable();gap.setColor(Color.TRANSPARENT);gap.setSize(dp(8),1);r.setDividerDrawable(gap);r.setShowDividers(LinearLayout.SHOW_DIVIDER_MIDDLE);return r;}
 private void actions(LinearLayout parent,Button... buttons){LinearLayout r=row();for(Button b:buttons)r.addView(b,new LinearLayout.LayoutParams(0,-2,1));LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,-2);lp.topMargin=dp(8);parent.addView(r,lp);}
 private void build(){LinearLayout root=new LinearLayout(this);root.setOrientation(1);root.setPadding(dp(12),dp(6),dp(12),dp(8));root.setBackgroundColor(0xFF0C141C);root.setOnApplyWindowInsetsListener((v,in)->{v.setPadding(dp(12)+in.getSystemWindowInsetLeft(),dp(6)+in.getSystemWindowInsetTop(),dp(12)+in.getSystemWindowInsetRight(),dp(8)+in.getSystemWindowInsetBottom());return in.consumeSystemWindowInsets();});LinearLayout heading=row();heading.addView(text("PIANO ROOM · VÍDEO",16),new LinearLayout.LayoutParams(0,-2,1));rotate=button("Girar",()->{if(recording||preparing||finalizing)return;setRequestedOrientation(getResources().getConfiguration().orientation==Configuration.ORIENTATION_LANDSCAPE?ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT:ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);});heading.addView(rotate,new LinearLayout.LayoutParams(dp(72),dp(48)));heading.addView(button("Voltar",this::finish),new LinearLayout.LayoutParams(dp(72),dp(48)));root.addView(heading);workArea=new LinearLayout(this);LinearLayout.LayoutParams areaLp=new LinearLayout.LayoutParams(-1,0,1);areaLp.topMargin=dp(8);root.addView(workArea,areaLp);previewFrame=new FrameLayout(this);previewFrame.setBackgroundColor(Color.BLACK);preview=new TextureView(this);previewFrame.addView(preview,new FrameLayout.LayoutParams(-1,-1));workArea.addView(previewFrame);ScaleGestureDetector pinch=new ScaleGestureDetector(this,new ScaleGestureDetector.SimpleOnScaleGestureListener(){public boolean onScale(ScaleGestureDetector d){setZoom(zoom*d.getScaleFactor());return true;}});preview.setOnTouchListener((v,e)->{pinch.onTouchEvent(e);return true;});preview.setSurfaceTextureListener(new TextureView.SurfaceTextureListener(){public void onSurfaceTextureAvailable(SurfaceTexture t,int w,int h){open();}public void onSurfaceTextureSizeChanged(SurfaceTexture t,int w,int h){transform();}public boolean onSurfaceTextureDestroyed(SurfaceTexture t){stopRecording();closeCamera();return true;}public void onSurfaceTextureUpdated(SurfaceTexture t){}});
  controlScroll=new ScrollView(this);LinearLayout controls=new LinearLayout(this);controls.setOrientation(1);controls.setPadding(dp(8),dp(8),dp(8),dp(8));controlScroll.addView(controls);workArea.addView(controlScroll);quality=button("Resolução e FPS · consultando câmera…",this::qualityDialog);controls.addView(quality,new LinearLayout.LayoutParams(-1,-2));zoomText=text("Zoom 1×",13);zoomText.setPadding(0,dp(10),0,0);controls.addView(zoomText);zoomBar=new SeekBar(this);zoomBar.setMax(1000);zoomBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onStartTrackingTouch(SeekBar s){}public void onStopTrackingTouch(SeekBar s){}public void onProgressChanged(SeekBar s,int p,boolean user){if(user)setZoom((float)(minZoom*Math.pow(maxZoom/minZoom,p/1000.0)));}});controls.addView(zoomBar,new LinearLayout.LayoutParams(-1,dp(48)));LinearLayout lenses=row();for(float z:new float[]{.6f,1,3,10}){Button b=button(z+"×",()->setZoom(z));lensButtons.add(b);lenses.addView(b,new LinearLayout.LayoutParams(0,dp(48),1));}controls.addView(lenses);record=button("● Gravar",this::toggleRecord);flip=button("Frontal / traseira",()->{if(recording||preparing||finalizing){say("Finalize o vídeo antes de trocar frontal/traseira.");return;}front=!front;closeCamera();});actions(controls,record,flip);status=text("Preparando câmera…",12);status.setPadding(0,dp(10),0,dp(8));controls.addView(status);controls.addView(text("Câmera + prévia + H.264 validados juntos; até 60 FPS em sessão normal. Áudio da mesa, sem microfone. Escolha a orientação antes de gravar; ela permanece fixa durante o vídeo. FPS variável pode cair em pouca luz. Ao sair, o vídeo é finalizado e o piano continua.",11));setContentView(root);relayout();controlsEnabled();}
 private void relayout(){boolean wide=getResources().getConfiguration().orientation==Configuration.ORIENTATION_LANDSCAPE;workArea.setOrientation(wide?0:1);LinearLayout.LayoutParams pp=new LinearLayout.LayoutParams(wide?0:-1,wide?-1:0,1);if(wide)pp.rightMargin=dp(10);else pp.bottomMargin=dp(8);previewFrame.setLayoutParams(pp);controlScroll.setLayoutParams(new LinearLayout.LayoutParams(wide?dp(260):-1,wide?-1:-2));preview.post(this::transform);}
 @Override public void onConfigurationChanged(Configuration c){super.onConfigurationChanged(c);relayout();}
 private void controlsEnabled(){boolean idle=!recording&&!preparing&&!finalizing&&!changingCamera;quality.setEnabled(idle&&mode!=null);rotate.setEnabled(idle);flip.setEnabled(idle);record.setEnabled(!finalizing&&(recording||preparing||idle&&camera!=null));}
 private void qualityLabel(){quality.setText(mode==null?"Resolução e FPS · consultando câmera…":mode.label()+"  ▾");controlsEnabled();}
 private void qualityDialog(){if(recording||preparing||finalizing||mode==null)return;List<CameraVideoModes.Mode> options=new ArrayList<>(videoModes);String[] names=new String[options.size()];for(int i=0;i<names.length;i++)names[i]=options.get(i).label();new AlertDialog.Builder(this).setTitle("Qualidade · "+(front?"frontal":"traseira")).setSingleChoiceItems(names,options.indexOf(mode),(d,n)->{getSharedPreferences("PianoCamera",0).edit().putString("quality."+front,options.get(n).key()).apply();d.dismiss();quality.setEnabled(false);closeCamera();}).setNegativeButton("Fechar",null).show();}
 private void say(String value){ui.post(()->status.setText(value));}
 @Override public void onRequestPermissionsResult(int req,String[] ps,int[] results){super.onRequestPermissionsResult(req,ps,results);if(req==72&&results.length>0&&results[0]==PackageManager.PERMISSION_GRANTED)open();else say("Autorize a câmera nos ajustes para gravar vídeo.");}
 private void selectCamera()throws Exception {String chosen=null;float best=-1;for(String id:manager.getCameraIdList()){CameraCharacteristics c=manager.getCameraCharacteristics(id);Integer facing=c.get(CameraCharacteristics.LENS_FACING);if(facing==null||facing!=(front?CameraCharacteristics.LENS_FACING_FRONT:CameraCharacteristics.LENS_FACING_BACK))continue;Range<Float> range=Build.VERSION.SDK_INT>=30?c.get(CameraCharacteristics.CONTROL_ZOOM_RATIO_RANGE):null;float score=range==null?1:range.getUpper()/range.getLower();int[] caps=c.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES);if(caps!=null)for(int cap:caps)if(cap==CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA)score+=1000;if(score>best){best=score;chosen=id;chars=c;}}
  if(chosen==null)throw new IOException("Câmera não disponível");cameraId=chosen;Range<Float> range=Build.VERSION.SDK_INT>=30?chars.get(CameraCharacteristics.CONTROL_ZOOM_RATIO_RANGE):null;Float digital=chars.get(CameraCharacteristics.SCALER_AVAILABLE_MAX_DIGITAL_ZOOM);minZoom=range==null?1:range.getLower();maxZoom=range==null?(digital==null?1:digital):range.getUpper();zoom=Math.max(minZoom,Math.min(1,maxZoom));videoModes=CameraVideoModes.read(chars);mode=CameraVideoModes.choose(videoModes,getSharedPreferences("PianoCamera",0).getString("quality."+front,""));if(mode==null)throw new IOException("Nenhuma combinação de vídeo/prévia/FPS compatível com H.264");size=mode.video;}
 private void retryOpen(){if(destroyed){if(!epoch.opening())cameraThread.quitSafely();}else ui.post(this::open);}
 private void open(){
  if(!visible||destroyed||camera!=null||!preview.isAvailable()||checkSelfPermission(Manifest.permission.CAMERA)!=PackageManager.PERMISSION_GRANTED)return;
  final long token=epoch.beginOpen();if(token==0)return;
  handler.post(()->{try{
   if(!visible||destroyed||!epoch.device(token)){epoch.finishOpen(token);retryOpen();return;}
   selectCamera();manager.openCamera(cameraId,new CameraDevice.StateCallback(){
    public void onOpened(CameraDevice d){
     epoch.finishOpen(token);
     if(!epoch.device(token)||!visible||destroyed){d.close();retryOpen();return;}
     camera=d;changingCamera=false;createSession(false);
     ui.post(()->{if(!epoch.device(token)||!visible)return;setZoom(zoom);
      for(int i=0;i<lensButtons.size();i++){float preset=new float[]{.6f,1,3,10}[i];lensButtons.get(i).setEnabled(preset>=minZoom&&preset<=maxZoom);}qualityLabel();transform();
     });
    }
    public void onDisconnected(CameraDevice d){cameraFailure(d,token,"Câmera desconectada");}
    public void onError(CameraDevice d,int error){cameraFailure(d,token,"Câmera indisponível ("+error+")");}
   },handler);
  }catch(Exception e){epoch.finishOpen(token);if(epoch.device(token)){changingCamera=false;say("Falha na câmera: "+e.getMessage());ui.post(this::controlsEnabled);}if(destroyed)retryOpen();}});
 }
 private void cameraFailure(CameraDevice d,long token,String reason){
  epoch.finishOpen(token);boolean current=epoch.device(token);d.close();
  if(!current){retryOpen();return;}
  epoch.close();camera=null;request=null;
  if(session!=null){session.close();session=null;}
  if(previewSurface!=null){previewSurface.release();previewSurface=null;}
  ui.post(()->{changingCamera=false;stopRecording();controlsEnabled();say(reason);});if(destroyed)retryOpen();
 }
 private void createSession(boolean take){
  if(!visible||destroyed||take&&!preparing||camera==null)return;
  final long deviceToken=epoch.device(),sessionToken=epoch.nextSession();final CameraDevice owner=camera;
  final MediaRecorder takeRecorder=take?recorder:null;
  try{
   if(session!=null){session.close();session=null;}request=null;
   SurfaceTexture texture=preview.getSurfaceTexture();if(texture==null)return;
   texture.setDefaultBufferSize(mode.preview.getWidth(),mode.preview.getHeight());
   if(previewSurface==null)previewSurface=new Surface(texture);
   ArrayList<Surface> targets=new ArrayList<>();targets.add(previewSurface);
   if(take){if(takeRecorder==null)throw new IOException("Gravador de vídeo indisponível");targets.add(takeRecorder.getSurface());}
   final CaptureRequest.Builder builder=owner.createCaptureRequest(take?CameraDevice.TEMPLATE_RECORD:CameraDevice.TEMPLATE_PREVIEW);
   for(Surface surface:targets)builder.addTarget(surface);
   builder.set(CaptureRequest.CONTROL_AF_MODE,CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_VIDEO);
   builder.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE,mode.ae);builder.setTag(take?"record":"preview");applyZoom(builder);
   owner.createCaptureSession(targets,new CameraCaptureSession.StateCallback(){
    public void onConfigured(CameraCaptureSession configured){
     if(!epoch.session(deviceToken,sessionToken)||owner!=camera||!visible||destroyed||take&&(!preparing||recorder!=takeRecorder)){configured.close();return;}
     session=configured;request=builder;
     try{
      applyZoom(builder);
      if(take){videoFirst=0;takeRecorder.start();}
      configured.setRepeatingRequest(builder.build(),capture,handler);
      ui.post(()->{if(!epoch.session(deviceToken,sessionToken))return;if(take){if(!preparing||!visible)return;recording=true;preparing=false;record.setText("■ Finalizar");flip.setEnabled(false);}else if(!preparing&&!finalizing){record.setText("● Gravar");flip.setEnabled(true);}controlsEnabled();});
      say(take?"● Gravando vídeo + áudio da mesa":"Câmera "+(front?"frontal":"traseira")+" • "+mode.label()+" • zoom "+minZoom+"–"+maxZoom+"×");
     }catch(Exception e){say("Falha ao iniciar a câmera: "+e.getMessage());ui.post(CameraActivity.this::stopRecording);}
    }
    public void onConfigureFailed(CameraCaptureSession configured){
     configured.close();if(!epoch.session(deviceToken,sessionToken))return;
     say("Combinação de câmera/vídeo recusada");if(take)ui.post(CameraActivity.this::stopRecording);
    }
   },handler);
  }catch(Exception e){if(epoch.session(deviceToken,sessionToken)){say("Não abriu a prévia: "+e.getMessage());if(take)ui.post(this::stopRecording);}}
 }
 private final CameraCaptureSession.CaptureCallback capture=new CameraCaptureSession.CaptureCallback(){
  public void onCaptureCompleted(CameraCaptureSession active,CaptureRequest r,TotalCaptureResult result){
   if(active!=session||!visible||destroyed)return;
   long now=SystemClock.elapsedRealtime();if(now-lastCameraLog>1000){lastCameraLog=now;
    Float focal=result.get(CaptureResult.LENS_FOCAL_LENGTH);String physical=Build.VERSION.SDK_INT>=28?result.get(CaptureResult.LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID):null;
    final String diagnostic="Câmera "+cameraId+" / "+(front?"frontal":"traseira")+" / "+mode.label()+" / faixa "+minZoom+"–"+maxZoom+"x / zoom "+zoom+"x / física "+physical+" / focal "+focal+"mm";
    ui.post(()->{if(service!=null)service.prefs.edit().putString("cameraDiagnostic",diagnostic).apply();if(focal!=null)zoomText.setText(String.format(Locale.US,"Zoom %.2f× • lente %.1f mm • faixa %.1f–%.1f×",zoom,focal,minZoom,maxZoom));});
   }
   if((recording||preparing)&&videoFirst==0&&"record".equals(r.getTag())){
    Long sensor=result.get(CaptureResult.SENSOR_TIMESTAMP);Integer source=chars.get(CameraCharacteristics.SENSOR_INFO_TIMESTAMP_SOURCE);
    videoFirst=sensor!=null&&source!=null&&source==CameraCharacteristics.SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME?sensor:SystemClock.elapsedRealtimeNanos();
   }
  }
 };
 private void setZoom(float value){zoom=Math.max(minZoom,Math.min(maxZoom,value));zoomText.setText(String.format(Locale.US,"Zoom %.2f× • faixa %.1f–%.1f×",zoom,minZoom,maxZoom));zoomBar.setProgress(maxZoom<=minZoom?0:(int)(Math.log(zoom/minZoom)/Math.log(maxZoom/minZoom)*1000));handler.post(()->{try{applyZoom(request);if(session!=null&&request!=null)session.setRepeatingRequest(request.build(),capture,handler);}catch(Exception e){say("Zoom recusado: "+e.getMessage());}});}
 private void applyZoom(CaptureRequest.Builder builder){if(builder==null||chars==null)return;if(Build.VERSION.SDK_INT>=30&&chars.get(CameraCharacteristics.CONTROL_ZOOM_RATIO_RANGE)!=null)builder.set(CaptureRequest.CONTROL_ZOOM_RATIO,zoom);else{Rect rect=chars.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE);if(rect!=null){int w=(int)(rect.width()/zoom),h=(int)(rect.height()/zoom),x=rect.centerX()-w/2,y=rect.centerY()-h/2;builder.set(CaptureRequest.SCALER_CROP_REGION,new Rect(x,y,x+w,y+h));}}}
 private void transform(){if(mode==null||chars==null||preview.getWidth()==0||preview.getHeight()==0)return;int degrees=getWindowManager().getDefaultDisplay().getRotation()*90;Integer raw=chars.get(CameraCharacteristics.SENSOR_ORIENTATION);int sensor=raw==null?90:raw;float w=preview.getWidth(),h=preview.getHeight(),cx=w/2,cy=h/2;float[] scale=VideoModePolicy.previewScale((int)w,(int)h,mode.preview.getWidth(),mode.preview.getHeight(),sensor,degrees);Matrix matrix=new Matrix();matrix.setScale(scale[0],scale[1],cx,cy);matrix.postRotate(-degrees,cx,cy);if(front)matrix.postScale(-1,1,cx,cy);preview.setTransform(matrix);lastDisplayRotation=degrees/90;}
 private void toggleRecord(){
  if(recording||preparing){stopRecording();return;}
  if(finalizing||changingCamera||service==null||camera==null||mode==null||service.captureBusy()){say("Aguarde a câmera ou finalize a gravação atual.");return;}
  final PianoService piano=service;takeOwner=piano;rawVideo=finalVideo=null;videoFirst=0;
  previousOrientation=getRequestedOrientation();setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LOCKED);preparing=true;controlsEnabled();piano.cameraTake=true;record.setText("Preparando…");flip.setEnabled(false);
  boolean accepted=piano.startTake(true,true,()->{
   if(!preparing||!visible||camera==null){stopRecording();return;}
   handler.post(()->{if(!preparing||!visible)return;try{
    finalVideo=piano.newRecording(".mp4");rawVideo=new File(finalVideo.getPath()+".video.mp4");
    recorder=new MediaRecorder();recorder.setOnErrorListener((r,what,extra)->ui.post(()->{if(recorder==r){stopRecording();say("Gravador interrompido ("+what+"/"+extra+"); áudio/MIDI serão preservados.");}}));recorder.setVideoSource(MediaRecorder.VideoSource.SURFACE);
    recorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4);recorder.setOutputFile(rawVideo.getPath());
    recorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);recorder.setVideoEncodingBitRate(mode.bitrate);
    recorder.setVideoFrameRate(mode.fps);recorder.setVideoSize(size.getWidth(),size.getHeight());
    Integer sensor=chars.get(CameraCharacteristics.SENSOR_ORIENTATION);int degrees=getWindowManager().getDefaultDisplay().getRotation()*90;
    recorder.setOrientationHint(VideoModePolicy.orientation(sensor==null?90:sensor,degrees,front));
    recorder.prepare();createSession(true);
   }catch(Exception e){say("Vídeo não iniciou: "+e.getMessage());ui.post(this::stopRecording);}});
  });
  if(!accepted){setRequestedOrientation(previousOrientation);preparing=false;controlsEnabled();piano.cameraTake=false;takeOwner=null;record.setText("● Gravar");flip.setEnabled(true);say(piano.message);}
 }
 private void stopRecording(){
  if(!recording&&!preparing)return;
  recording=false;preparing=false;epoch.invalidateSession();finalizing=true;
  record.setText("Salvando…");record.setEnabled(false);flip.setEnabled(false);
  final PianoService piano=takeOwner;takeOwner=null;
  handler.post(()->{boolean valid=false;
   if(session!=null){try{session.stopRepeating();}catch(Exception ignored){}try{session.abortCaptures();}catch(Exception ignored){}session.close();session=null;}request=null;
   try{if(recorder!=null){recorder.stop();valid=true;}
  }catch(Exception e){say("Vídeo muito curto ou interrompido; áudio/MIDI serão preservados.");}
  finally{if(recorder!=null){recorder.release();recorder=null;}}
   final boolean ok=valid;final File video=rawVideo,out=finalVideo;final long firstVideo=videoFirst;
   if(!ok&&video!=null)video.delete();
   if(piano!=null){ui.post(()->{piano.cameraTake=false;piano.finishTake(()->{
    if(ok){File wave=piano.lastWave;long offset=(piano.wav.firstNanos>0&&firstVideo>0?(piano.wav.firstNanos-firstVideo)/1000:0)+piano.prefs.getInt("videoSyncMs",0)*1000L;
     say("Combinando vídeo com áudio limpo…");piano.exportVideo(video,wave,out,offset,()->{say(piano.captureStatus);afterStop();});
    }else{say(piano.captureStatus);afterStop();}
   });});}else ui.post(this::afterStop);
  });
 }
 private void afterStop(){finalizing=false;setRequestedOrientation(previousOrientation);controlsEnabled();if(visible&&!destroyed)galleryPrompt.show(this,service,this::legacyGallery);record.setEnabled(true);record.setText("● Gravar");flip.setEnabled(true);if(visible&&!destroyed&&camera!=null)handler.post(()->createSession(false));}
 private void closeCamera(){changingCamera=true;controlsEnabled();epoch.close();handler.post(()->{
  if(session!=null){session.close();session=null;}request=null;
  if(camera!=null){camera.close();camera=null;}
  if(previewSurface!=null){previewSurface.release();previewSurface=null;}
  retryOpen();
 });}
 private final Runnable health=new Runnable(){public void run(){if(!visible)return;if(lastDisplayRotation!=getWindowManager().getDefaultDisplay().getRotation())transform();if(preparing&&takeOwner!=null&&!takeOwner.captureBusy()){PianoService piano=takeOwner;stopRecording();say(piano.captureStatus);}if(service!=null&&!recording&&!preparing&&!finalizing)galleryPrompt.show(CameraActivity.this,service,CameraActivity.this::legacyGallery);if(service!=null&&(recording||preparing)&&!service.preparingTake&&(!service.synth.isRunning()||!service.wav.error.isEmpty()||!service.tape.warning.isEmpty())){stopRecording();say("A saída foi interrompida. A gravação foi finalizada.");}ui.postDelayed(this,500);}};
 private void legacyGallery(File file){legacyGalleryExport=file;startActivityForResult(new Intent(Intent.ACTION_CREATE_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE).setType("video/mp4").putExtra(Intent.EXTRA_TITLE,file.getName()),73);}
 protected void onSaveInstanceState(Bundle state){state.putString("legacyGalleryExport",legacyGalleryExport==null?"":legacyGalleryExport.getPath());super.onSaveInstanceState(state);}
 protected void onActivityResult(int request,int result,Intent data){super.onActivityResult(request,result,data);if(request==73&&result==RESULT_OK&&data!=null&&data.getData()!=null&&service!=null&&legacyGalleryExport!=null){service.exportFile(data.getData(),legacyGalleryExport);legacyGalleryExport=null;}}
 protected void onResume(){super.onResume();visible=true;ui.post(health);if(preview.isAvailable())open();}
 protected void onPause(){visible=false;galleryPrompt.pause();ui.removeCallbacks(health);stopRecording();closeCamera();super.onPause();}
 protected void onDestroy(){destroyed=true;if(bound)unbindService(binding);handler.post(()->{if(!epoch.opening())cameraThread.quitSafely();});super.onDestroy();}
}
