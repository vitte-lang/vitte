from pathlib import Path
lines = Path('bench/CMakeLists.txt').read_text().splitlines()
for i, line in enumerate(lines,1):
    if 220 <= i <= 260:
        print(f"{i}: {line}")
