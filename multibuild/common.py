import json
import requests
from pathlib import Path
import re


def load_json(url:str):
    for _ in range(3):
        try:
            r = requests.get(url)
            if r.status_code != 200:
                continue
            return json.loads(r.text)
        except:
            pass
    raise RuntimeError("Can't download " + url)

def latest_build_config_folder()->Path:
    template = None
    for f in Path(__file__).parent.glob("*.*.*"):
        if not re.match(r"^\d+\.\d+\.\d+$",f.name):
            continue
        if template == None:
            template = f
        elif template.name < f.name:
            template = f
    if template == None:
        return None
    return template