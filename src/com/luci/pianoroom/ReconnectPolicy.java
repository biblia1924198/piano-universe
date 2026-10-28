package com.luci.pianoroom;

/** One open at a time. Tokens reject callbacks from older sessions. */
public final class ReconnectPolicy {
 private int generation,attempt;private boolean enabled,opening;
 public void enable(){generation++;attempt=0;opening=false;enabled=true;}
 public void disable(){generation++;opening=false;enabled=false;}
 public void lost(){generation++;opening=false;attempt=0;}
 public boolean enabled(){return enabled;}
 public boolean opening(){return opening;}
 public int begin(){if(!enabled||opening)return -1;opening=true;return generation;}
 public boolean current(int token){return enabled&&opening&&token==generation;}
 public boolean finish(int token,boolean success){if(!current(token))return false;opening=false;if(success)attempt=0;return true;}
 public long nextDelay(){return Math.min(30000L,500L<<Math.min(attempt++,6));}
}
