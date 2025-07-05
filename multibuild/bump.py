from pathlib import Path
import sys

if not len(sys.argv) == 2:
    print("usage: bump.py <version>\n\te.g.\n\t\tbump.py 0.3.3")

MOD_VERSION = sys.argv[1]

for version in Path(__file__).parent.glob("*"):
    QPM_JSON = version / "qpm.json"
    if not QPM_JSON.exists():
        continue
    import json

    with QPM_JSON.open("r") as f:
        qpm_json = json.load(f)
    if qpm_json["info"]["version"] != MOD_VERSION:
        qpm_json["info"]["version"] = MOD_VERSION
        print(f"{QPM_JSON}: version not consist, fix it")
        with QPM_JSON.open("w") as f:
            json.dump(qpm_json, f)
