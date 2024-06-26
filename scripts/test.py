#!/usr/bin/env python3

#from common import *
from time import sleep

import sys, json
import builtins

def question(data):
  if isinstance(data, str):
    data = data.encode('utf8')
  sz = len(data)
  sys.stdout.buffer.write(sz.to_bytes(sz.bit_length(), 'big'))
  sys.stdout.buffer.write(data)
  sys.stdout.buffer.flush()
  print("sent to parent, starting to read from input\n")

  # read size bigendian
  buf = sys.stdin.buffer.read(4)
  sz = int.from_bytes(buf, 'big')
  return sys.stdin.buffer.read(sz)

def print(*args, **kwargs):
  """Print to stderr"""
  kwargs['file'] = sys.stderr
  kwargs['flush'] = True
  return builtins.print(*args, **kwargs)


print("script started with:", file=sys.stderr)
for arg in sys.argv[1:]:
  print(arg, end='')
print("\n")

res = question("get_protocol")
print(f"Res:{res}")

#sleep(3)
# cmds = [
#   "py:ge mig commando1",
#   "py:ge mig commando2",
#   "py:ge mig commando3"]

# fifo= Fifo()

# for cmd in cmds:
#   print(f"sending cmd {cmd}")
#   fifo.write(cmd)
#   print(fifo.read())



# fifo = Fifo()
# req = json.dumps({"cmd":"all_commands"}, indent=2)
# print("sending:", req,  file=sys.stderr)
# fifo.write(req)
# msg = fifo.read()
# print("received:", msg,  file=sys.stderr)
# resp = json.loads(msg)
# print(dir(resp),  file=sys.stderr)


print("script finished")
