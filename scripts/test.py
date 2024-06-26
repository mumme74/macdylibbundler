#!/usr/bin/env python3

from common import print, question
from time import sleep
import sys

import json


print("script started with:", file=sys.stderr)
for arg in sys.argv[1:]:
  print(arg, end='')
print("\n")

res = question("get_protocol")
print(f"Res:{str(res)}")

res = question("all_settings")
print(f"All settings:{res}")

frameworks = json.loads(question("all_settings"))
print(json.dumps(frameworks, indent=2))


print("script finished")
