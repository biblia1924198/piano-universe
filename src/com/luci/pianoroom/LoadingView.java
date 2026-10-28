package com.luci.pianoroom;
import android.content.Context;import android.graphics.drawable.GradientDrawable;import android.view.Gravity;import android.widget.*;
/** Visible, indeterminate progress: never invents a percentage for parsing or disk I/O. */
public final class LoadingView extends LinearLayout {
 private final TextView title,detail;private final ProgressBar progress;
 public LoadingView(Context c){super(c);setOrientation(VERTICAL);setGravity(Gravity.CENTER);int p=dp(20);setPadding(p,p,p,p);GradientDrawable bg=new GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM,new int[]{0xFFFFFEFC,RoomTheme.SURFACE});bg.setCornerRadius(dp(20));bg.setStroke(dp(1),RoomTheme.BORDER);setBackground(bg);setElevation(dp(4));progress=new ProgressBar(c);addView(progress,new LayoutParams(dp(48),dp(48)));title=new TextView(c);title.setTextSize(18);title.setTextColor(RoomTheme.GREEN);title.setGravity(Gravity.CENTER);title.setPadding(0,dp(12),0,dp(8));addView(title,new LayoutParams(-1,-2));detail=new TextView(c);detail.setTextSize(13);detail.setTextColor(RoomTheme.MUTED);detail.setGravity(Gravity.CENTER);detail.setMaxLines(5);detail.setEllipsize(android.text.TextUtils.TruncateAt.END);addView(detail,new LayoutParams(-1,-2));setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_YES);title.setAccessibilityLiveRegion(ACCESSIBILITY_LIVE_REGION_POLITE);}
 private int dp(int n){return Math.round(n*getResources().getDisplayMetrics().density);}
 public void status(String heading,String message,boolean busy){title.setText(heading);detail.setText(message);progress.setVisibility(busy?VISIBLE:GONE);}
}
