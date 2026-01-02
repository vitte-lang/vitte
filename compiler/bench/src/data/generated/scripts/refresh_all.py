# Deterministically (re)generate all datasets under bench/src/data/generated.
# Usage: python3 refresh_all.py <out_root>
import os, sys, random

seed = 1
random.seed(seed)

out = sys.argv[1] if len(sys.argv) > 1 else "."
def w(path, txt):
    p = os.path.join(out, path)
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, "w", encoding="utf-8", newline="\n") as f:
        f.write(txt)

# lex: token dense
n = 4000
buf = ["prog gen_lex_token_dense\n"]
for i in range(n):
    a = random.randint(0, 9999)
    b = random.randint(0, 9999)
    c = random.randint(0, 9999)
    buf.append(f"  let x{i} = ({a}+{b})*({c}-1)/3%7==0 && true || false\n")
buf.append("  ret\n.end\n")
w("lex/token_dense_4k.vitte", "".join(buf))

# parse: deep blocks
depth = 200
buf = ["prog gen_parse_deep\n"]
for i in range(depth):
    buf.append("  " * (i % 6) + "when true\n")
buf.append("  ret\n.end\n")
w("parse/deep_blocks_200.vitte", "".join(buf))

# lower: surface constructs
buf = ["prog gen_lower\n"]
for i in range(3000):
    buf.append(f"  set x{i} = {i}\n")
    if i % 10 == 0:
        buf.append("  when true\n    do\n      say \"branch\"\n    .end\n  .end\n")
buf.append("  ret\n.end\n")
w("lower/surface_3k.vitte", "".join(buf))

# sema: many decls
buf = ["mod gen_sema\n"]
for i in range(1200):
    buf.append(f"  fn f{i}(x:i32)->i32\n    ret x + {i}\n  .end\n")
buf.append(".end\n")
w("sema/many_decls_1200.vitte", "".join(buf))

# proj: multi-file project
proj = os.path.join(out, "proj/project_64")
os.makedirs(proj, exist_ok=True)
with open(os.path.join(proj, "main.vitte"), "w", encoding="utf-8", newline="\n") as f:
    f.write("prog proj\n  ret\n.end\n")
for i in range(64):
    with open(os.path.join(proj, f"m{i}.vitte"), "w", encoding="utf-8", newline="\n") as f:
        f.write(f"mod proj.m{i}\n")
        for k in range(40):
            f.write(f"  fn g{k}(a:i32)->i32\n    ret a+{k}\n  .end\n")
        f.write(".end\n")

print("ok (seed=%d): %s" % (seed, out))
