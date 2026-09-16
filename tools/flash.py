#!/usr/bin/env python3
"""Flash the release images at their verified offsets; leaves NVS untouched."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--port',required=True);p.add_argument('--profile',choices=['full','simple'],required=True);p.add_argument('--baud',default='460800');a=p.parse_args()
    root=Path(__file__).resolve().parents[1]/'firmware'/a.profile
    manifest=json.loads((root/'manifest.json').read_text())
    args=[sys.executable,'-m','esptool','--chip','esp32','--port',a.port,'--baud',a.baud,'write_flash','--flash_mode','dio','--flash_freq','40m','--flash_size','4MB']
    for item in manifest['images']:
        path=root/item['file']
        if hashlib.sha256(path.read_bytes()).hexdigest()!=item['sha256']:p.error('Checksum mismatch: '+item['file'])
        args.extend([item['offset'],str(path)])
    return subprocess.call(args)
if __name__=='__main__':sys.exit(main())
