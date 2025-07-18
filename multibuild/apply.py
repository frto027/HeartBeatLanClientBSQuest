import sys
from pathlib import Path
import json

PROJECT_ROOT = Path(__file__).parent.parent

def usage():
    print("usage: python ./multibuild/apply.py <game version, e.g. 1.40.6>")

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
    if f.name.endswith(".json"):
        with f.open('r') as fp:
            _tmp = json.load(fp)
        with t.open('r') as fp:
            _target = json.load(fp)
        print(f"overwriting keys from file: {f}")
        for k in _tmp:
            print(f"  key '{k}' overwritten")
            keys = k.split(".")

            #_target[k] = _tmp[k]
            right = _tmp
            left = _target
            for kk in keys[:-1]:
                left = left[kk]
            left[keys[-1]] = right[k]
            
        with t.open('w') as fp:
            json.dump(_target, fp)
        continue

    print(f"copy {f} to {t}")
    with t.open("wb") as _t:
        with f.open("rb") as _f:
            _t.write(_f.read())
    
