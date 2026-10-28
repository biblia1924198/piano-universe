const assert=require('assert'),fs=require('fs'),path=require('path');
const root=path.resolve(__dirname,'../../docs');
const {UniversePiano,KEY_NOTES,frequency}=require(path.join(root,'piano.js'));
const html=fs.readFileSync(path.join(root,'index.html'),'utf8');
class Element{
 constructor(tag='BUTTON'){this.tagName=tag;this.listeners={};this.dataset={};this.attrs={};this.value='35';this.pressed=false;this.label={textContent:''};this.classList={toggle:(n,v)=>this.pressed=v};}
 addEventListener(n,f){(this.listeners[n]??=[]).push(f);}emit(n,e={}){for(const f of this.listeners[n]||[])f({preventDefault(){},...e});}setAttribute(n,v){this.attrs[n]=v;}querySelector(){return this.label;}setPointerCapture(){}
}
class Param{
 constructor(v=0){this.value=v;this.events=[];}setValueAtTime(v,t){this.add(v,t);}linearRampToValueAtTime(v,t){this.add(v,t);}exponentialRampToValueAtTime(v,t){assert(v>0);this.add(v,t);}setTargetAtTime(v,t,tau){assert(tau>0);this.add(v,t);}cancelScheduledValues(t){assert(Number.isFinite(t));}
 add(v,t){assert(Number.isFinite(v)&&Number.isFinite(t));this.events.push({v,t});}
}
class Node{
 constructor(){this.connections=[];this.gain=new Param();this.frequency=new Param();}
 connect(n){this.connections.push(n);}disconnect(){this.disconnected=true;}start(t){assert(t>=0);this.started=t;}stop(t){assert(t>=0);this.stopped=t;if(this.onended)this.onended();}
}
class Audio{
 constructor(){this.state='suspended';this.currentTime=1;this.destination={};this.oscillators=[];}
 createGain(){return new Node();}createOscillator(){const o=new Node();this.oscillators.push(o);return o;}
 createDynamicsCompressor(){const n=new Node();for(const p of ['threshold','knee','ratio','attack','release'])n[p]=new Param();return n;}
 async resume(){this.state='running';}
}
function fixture(Ctor=Audio){
 const keys=[...html.matchAll(/<button class="piano-key (white|black)"([^>]+)>/g)].map(m=>{const e=new Element();for(const [key,name] of [['midi','data-midi'],['notePt','data-note-pt'],['noteEn','data-note-en']])e.dataset[key]=new RegExp(name+'="([^"]+)"').exec(m[2])[1];e.color=m[1];return e;});
 assert.strictEqual(keys.length,13);assert.deepStrictEqual(keys.map(e=>+e.dataset.midi).sort((a,b)=>a-b),Array.from({length:13},(_,i)=>60+i));assert.strictEqual(keys.filter(e=>e.color==='black').length,5);assert(keys.filter(e=>e.color==='black').every(e=>e.dataset.notePt.includes('♯ / ')&&e.dataset.notePt.includes('♭')));
 const doc=new Element();doc.documentElement={lang:'pt-BR'};doc.getElementById=id=>elements[id];doc.querySelectorAll=()=>keys;
 const elements={'piano-status':new Element('P'),'piano-volume':new Element('INPUT'),'piano-stop':new Element()};
 const win=new Element();win.AudioContext=Ctor;win.timers=new Map();win.setTimeout=f=>{const id=Symbol();win.timers.set(id,f);return id;};win.clearTimeout=id=>win.timers.delete(id);
 return {piano:new UniversePiano(doc,win),doc,win,keys,elements};
}
(async()=>{
 assert(Math.abs(frequency(60)-261.6255653006)<1e-8);assert(Math.abs(frequency(61)/frequency(60)-Math.pow(2,1/12))<1e-12);assert.strictEqual(Object.keys(KEY_NOTES).length,13);
 let f=fixture(),p=f.piano;
 for(const n of [60,64,67])await p.start('note'+n,n);
 assert.strictEqual(p.voices.size,3);assert.strictEqual(p.context.oscillators.length,9);
 p.release('note64');assert.strictEqual(p.voices.size,2);assert(!p.buttons.get(64).pressed);
 await p.start('also60',60);p.release('note60');assert(p.buttons.get(60).pressed);p.release('also60');assert(!p.buttons.get(60).pressed);
 p.silence();assert.strictEqual(p.voices.size,0);assert(p.context.oscillators.every(o=>o.stopped!==undefined));
 f.elements['piano-volume'].value='100';f.elements['piano-volume'].emit('input');
 for(let i=0;i<20;i++)await p.start('poly'+i,60+i%13);
 assert.strictEqual(p.voices.size,16);assert(!p.voices.has('poly0'));
 let bound=0;for(const v of p.voices.values()){const peak=Math.max(...v.gain.gain.events.map(e=>e.v));const partials=v.oscillators.reduce((s,o)=>s+o.connections[0].gain.value,0);bound+=peak*partials;}
 const master=Math.max(p.master.gain.value,...p.master.gain.events.map(e=>e.v));assert(bound*master<.95,'Full-scale voice sum exceeds intended headroom');
 f.doc.hidden=true;f.doc.emit('visibilitychange');assert.strictEqual(p.voices.size,0);
 f.doc.hidden=false;f.win.emit('keydown',{code:'KeyA',target:new Element('TEXTAREA')});assert.strictEqual(p.voices.size,0);
 f.win.emit('keydown',{code:'KeyA',target:new Element('BODY')});await Promise.resolve();assert.strictEqual(p.voices.size,1);
 f.win.emit('keydown',{code:'KeyA',repeat:true,target:new Element('BODY')});assert.strictEqual(p.voices.size,1);f.win.emit('keyup',{code:'KeyA'});assert.strictEqual(p.voices.size,0);
 const key=p.buttons.get(61);key.emit('pointerdown',{pointerId:9});await Promise.resolve();assert(key.pressed);key.emit('pointercancel',{pointerId:9});assert(!key.pressed);
 key.emit('keydown',{code:'Space'});await Promise.resolve();assert(key.pressed);key.emit('keyup',{code:'Space'});assert(!key.pressed);
 await p.start('blur',60);f.win.emit('blur');assert.strictEqual(p.voices.size,0);
 const old=p.context;old.state='closed';await p.start('recovered',60);assert.notStrictEqual(p.context,old);p.silence();
 f.doc.documentElement.lang='en';f.doc.emit('universe:language');assert.strictEqual(key.label.textContent,'C♯\nD♭');
 class DeferredAudio extends Audio{resume(){return new Promise(resolve=>{this.unlock=()=>{this.state='running';resolve();};});}}
 f=fixture(DeferredAudio);p=f.piano;const pending=p.start('quick',60);assert.strictEqual(p.voices.size,1);p.release('quick');p.context.unlock();await pending;assert.strictEqual(p.voices.size,0);assert.strictEqual(p.context.oscillators.length,0);
 const bad=fixture(class{constructor(){throw new Error('Audio unavailable');}});await bad.piano.start('error',60);assert.strictEqual(bad.piano.voices.size,0);
 console.log('PASS: 13 chromatic keys and enharmonic labels; C4 tuning; chords; same-note ownership; cancel/blur/visibility cleanup; bounded 16-voice peak; volume; typing guard; repeated keydown; closed-context recovery; audio-unlock race; unavailable audio; bilingual labels.');
})().catch(e=>{console.error(e);process.exitCode=1});
