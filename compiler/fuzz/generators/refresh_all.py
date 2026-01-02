# Deterministically regenerate large fuzz fixtures.
# Usage: python3 refresh_all.py <out_root>
import os, sys, random

random.seed(1)
out = sys.argv[1] if len(sys.argv) > 1 else "."
os.makedirs(out, exist_ok=True)

def w(path, txt):
    p = os.path.join(out, path)
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, "w", encoding="utf-8", newline="\n") as f:
        f.write(txt)

# regenerate a token-dense file
n = 2000
buf = ["prog lex_token_dense\n"]
for i in range(n):
    a = random.randint(0, 9999)
    b = random.randint(0, 9999)
    c = random.randint(0, 9999)
    buf.append(f"  let x{i} = ({a}+{b})*({c}-1)/3%7==0 && true || false\n")
buf.append("  ret\n.end\n")
w("lex/lex_token_dense.vitte", "".join(buf))

print("ok:", out)
