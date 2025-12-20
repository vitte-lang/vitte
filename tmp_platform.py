from pathlib import Path
lines = Path('bench/src/bench/platform.h').read_text().splitlines()
for i, line in enumerate(lines,1):
    if 70 <= i <= 120 or 180 <= i <= 220:
        print(f"{i}: {line}")
