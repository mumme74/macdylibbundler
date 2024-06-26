#!/usr/bin/env python3

from common import *

import sys, json

print("script started with:")

for arg in sys.argv[1:]:
  print(arg, end='')
print("\n")

cmds = [
  "py:ge mig commando1",
  "py:ge mig commando2",
  "py:ge mig commando3"]

fifo= Fifo()

for cmd in cmds:
  print(f"sending cmd {cmd}")
  fifo.write(cmd)
  print(fifo.read())



# fifo = Fifo()
# req = json.dumps({"cmd":"all_commands"}, indent=2)
# print("sending:", req,  file=sys.stderr)
# fifo.write(req)
# msg = fifo.read()
# print("received:", msg,  file=sys.stderr)
# resp = json.loads(msg)
# print(dir(resp),  file=sys.stderr)


print("script finished")
