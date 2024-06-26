import sys, builtins

def question(data):
  """Send and receive from parent process"""
  if isinstance(data, str):
    data = data.encode('utf8')
  sz = len(data)
  sys.stdout.buffer.write(sz.to_bytes(sz.bit_length(), 'big'))
  sys.stdout.buffer.write(data)
  sys.stdout.buffer.flush()
  print("sent to parent, starting to read from input\n")

  # read size bigendian
  buf = sys.stdin.buffer.read(4)
  #print(f"reading {buf} from input\n")
  sz = int.from_bytes(buf, 'big')
  #print(f"reading {sz} bytes from input\n")
  return sys.stdin.buffer.read(sz)

def print(*args, **kwargs):
  """Print to stderr"""
  kwargs['file'] = sys.stderr
  kwargs['flush'] = True
  return builtins.print(*args, **kwargs)
