import json
with open("qpm.json") as f:
    _this = json.load(f)
version = _this["info"]["version"]
print(f"version=v{version}")
