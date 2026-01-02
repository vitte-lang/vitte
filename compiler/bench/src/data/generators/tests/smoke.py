import os, sys, tempfile, subprocess

THIS = os.path.dirname(__file__)
ROOT = os.path.normpath(os.path.join(THIS, ".."))

def run(cmd):
    p = subprocess.run(cmd, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if p.returncode != 0:
        sys.stderr.write(p.stderr)
        raise SystemExit(p.returncode)
    return p.stdout.strip()

def main():
    with tempfile.TemporaryDirectory() as td:
        out = os.path.join(td, "gen")
        run([sys.executable, "gen_tokens.py", "--out", out, "--seed", "1", "--n", "10", "--name", "t"])
        run([sys.executable, "gen_parse.py", "--out", out, "--seed", "1", "--depth", "10", "--match", "10"])
        run([sys.executable, "gen_lower.py", "--out", out, "--n", "10"])
        run([sys.executable, "gen_sema.py", "--out", out, "--n", "10"])
        run([sys.executable, "gen_trivia.py", "--out", out, "--seed", "1", "--n", "10"])
        run([sys.executable, "gen_paths.py", "--out", out])
        run([sys.executable, "gen_numbers.py", "--out", out])
        run([sys.executable, "gen_unicode_idents.py", "--out", out])
        print("ok")

if __name__ == "__main__":
    main()
