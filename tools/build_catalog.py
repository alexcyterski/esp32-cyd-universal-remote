#!/usr/bin/env python3
"""Build a compact, reproducible catalog from post-license Flipper-IRDB files."""
import argparse, collections, hashlib, json, struct, subprocess
from pathlib import Path
LICENSE_COMMIT='2319685f2cbf0cd3f809609622cade14d24fb819'
FMT='<IIIIIQIHHBB'
def rev(v,n):return int(f'{v:0{n}b}'[::-1],2)
def field(s):return int.from_bytes(bytes.fromhex(s),'little')
def convert(d):
    if d.get('type')=='raw':
        khz=round(int(d['frequency'])/1000);raw=list(map(int,d['data'].split()))
        if not 30<=khz<=60 or not 3<=len(raw)<=1024 or any(not 0<v<=65535 for v in raw) or sum(raw)>1500000:raise ValueError('raw limits')
        return 0,0,khz,0,raw
    p=d['protocol'];a=field(d['address']);cmd=field(d['command'])
    if p=='NEC' and a<256 and cmd<256:return 1,32,38,(rev(a,8)<<24)|(rev(a^255,8)<<16)|(rev(cmd,8)<<8)|rev(cmd^255,8),[]
    if p=='NECext' and a<65536 and cmd<65536:return 1,32,38,(rev(a,16)<<16)|rev(cmd,16),[]
    if p=='Samsung32' and a<256 and cmd<256:return 2,32,38,(rev(a,8)<<24)|(rev(a,8)<<16)|(rev(cmd,8)<<8)|rev(cmd^255,8),[]
    if p in ('SIRC','SIRC15','SIRC20'):
        bits={'SIRC':12,'SIRC15':15,'SIRC20':20}[p]
        if a >= (1<<(bits-7)) or cmd>=128:raise ValueError('Sony range')
        return 3,bits,40,rev((a<<7)|cmd,bits),[]
    if p=='RC5' and a<32 and cmd<64:return 4,12,36,(a<<6)|cmd,[]
    if p=='RC6' and a<256 and cmd<256:return 5,20,36,(a<<8)|cmd,[]
    raise ValueError('unsupported '+p)
def blocks(p):
    d={}
    for l in p.read_text(errors='replace').splitlines():
        if l.startswith('name:'):
            if 'name' in d:yield d
            d={}
        if ': ' in l and not l.startswith('#'):
            k,v=l.split(': ',1);d[k]=v.strip()
    if 'name' in d:yield d
def clean(s):return ''.join(c if 32<=ord(c)<127 else ' ' for c in s).replace('_',' ').strip()
def main():
    ap=argparse.ArgumentParser();ap.add_argument('repo',type=Path);ap.add_argument('--output',type=Path,default=Path('assets'));args=ap.parse_args()
    git=lambda *a:subprocess.check_output(['git','-C',str(args.repo),*a]).decode().strip()
    head=git('rev-parse','HEAD')
    # Only files absent at licensing cutoff. Older files were explicitly not relicensed.
    old=set(git('ls-tree','-r','--name-only',LICENSE_COMMIT).splitlines())
    files=sorted(p for p in args.repo.rglob('*.ir') if '.git' not in p.parts and p.relative_to(args.repo).as_posix() not in old)
    rows=[];skips=collections.Counter();sources=[]
    for p in files:
        rel=p.relative_to(args.repo);parts=rel.parts
        if len(parts)<3 or parts[0].startswith('_'):continue
        sources.append(str(rel))
        for d in blocks(p):
            try:code=convert(d)
            except (ValueError,KeyError,OverflowError):skips[d.get('protocol',d.get('type','unknown'))]+=1;continue
            action=clean(d.get('name','Action'));model=clean(p.stem);typ=clean(parts[0]);brand=clean(parts[1])
            canonical='|'.join([str(rel),action,json.dumps(code)])
            uid=int.from_bytes(hashlib.sha256(canonical.encode()).digest()[:4],'little')
            rows.append((typ,brand,action,model,uid,code))
    rows.sort(key=lambda r:(r[0].lower(),r[1].lower(),r[2].lower(),r[3].lower(),r[4]))
    seen=set();unique=[]
    for r in rows:
        if r[4] not in seen:unique.append(r);seen.add(r[4])
    pool=bytearray();strings={};records=bytearray()
    def string(s):
        if s not in strings:strings[s]=len(pool);pool.extend(s.encode()+b'\0')
        return strings[s]
    manifest=[]
    for typ,brand,action,model,uid,(proto,bits,khz,value,raw) in unique:
        offsets=[string(x) for x in (typ,brand,model,action)];off=len(pool)
        if raw:pool.extend(struct.pack('<'+'H'*len(raw),*raw))
        records.extend(struct.pack(FMT,uid,*offsets,value,off,len(raw),khz,proto,bits))
        manifest.append(dict(id=uid,type=typ,manufacturer=brand,action=action,model=model,protocol=proto))
    out=struct.pack('<4sII',b'PUL1',len(unique),12+len(records))+records+pool
    if len(out)>2300000:raise SystemExit('Catalog exceeds flash budget; select fewer source files explicitly.')
    args.output.mkdir(parents=True,exist_ok=True)
    (args.output/'catalog.bin').write_bytes(out)
    report=dict(source='https://github.com/Lucaslhm/Flipper-IRDB',commit=head,license='CC0-1.0 for post-cutoff files only',license_cutoff=LICENSE_COMMIT,source_files=len(sources),actions=len(unique),device_types=len(set(r[0] for r in unique)),manufacturers=len(set(r[1] for r in unique)),bytes=len(out),skipped_protocols=dict(skips),files=sources)
    (args.output/'catalog_report.json').write_text(json.dumps(report,indent=2))
    (args.output/'catalog_index.json').write_text(json.dumps(manifest,indent=2))
    (args.output/'FLIPPER_IRDB_LICENSE.txt').write_bytes((args.repo/'LICENSE').read_bytes())
    print(json.dumps({k:v for k,v in report.items() if k!='files'},indent=2))
if __name__=='__main__':main()
