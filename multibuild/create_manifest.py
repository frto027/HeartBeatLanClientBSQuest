import sys
from pathlib import Path
import json
import requests
import re
import zipfile
import io
from common import load_json, latest_build_config_folder

PROJECT_ROOT = Path(__file__).parent.parent

def usage():
    print("usage: python ./multibuild/create_manifest.py <game version, e.g. 1.40.7>")

if not len(sys.argv) == 2:
    usage()
    exit(1)

VERSION_FOLDER = Path(__file__).parent / sys.argv[1]

if VERSION_FOLDER.exists():
    print("version already exists.")
    exit(1)

if VERSION_FOLDER.parent != Path(__file__).parent:
    print("invalid version")
    exit(1)


################ check the core mod repo and findout an avaliable version #########

coremod_manifest = load_json("https://raw.githubusercontent.com/QuestPackageManager/bs-coremods/refs/heads/main/core_mods.json")

found_version = None

for k in coremod_manifest:
    if k.startswith(sys.argv[1]):
        found_version = k

if found_version == None:
    print(f"can't found core mods for version {sys.argv[1]} ")
    exit(-1)

################ select an old config as dependency template to update ############

print(f"create mod manifest for version {found_version}")

template = latest_build_config_folder()
assert template != None
print(f"use template: {template}")


with (template / "qpm.json").open("r") as f:
    deps = json.load(f)["dependencies"]
# we need fill dep_latest_versions
dep_latest_version:dict[str,str] = {}
for dep in deps:
    dep_latest_version[dep['id']] = dep["versionRange"]


############ update core mods ############

for core in coremod_manifest[found_version]['mods']:
    if core['id'] in dep_latest_version:
        dep_latest_version[core['id']] = "^" + core['version']

############ update non-core mods ########

other_manifest = load_json("https://mods.bsquest.xyz/mods.json")

if not found_version in other_manifest:
    print("can't find mods for version " + found_version)
else:
    for mod in other_manifest[found_version]:
        id = mod['id']

        if id in dep_latest_version:
            ver = "^" + mod["version"] if dep_latest_version[id].startswith("^") else mod["version"]
            if dep_latest_version[id] == "" or dep_latest_version[id] < ver:
                dep_latest_version[id] = ver

for mod in other_manifest['global']:
    id = mod['id']
    if id in dep_latest_version:
        ver = "^" + mod["version"] if dep_latest_version[id].startswith("^") else mod["version"]
        if dep_latest_version[id] == "" or dep_latest_version[id] < ver:
            dep_latest_version[id] = ver

################ find an avaliable bs_cordl version ##############

def has_cordl_version(ver:str):
    cordl_versions = load_json("https://qpackages.com/bs-cordl?limit=0")
    for mod in cordl_versions:
        if mod['version'] == ver:
            return True
    return False

def try_get_bs_cordl_from_source_zip(zip_url):
    binary = requests.get(zip_url)
    if binary.status_code != 200:
        return None
    print(f"downloaded {len(binary.content)} bytes.")
    with zipfile.ZipFile(io.BytesIO(binary.content)) as file:
        game_version = ""
        bs_cordl_version = ""
        for f in file.filelist:
            if re.match(r"^[^/]+/mod.template.json$", f.filename):
                try:
                    j = json.loads(file.read(f.filename))
                    if "packageVersion" in j:
                        game_version = j["packageVersion"]
                except:
                    pass
            if re.match(r"^[^/]+/qpm.json$", f.filename):
                try:
                    j = json.loads(file.read(f.filename))
                    for dep in j["dependencies"]:
                        if dep["id"] == "bs-cordl":
                            bs_cordl_version = dep["versionRange"]
                except:
                    pass
        
        if game_version == found_version:
            return bs_cordl_version
    return None

def get_bs_cordl_from_mod_source(github_path, browser_download_url):
    for rel in load_json(f"https://api.github.com/repos/{github_path}/releases"):
        for asset in rel["assets"]:
            if "browser_download_url" in asset and asset["browser_download_url"] == browser_download_url:
                if "zipball_url" in rel:
                    ret = try_get_bs_cordl_from_source_zip(rel["zipball_url"])
                    if ret != None:
                        return ret
    return None
        
BETTER_MODS = ["PlayerDataKeeper", "UnicodeQuest","MetaCore","SongDownloader","Lapiz","PlaylistCore","Quest-SongCore","Quest-BSML","DanTheMan827-BeatSaber"]

def find_a_bs_cordl_version():
    better_mods:dict[str:object] = {}
    mods = []
    for mod in coremod_manifest[found_version]['mods']:
        m = re.match(r"https://github\.com/([^/]+/([^/]+))/.*", mod["downloadLink"])
        if m:
            obj = {
                "path":m[1],
                "link":mod["downloadLink"]
            }

            if m[2] in BETTER_MODS:
                better_mods[m[2]] = obj
            else:
                mods.append(obj)
    better_mods_flat = []
    for m in BETTER_MODS:
        if m in better_mods:
            better_mods_flat.append(better_mods[m])

    for mod in better_mods_flat + mods:
        print(f"try find bs_cordl version from {mod['link']}")
        ret = get_bs_cordl_from_mod_source(mod['path'],mod['link'])
        if ret:
            return ret
    return None
    # mods=[]


    # for mod in other_manifest[found_version]:
    #     m = re.match(r"https://github\.com/([^/]+/[^/]+)/.*", mod["source"])
    #     if m:
    #         mod["github_path"] = m[1]
    #         mods.append(mod)
    #         # print("try obtain bs-cordl version from " + mod["source"])
    #         # ret = get_bs_cordl_from_mod_source(m[1], mod["download"])
    #         # if ret != None:
    #         #     return ret

    # for mod in mods:
    #     print("try obtain bs-cordl version from " + mod["source"])
    #     ret = get_bs_cordl_from_mod_source(mod["github_path"], mod["download"])
    #     if ret != None:
    #         return ret

    return None

cordl_version = find_a_bs_cordl_version()

print(f"cordl version is {cordl_version}")
if cordl_version == None:
    print("not found, exit")
    exit(-1)

dep_latest_version['bs-cordl'] = cordl_version

for dep in deps:
    dep['versionRange'] = dep_latest_version[dep['id']]

####################### ok, it's done #############

VERSION_FOLDER.mkdir()
with (VERSION_FOLDER / 'qpm.json').open('w') as f:
    json.dump({
        "workspace.qmodOutput" : f"HeartBeatQuest.{sys.argv[1].replace(".","_")}.qmod",
        "dependencies":deps
    },f, indent=2)

with (VERSION_FOLDER / 'mod.template.json').open('w') as f:
    json.dump({
        "packageVersion":found_version
    },f, indent=2)

with (VERSION_FOLDER / 'target.cmake').open('w') as f:
    f.write(f'set(TARGET_GAME_VERSION "{sys.argv[1].replace(".","_")}")')

print("done")
