from pathlib import Path
import json
root=Path(__file__).resolve().parents[1]
for p in root.rglob('*'):
 if not p.is_file() or any(x in p.parts for x in ['.git','.gradle','build']):continue
 if p.relative_to(root).as_posix().startswith('docs/downloads/'):continue
 if p.suffix.lower() in ['.p12','.jks','.keystore','.pfx','.pem']:raise SystemExit('Credencial proibida: '+str(p))
 if p.stat().st_size>95*1024*1024:raise SystemExit('Arquivo grande deve ir em Releases: '+str(p))
for name in ['README.md','LICENSE','CONTRIBUTING.md','THIRD_PARTY_NOTICES.md','PRIVACY.md','docs/index.html','docs/site.js']:
 assert (root/name).is_file(),name
c=json.loads((root/'docs/site-config.json').read_text())
assert not c.get('pixKey') or c.get('pixPayload'),'Pix necessita payload validado pelo mantenedor.'
print('Fontes sem chaves de assinatura; documentos e site presentes.')
