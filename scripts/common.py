import sys, os, json

if len(sys.argv) < 2:
  print("Not enough arguments given to {}".format(sys.argv[0]), file=sys.stderr)
  exit(1)
if not os.path.exists(sys.argv[1]):
  print("File {} does not exist".format(sys.argv[1]), file=sys.stderr)
  exit(1)

jsonData=None
try:
  with open(sys.argv[1], 'r') as jsonFile:
    jsonData = json.load(jsonFile)
except JSONDecodeError as e:
  print("Error during parsing json file {}\n{}".format(sys.argv[1], e))
  exit(1)
