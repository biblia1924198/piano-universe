package com.luci.pianoroom;

/** Reject stale device/session callbacks, including callbacks after pause or a lens flip. */
public final class CameraEpoch {
 private long device=1,session,pending;
 public synchronized long beginOpen(){if(pending!=0)return 0;pending=++device;session++;return pending;}
 public synchronized void finishOpen(long token){if(pending==token)pending=0;}
 public synchronized boolean opening(){return pending!=0;}
 public synchronized boolean device(long token){return token==device;}
 public synchronized long device(){return device;}
 public synchronized long nextSession(){return ++session;}
 public synchronized boolean session(long deviceToken,long sessionToken){return deviceToken==device&&sessionToken==session;}
 public synchronized void invalidateSession(){session++;}
 public synchronized void close(){device++;session++;}
}
