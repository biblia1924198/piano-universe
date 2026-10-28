from pathlib import Path
import hashlib,json,sys,zipfile
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'assets/bundled-assets-manifest.json').read_text())
if len(sys.argv)!=2:raise SystemExit('Uso: python scripts/restore_assets.py Piano-Universe-Bancos-Incluidos.zip')
with zipfile.ZipFile(sys.argv[1]) as z:
 for entry in manifest:
  name=entry['path']; data=z.read(name)
  if len(data)!=entry['bytes'] or hashlib.sha256(data).hexdigest()!=entry['sha256']:raise SystemExit('Banco diferente do esperado: '+name)
  dest=root/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data);print('OK:',name)
