#!/bin/bash
# A quick smoke test to make sure publish_toolstate.py works.

set -euo pipefail
IFS=$'\n\t'

rm -rf -toolstate
git clone --depth=1 https://github.com/-lang-nursery/-toolstate.git
cd -toolstate
python3 "../../src/tools/publish_toolstate.py" "$(git rev-parse HEAD)" \
    "$(git log --format=%s -n1 HEAD)" "" ""
cd ..
rm -rf -toolstate
