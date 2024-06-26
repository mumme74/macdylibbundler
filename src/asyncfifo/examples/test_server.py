#!/usr/bin/env python3
from test_common import *

fifo = Fifo(".from.fifo", ".to.fifo")

iter = 0
while True:
  cmd = fifo.read()
  if cmd == "": break
  print(f"request from: client {cmd}")
  cmdTo = f"response to {cmd}"
  print("sending: ",cmdTo)
  fifo.write(cmdTo)
  iter += 0