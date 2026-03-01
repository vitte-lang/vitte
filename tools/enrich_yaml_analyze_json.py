#!/usr/bin/env python3
from __future__ import annotations
import json,time
from pathlib import Path

def pct(v,q):
    if not v:return 0.0
    xs=sorted(v); i=int(round((len(xs)-1)*q)); return float(xs[max(0,min(len(xs)-1,i))])

def main()->int:
    repo=Path(__file__).resolve().parents[1]; report=repo/'target/reports/yaml_analyze.json'; tel=repo/'.vitte-cache/vitte-ide-gtk/telemetry.log'; payload={}
    if report.exists():
        raw=report.read_text(encoding='utf-8').strip()
        try: payload=json.loads(raw) if raw else {}
        except json.JSONDecodeError: payload={'raw':raw}
    lat=[]
    if tel.exists():
        for line in tel.read_text(encoding='utf-8',errors='ignore').splitlines():
            if '|find_in_files|' not in line: continue
            for frag in line.split('|')[-1].split(','):
                if frag.startswith('ms='):
                    try: lat.append(float(frag[3:]))
                    except ValueError: pass
    payload['yaml_telemetry']={'generated_at':int(time.time()),'latency_p50_ms':pct(lat,0.5),'latency_p95_ms':pct(lat,0.95)}
    report.write_text(json.dumps(payload,indent=2,sort_keys=True)+'\n',encoding='utf-8'); return 0
if __name__=='__main__': raise SystemExit(main())
