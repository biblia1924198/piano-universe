const fs=require('fs'),vm=require('vm'),assert=require('assert');
const root=require('path').resolve(__dirname,'../../docs')+'/';
const html=fs.readFileSync(root+'index.html','utf8'),script=fs.readFileSync(root+'site.js','utf8');
class Element{
 constructor(id,attrs={}){this.id=id;this.dataset={};this.innerHTML='original '+id;this.textContent='';this.value='';this.hidden=false;this.handlers={};this.attrs=attrs;this.validity='';this.focused=false;}
 addEventListener(n,f){this.handlers[n]=f;} setAttribute(k,v){this.attrs[k]=v;} removeAttribute(k){delete this.attrs[k];}
 focus(){this.focused=true;}select(){this.selected=true;}setCustomValidity(v){this.validity=v;}reportValidity(){this.reported=true;}
 showModal(){this.open=true;}close(){this.open=false;}click(){if(this.onclick)this.onclick();}
}
const ids={};for(const m of html.matchAll(/\bid="([^"]+)"/g)){assert(!ids[m[1]],'duplicate id '+m[1]);ids[m[1]]=new Element(m[1]);}ids['report-result'].hidden=true;
const langEls=[...html.matchAll(/data-i18n="([^"]+)"/g)].map((m,i)=>{const el=new Element('lang'+i);el.dataset.i18n=m[1];return el;});
const ghEls=[...html.matchAll(/data-github="([^"]+)"/g)].map((m,i)=>{const el=new Element('gh'+i);el.dataset.github=m[1];return el;});
const document={dispatchEvent:()=>{},documentElement:{lang:'pt-BR'},getElementById:id=>{assert(ids[id],'unknown id '+id);return ids[id];},querySelectorAll:s=>s==='[data-i18n]'?langEls:s==='[data-github]'?ghEls:[],createElement:()=>new Element('download')};
const savedConfig=JSON.parse(fs.readFileSync(root+'site-config.json','utf8'));
const config={...savedConfig,preview:true,repository:null,pixKey:null,pixPayload:null};
const opened=[],copied=[];
const context={window:{PIANO_UNIVERSE:config,open:(...x)=>opened.push(x)},document,navigator:{clipboard:{writeText:async t=>copied.push(t)}},Blob,URL:{createObjectURL:()=> 'blob:local',revokeObjectURL:()=>{}},setTimeout:f=>f(),Event,console};
(async()=>{
vm.runInNewContext(script,context);
assert.strictEqual(config.preview,true);assert.strictEqual(config.repository,null);assert.strictEqual(config.pixPayload,null);
ghEls[0].click();assert(ids.notice.open);assert.strictEqual(opened.length,0);ids['notice-ok'].click();assert(!ids.notice.open);
await ids['pix-button'].onclick();assert(ids.notice.open);assert.strictEqual(copied.length,0);ids['close-notice'].click();
ids.language.click();assert.strictEqual(document.documentElement.lang,'en');assert(langEls.find(x=>x.dataset.i18n==='headline').innerHTML.includes('Your piano'));
ids.language.click();assert.strictEqual(document.documentElement.lang,'pt-BR');
const submit=ids['suggestion-form'].handlers.submit;ids.message.value='   ';submit({preventDefault(){}});assert(ids.message.reported);assert(ids['report-result'].hidden);
ids.message.value='Pedal muito longo <script>literal</script>';ids.device.value='S23 Ultra';ids.kind.value='Problema';submit({preventDefault(){}});assert(!ids['report-result'].hidden);assert(ids.report.value.includes('S23 Ultra'));assert(ids.report.value.includes('<script>literal</script>'));
await ids['copy-report'].onclick();assert.strictEqual(copied.at(-1),ids.report.value);
ids['save-report'].click();
config.pixKey=savedConfig.pixKey;await ids['pix-button'].onclick();assert.strictEqual(copied.at(-1),'a1c9629b-336b-4953-8942-4911a433c0f7');config.pixKey=null;
context.navigator.clipboard.writeText=async()=>{throw new Error('Clipboard unavailable')};await ids['copy-report'].onclick();assert(ids.report.selected);
config.repository='example-owner/piano-universe';config.releaseUrl='https://github.com/example-owner/piano-universe/releases/latest/download/Piano-Universe-v0.27-preview1.apk';vm.runInNewContext(script,context);ghEls[0].click();assert(opened.at(-1)[0].startsWith('https://github.com/example-owner/piano-universe'));assert.strictEqual(ids['hero-download'].href,config.releaseUrl);
assert.strictEqual(JSON.parse(fs.readFileSync(root+'site-config.json','utf8')).repository,'biblia1924198/piano-universe');
console.log('PASS: repository links, PT/EN, safe report generation, clipboard fallback, TXT export and exact Pix key copying. No payment is initiated.');
})().catch(e=>{console.error(e);process.exitCode=1});
