#!/usr/bin/env python3
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
EN = ROOT / 'locales' / 'en'
LOCALES_DIR = ROOT / 'locales'


def parse_ftl(path: Path):
    out = {}
    for raw in path.read_text(encoding='utf-8').splitlines():
        line = raw.strip()
        if not line or line.startswith('#') or '=' not in line:
            continue
        key, value = line.split('=', 1)
        out[key.strip()] = value.strip()
    return out


def collect_keys():
    en_msgs = parse_ftl(EN / 'diagnostics.ftl')
    en_explain = parse_ftl(EN / 'diagnostics_explain.ftl')
    core_keys = set(en_msgs.keys())
    explain_keys = set(en_explain.keys())
    locales = sorted([p.name for p in LOCALES_DIR.iterdir() if p.is_dir()])
    report_lines = []
    report_lines.append(f'Locales: {len(locales)}')
    report_lines.append(f'Core keys (en): {len(core_keys)}')
    report_lines.append(f'Explain keys (en): {len(explain_keys)}')
    report_lines.append('')
    for loc in locales:
        loc_path = LOCALES_DIR / loc
        m = loc_path / 'diagnostics.ftl'
        e = loc_path / 'diagnostics_explain.ftl'
        if not m.exists() or not e.exists():
            report_lines.append(f'{loc}: MISSING FILES: diagnostics.ftl present={m.exists()} explain present={e.exists()}')
            continue
        msgs = parse_ftl(m)
        explains = parse_ftl(e)
        missing_msgs = sorted(list(core_keys - set(msgs.keys())))
        missing_expl = sorted([k for k in (explain_keys - set(explains.keys()))])
        extra_msgs = sorted(list(set(msgs.keys()) - core_keys))
        extra_expl = sorted(list(set(explains.keys()) - explain_keys))
        report_lines.append(f'Locale {loc}: msgs={len(msgs)} missing_msgs={len(missing_msgs)} extra_msgs={len(extra_msgs)} explain={len(explains)} missing_explain={len(missing_expl)} extra_explain={len(extra_expl)}')
        if missing_msgs:
            report_lines.append('  Missing message keys:')
            for k in missing_msgs[:20]:
                report_lines.append('   - '+k)
            if len(missing_msgs) > 20:
                report_lines.append(f'   ... and {len(missing_msgs)-20} more')
        if missing_expl:
            report_lines.append('  Missing explain keys:')
            for k in missing_expl[:20]:
                report_lines.append('   - '+k)
            if len(missing_expl) > 20:
                report_lines.append(f'   ... and {len(missing_expl)-20} more')
    return '\n'.join(report_lines)


if __name__ == '__main__':
    print(collect_keys())
