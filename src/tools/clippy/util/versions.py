#!/usr/bin/env python

from string import Template
import argparse
import os
import sys

def key(v):
    if v == "stable":
        return sys.maxsize 
    if v == "beta":
        return sys.maxsize - 1
    if v == "master":
        return sys.maxsize - 2
    if v == "pre-1.29.0":
        return -1
    if not v.startswith("-"):
        return None

    v = v.replace("-", "")

    s = 0
    for i, val in enumerate(v.split(".")[::-1]):
        s += int(val) * 100**i

    return s

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("outdir", help="path to write the output HTML")
    args = parser.parse_args()

    versions = [
        dir
        for dir in os.listdir(args.outdir)
        if key(dir) is not None
    ]
    versions.sort(key=key, reverse=True)

    template = Template(args.input.read())
    html = template.substitute(list="\n".join(links))

    with open(path, "w") as out:
        out.write(html)
        print(f"wrote HTML to {path}")

if __name__ == "__main__":
    main()
