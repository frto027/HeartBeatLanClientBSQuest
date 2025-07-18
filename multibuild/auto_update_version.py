import re
from common import load_json, latest_build_config_folder

#### get the remote latest game version ####
versions = load_json("https://mods.bsquest.xyz/versions.json")
latest = None
for v in versions:
    if latest == None:
        latest = v
    elif re.match(r"^\d+\.\d+\.\d+(_\d+)?$", v) and v > latest:
        latest = v
if "_" in latest:
    latest = latest.split("_")[0]

#### local latest ####

local_latest = latest_build_config_folder().name

#### update if needed ####

if local_latest != latest:
    print("we need update")
    cmd = f"python ./multibuild/create_manifest.py {latest}"
    import os
    ret = os.system(cmd)
    if ret != 0:
        exit(ret)
    if "true" == os.environ["GITHUB_ACTIONS"]:
        with open(os.environ["GITHUB_OUTPUT"],'w') as f:
            f.write(f"updated=true\nversion={latest}\n")