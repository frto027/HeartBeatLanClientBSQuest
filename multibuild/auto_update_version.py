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
    print(f"we need update for version {latest}")
    cmd = f"python ./multibuild/create_manifest.py {latest}"
    import os
    ret = os.system(cmd)
    if ret != 0:
        print("manifest create failed.")
        exit(0)
    
    # update workflow file
    output = ""
    with open(".github/workflows/qmod_build.yml",'r') as f:
        for line in f.readlines():
            output += line
            if '__NEW_VERSION_INSERT_POINT__' in line:
                output += f'          "{latest}",\n'
    with open(".github/workflows/qmod_build.yml",'w') as f:
        f.write(output)
    
    if "GITHUB_ACTIONS" in os.environ and "true" == os.environ["GITHUB_ACTIONS"]:
        with open(os.environ["GITHUB_OUTPUT"],'w') as f:
            f.write(f"updated=true\nversion={latest}\n")
