package com.luci.pianoroom;
import android.content.Context;import android.graphics.*;import android.os.Bundle;import android.view.*;import android.view.accessibility.AccessibilityNodeInfo;
/** Long tactile light-theme fader. Values affect audio only when touched. */
public final class MixerFader extends View {
 public interface Change {void value(int percent);void tracking(boolean value);}
 private final Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);private final Change listener;private final int color,maximum;private final boolean metered;
 private final Typeface type=Typeface.create("sans-serif-medium",Typeface.NORMAL);private int value=80;private float level;private boolean dragging;private Shader rail,metal;
 public MixerFader(Context c,int tint,Change change){this(c,tint,200,true,change);}
 public MixerFader(Context c,int tint,int max,boolean meter,Change change){super(c);color=tint;maximum=max;metered=meter;listener=change;setFocusable(true);setContentDescription("Fader de volume");}
 protected void onSizeChanged(int w,int h,int ow,int oh){float x=w*.5f;rail=new LinearGradient(x-d(6),0,x+d(6),0,new int[]{0xFFBFC6B9,0xFFEEF0E9,0xFFBAC3B5},null,Shader.TileMode.CLAMP);metal=new LinearGradient(-d(17),-d(17),d(17),d(17),new int[]{0xFFFCFCF9,0xFFCACFC5,0xFFFAFAF5,0xFFB7BDB2},new float[]{0,.36f,.54f,1},Shader.TileMode.CLAMP);}
 public void value(int n){if(dragging)return;int next=Math.max(0,Math.min(maximum,n));if(next!=value){value=next;invalidate();}}
 public void level(float n){float next=Math.max(0,Math.min(1,n));if(next!=level){level=next;invalidate();}}
 private float d(float n){return n*getResources().getDisplayMetrics().density;}
 private float top(){return d(39);}private float bottom(){return Math.max(top()+d(30),getHeight()-d(19));}
 private void rect(Canvas c,int col,float x,float y,float w,float h,float r){p.setShader(null);p.setColor(col);p.setStyle(Paint.Style.FILL);c.drawRoundRect(x,y,x+w,y+h,r,r,p);}
 protected void onDraw(Canvas c){float top=top(),bottom=bottom(),height=bottom-top,x=getWidth()*.5f,y=bottom-height*value/maximum;
  p.setShader(null);p.setTypeface(type);p.setTextAlign(Paint.Align.CENTER);p.setTextSize(d(23));p.setColor(RoomTheme.INK);c.drawText(value+"%",getWidth()/2f,d(25),p);
  rect(c,0xFFE2E2D9,x-d(9),top-d(3),d(18),height+d(6),d(8));p.setShader(rail);p.setColor(Color.WHITE);c.drawRoundRect(x-d(6),top,x+d(6),bottom,d(5),d(5),p);p.setShader(null);rect(c,color,x-d(4),y,d(8),bottom-y,d(4));
  for(int i=0;i<=8;i++){float t=bottom-height*i/8;p.setColor(i%2==0?0xFF849081:0xFFBFC5BA);p.setStrokeWidth(d(1));c.drawLine(x-d(28),t,x-d(i%2==0?20:24),t,p);}
  p.setTextSize(d(10));p.setColor(RoomTheme.MUTED);p.setTextAlign(Paint.Align.RIGHT);for(int i=0;i<=4;i++)c.drawText(Math.round(maximum*i/4f)+"%",x-d(32),bottom-height*i/4+d(3),p);
  if(metered)for(int i=0;i<18;i++){float t=bottom-height*(i+1)/18;rect(c,i/18f<level?(i>15?RoomTheme.RED:i>13?RoomTheme.COPPER:color):0xFFDCDDD5,x+d(28),t,d(6),Math.max(d(1),height/18-d(2)),d(2));}
  else{rect(c,0xFFD2D6CA,x+d(29),top,d(3),height,d(2));rect(c,color,x+d(29),y,d(3),bottom-y,d(2));}
  p.setShader(null);p.setColor(0x220F1811);c.drawCircle(x+d(1),y+d(3),d(17),p);c.save();c.translate(x,y);p.setShader(metal);p.setColor(Color.WHITE);c.drawCircle(0,0,d(17),p);p.setShader(null);p.setStyle(Paint.Style.STROKE);p.setColor(0xFF8D9687);p.setStrokeWidth(d(1));c.drawCircle(0,0,d(17),p);p.setStyle(Paint.Style.FILL);p.setColor(color);p.setStrokeWidth(d(1.5f));c.drawLine(-d(10),0,d(10),0,p);c.restore();
 }
 private void update(float y){int n=Math.round(maximum*(bottom()-y)/(bottom()-top()));value=Math.max(0,Math.min(maximum,n));listener.value(value);invalidate();}
 public boolean onTouchEvent(MotionEvent e){switch(e.getActionMasked()){case MotionEvent.ACTION_DOWN:dragging=true;listener.tracking(true);if(getParent()!=null)getParent().requestDisallowInterceptTouchEvent(true);update(e.getY());return true;case MotionEvent.ACTION_MOVE:update(e.getY());return true;case MotionEvent.ACTION_UP:case MotionEvent.ACTION_CANCEL:dragging=false;listener.tracking(false);if(getParent()!=null)getParent().requestDisallowInterceptTouchEvent(false);if(e.getActionMasked()==MotionEvent.ACTION_UP)performClick();return true;}return true;}
 public boolean performClick(){super.performClick();return true;}
 public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info){super.onInitializeAccessibilityNodeInfo(info);info.setClassName("android.widget.SeekBar");info.setRangeInfo(AccessibilityNodeInfo.RangeInfo.obtain(AccessibilityNodeInfo.RangeInfo.RANGE_TYPE_INT,0,maximum,value));info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SCROLL_FORWARD);info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SCROLL_BACKWARD);info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SET_PROGRESS);}
 public boolean performAccessibilityAction(int action,Bundle args){int n=value;if(action==AccessibilityNodeInfo.ACTION_SCROLL_FORWARD)n+=10;else if(action==AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD)n-=10;else if(action==AccessibilityNodeInfo.AccessibilityAction.ACTION_SET_PROGRESS.getId()&&args!=null)n=Math.round(args.getFloat(AccessibilityNodeInfo.ACTION_ARGUMENT_PROGRESS_VALUE));else return super.performAccessibilityAction(action,args);value=Math.max(0,Math.min(maximum,n));listener.value(value);invalidate();return true;}
}
