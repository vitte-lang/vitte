from pathlib import Path
lines = Path('compiler/tests/unit/test_muf_rust.c').read_text().splitlines()
for i, line in enumerate(lines,1):
    print(f"{i}: {line}")
