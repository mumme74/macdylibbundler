#!/usr/bin/env python3

from test_common import *

import sys

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

print("script finished")
