package com.luci.pianoroom;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.View;
/** Family thumbnails decoded once, outside drawing and the audio thread. */
public final class InstrumentArt extends View {
 private static final int[] ART={R.drawable.grand_art,R.drawable.family_mallets,R.drawable.family_organs,R.drawable.family_guitars,R.drawable.family_basses,R.drawable.family_strings,R.drawable.family_orchestra,R.drawable.family_brass,R.drawable.family_woodwinds,R.drawable.family_flutes,R.drawable.family_synth_lead,R.drawable.family_pads,R.drawable.family_textures,R.drawable.family_ethnic,R.drawable.family_percussion,R.drawable.family_effects,R.drawable.family_drums,R.drawable.digital_art};
 private static final Bitmap[] CACHE=new Bitmap[ART.length];
 private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
 private final RectF bounds=new RectF();
 private int kind; private Bitmap bitmap;
 public InstrumentArt(Context context,int value){super(context);kind=value;load();setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_NO);}
 public void kind(int value){if(kind!=value){kind=value;load();fit();invalidate();}}
 private int index(){if(kind>=20&&kind<=36)return kind-20;if(kind==2)return 17;int[] legacy={0,0,17,11,10,6,1,2,3,4,5,7,8,9,12,2,16,15,5,13};return kind>=0&&kind<legacy.length?legacy[kind]:0;}
 public static void trim(){synchronized(CACHE){java.util.Arrays.fill(CACHE,null);}}
 public static void preload(Context c){for(int n=0;n<ART.length;n++)synchronized(CACHE){if(CACHE[n]==null)CACHE[n]=BitmapFactory.decodeResource(c.getResources(),ART[n]);}}
 private void load(){int n=index();synchronized(CACHE){if(CACHE[n]==null)CACHE[n]=BitmapFactory.decodeResource(getResources(),ART[n]);bitmap=CACHE[n];}}
 protected void onSizeChanged(int w,int h,int oldw,int oldh){super.onSizeChanged(w,h,oldw,oldh);fit();}
 private void fit(){if(bitmap==null)return;float scale=Math.min(getWidth()/(float)bitmap.getWidth(),getHeight()/(float)bitmap.getHeight());float w=bitmap.getWidth()*scale,h=bitmap.getHeight()*scale;bounds.set((getWidth()-w)/2,(getHeight()-h)/2,(getWidth()+w)/2,(getHeight()+h)/2);}
 protected void onDraw(Canvas canvas){super.onDraw(canvas);if(bitmap!=null)canvas.drawBitmap(bitmap,null,bounds,paint);}
}
