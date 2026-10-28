import com.luci.pianoroom.*;
public final class CaptureLifecycleTest {
 static void check(boolean b,String s){if(!b)throw new AssertionError(s);}
 public static void main(String[] args){
  CameraEpoch camera=new CameraEpoch();long first=camera.beginOpen();
  check(first!=0&&camera.beginOpen()==0,"no concurrent opens");
  camera.close();check(!camera.device(first)&&camera.opening(),"pause invalidates device but keeps pending open tracked");
  camera.finishOpen(first);long second=camera.beginOpen();check(second!=0&&camera.device(second),"reopen after stale callback");
  camera.finishOpen(first);check(camera.opening(),"old callback cannot clear new pending device");
  camera.finishOpen(second);long preview=camera.nextSession(),record=camera.nextSession();
  check(!camera.session(second,preview)&&camera.session(second,record),"late preview cannot replace record session");
  camera.invalidateSession();check(camera.device(second)&&!camera.session(second,record),"stop invalidates session, keeps device");
  long next=camera.nextSession();check(camera.session(second,next),"next preview accepted");
  camera.close();check(!camera.session(second,next),"callback after close discarded");
  AudioStartGate gate=new AudioStartGate(0);
  check(gate.observe(150,true,false,44100,0)==AudioStartGate.WAIT,"slow stream does not record stale default format");
  check(gate.observe(500,true,true,48000,1)==AudioStartGate.WAIT,"negotiated rate needs stability");
  check(gate.observe(599,true,true,48000,1)==AudioStartGate.WAIT&&gate.observe(600,true,true,48000,1)==AudioStartGate.READY,"ready after format settles");
  gate=new AudioStartGate(0);gate.observe(100,true,true,48000,1);
  check(gate.observe(199,true,true,44100,0)==AudioStartGate.WAIT&&gate.observe(299,true,true,44100,0)==AudioStartGate.READY,"fallback resets readiness interval");
  gate=new AudioStartGate(0);check(gate.observe(4000,true,false,44100,0)==AudioStartGate.FAILED,"bounded startup timeout");
  check(new AudioStartGate(0).observe(1,false,false,44100,0)==AudioStartGate.FAILED,"missing route fails promptly");
  System.out.println("PASS: stale camera opens/sessions rejected; pause/reopen and canceled session; real audio format readiness, fallback and timeout.");
 }
}
