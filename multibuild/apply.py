import sys
from pathlib import Path

MOD_VERSION = "0.3.2"

PROJECT_ROOT = Path(__file__).parent.parent

def usage():
    print("usage: ./multibuild/apply.py <game version, e.g. 1.40.6>")

if not len(sys.argv) == 2:
    usage()
    exit(1)

VERSION_FOLDER = Path(__file__).parent / sys.argv[1]

if not VERSION_FOLDER.exists():
    print("version not exists.")
    exit(1)
if VERSION_FOLDER.parent != Path(__file__).parent:
    print("invalid version")
    exit(1)

for f in VERSION_FOLDER.glob("*"):
    t = PROJECT_ROOT / f.name
    print(f"copy {f} to {t}")
    with t.open("wb") as _t:
        with f.open("rb") as _f:
            _t.write(_f.read())
    
