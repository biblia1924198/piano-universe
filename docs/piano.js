"use strict";
(()=>{
 const KEY_NOTES=Object.freeze({KeyA:60,KeyW:61,KeyS:62,KeyE:63,KeyD:64,KeyF:65,KeyT:66,KeyG:67,KeyY:68,KeyH:69,KeyU:70,KeyJ:71,KeyK:72});
 const frequency=midi=>440*Math.pow(2,(midi-69)/12);
 class UniversePiano{
  constructor(doc,win){
   this.doc=doc;this.win=win;this.voices=new Map();this.counts=new Map();this.buttons=new Map();this.context=null;this.master=null;this.volume=.35;
   this.status=doc.getElementById('piano-status');this.slider=doc.getElementById('piano-volume');
   for(const button of doc.querySelectorAll('.piano-key')){
    const midi=Number(button.dataset.midi);this.buttons.set(midi,button);
    button.addEventListener('pointerdown',event=>{if(event.pointerType==='mouse'&&event.button!==0)return;event.preventDefault();if(button.setPointerCapture)button.setPointerCapture(event.pointerId);this.start('pointer:'+event.pointerId,midi);});
    for(const name of ['pointerup','pointercancel','lostpointercapture'])button.addEventListener(name,event=>this.release('pointer:'+event.pointerId));
    button.addEventListener('keydown',event=>{if(event.code==='Space'||event.code==='Enter'){event.preventDefault();if(!event.repeat)this.start('control:'+midi,midi);}});
    button.addEventListener('keyup',event=>{if(event.code==='Space'||event.code==='Enter'){event.preventDefault();this.release('control:'+midi);}});
    // Screen readers and switch controls can activate a button without pointer events.
    button.addEventListener('click',event=>{if(event.detail===0&&!this.voices.has('control:'+midi)){this.start('assistive:'+midi,midi);win.setTimeout(()=>this.release('assistive:'+midi),230);}});
   }
   win.addEventListener('keydown',event=>{
    if(event.repeat||event.ctrlKey||event.altKey||event.metaKey||this.editing(event.target))return;
    const midi=KEY_NOTES[event.code];if(midi!==undefined){event.preventDefault();this.start('key:'+event.code,midi);}
   });
   win.addEventListener('keyup',event=>{if(KEY_NOTES[event.code]!==undefined)this.release('key:'+event.code);if(event.code==='Space'||event.code==='Enter')for(const source of [...this.voices.keys()])if(source.startsWith('control:'))this.release(source);});
   win.addEventListener('blur',()=>this.silence());win.addEventListener('pagehide',()=>this.silence());
   doc.addEventListener('visibilitychange',()=>{if(doc.hidden)this.silence();});
   doc.addEventListener('universe:language',()=>this.updateLabels());
   this.slider.addEventListener('input',()=>{this.volume=Math.min(1,Math.max(0,Number(this.slider.value)/100));if(this.master)this.master.gain.setTargetAtTime(this.volume*.25,this.context.currentTime,.02);});
   doc.getElementById('piano-stop').addEventListener('click',()=>this.silence());this.updateLabels();
  }
  english(){return this.doc.documentElement.lang.startsWith('en');}
  editing(target){return target&&(target.isContentEditable||['INPUT','TEXTAREA','SELECT'].includes(target.tagName));}
  audio(){
   if(this.context&&this.context.state!=='closed')return this.context;
   const Constructor=this.win.AudioContext||this.win.webkitAudioContext;
   if(!Constructor)throw new Error('Web Audio unavailable');
   this.context=new Constructor();this.master=this.context.createGain();this.master.gain.value=this.volume*.25;
   const compressor=this.context.createDynamicsCompressor();compressor.threshold.value=-18;compressor.knee.value=12;compressor.ratio.value=4;compressor.attack.value=.003;compressor.release.value=.2;
   this.master.connect(compressor);compressor.connect(this.context.destination);return this.context;
  }
  async start(source,midi){
   if(this.voices.has(source)||!this.buttons.has(midi))return;
   // Reserve before resume(): releasing during browser audio unlock must cancel the note.
   const voice={midi,oscillators:[],gain:null,timer:null};
   if(this.voices.size>=16)this.release(this.voices.keys().next().value);
   this.voices.set(source,voice);this.counts.set(midi,(this.counts.get(midi)||0)+1);this.paint(midi);
   try{
    const context=this.audio();if(context.state!=='running')await context.resume();
    if(this.voices.get(source)!==voice)return;
    const now=context.currentTime,gain=context.createGain();voice.gain=gain;
    gain.gain.setValueAtTime(.0001,now);gain.gain.linearRampToValueAtTime(.18,now+.008);gain.gain.exponentialRampToValueAtTime(.055,now+.28);gain.gain.setTargetAtTime(.0001,now+.28,1.8);gain.connect(this.master);
    for(const [partial,amount] of [[1,1],[2,.24],[3,.075]]){
     const oscillator=context.createOscillator(),weight=context.createGain();oscillator.type='sine';oscillator.frequency.setValueAtTime(frequency(midi)*partial,now);weight.gain.value=amount;oscillator.connect(weight);weight.connect(gain);
     oscillator.onended=()=>{oscillator.disconnect();weight.disconnect();if(voice.oscillators.every(o=>o.finished))gain.disconnect();};
     const oldEnd=oscillator.onended;oscillator.onended=()=>{oscillator.finished=true;oldEnd();};
     voice.oscillators.push(oscillator);oscillator.start(now);
    }
    voice.timer=this.win.setTimeout(()=>this.release(source),9000);
    this.status.textContent=this.label(midi)+(this.english()?' · browser sound':' · som do navegador');
   }catch{
    this.release(source);this.status.textContent=this.english()?'Audio could not start. Check your browser sound settings.':'Não foi possível iniciar o som. Confira o áudio e as permissões do navegador.';
   }
  }
  label(midi){const b=this.buttons.get(midi);return this.english()?b.dataset.noteEn:b.dataset.notePt;}
  paint(midi){const pressed=(this.counts.get(midi)||0)>0,b=this.buttons.get(midi);b.classList.toggle('pressed',pressed);b.setAttribute('aria-pressed',String(pressed));}
  release(source){
   const voice=this.voices.get(source);if(!voice)return;this.voices.delete(source);this.counts.set(voice.midi,Math.max(0,(this.counts.get(voice.midi)||1)-1));this.paint(voice.midi);
   if(voice.timer)this.win.clearTimeout(voice.timer);
   if(voice.gain){const now=this.context.currentTime;voice.gain.gain.cancelScheduledValues(now);voice.gain.gain.setTargetAtTime(.0001,now,.035);for(const oscillator of voice.oscillators)try{oscillator.stop(now+.16);}catch{}}
  }
  silence(){for(const source of [...this.voices.keys()])this.release(source);this.status.textContent=this.english()?'Sound off. Touch a key to play.':'Som silenciado. Toque uma tecla para tocar.';}
  updateLabels(){for(const [midi,button] of this.buttons){const label=this.label(midi);button.setAttribute('aria-label',label+(midi===72?' · C5':' · 4'));button.querySelector('.note-label').textContent=label.replace(' / ','\n');}}
 }
 if(typeof module!=='undefined'&&module.exports)module.exports={UniversePiano,KEY_NOTES,frequency};
 else if(document.getElementById('piano-keys'))new UniversePiano(document,window);
})();
