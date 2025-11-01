#!/usr/bin/env python3
import subprocess

CASES = r'''
$ zscore asdf n1
nil
$ zquery xxx 1 asdf 1 10
array length: 0
array end
$ zadd zset 1 n1
1
$ zadd zset 2 n2
1
$ zadd zset 1.1 n1
0
$ zscore zset n1
1.1
$ zquery zset 1 "" 0 10
array length: 4
n1
1.1
n2
2
array end
$ zquery zset 1.1 "" 1 10
array length: 2
n2
2
array end
$ zquery zset 1.1 "" 2 10
array length: 0
array end
$ zrem zset adsf
0
$ zrem zset n1
1
$ zquery zset 1 "" 0 10
array length: 2
n2
2
array end
'''

# Parse commands and expected outputs
cmds, outputs = [], []
for x in CASES.splitlines():
    x = x.strip()
    if not x:
        continue
    if x.startswith('$ '):
        cmds.append(x[2:])
        outputs.append('')
    else:
        outputs[-1] += x + '\n'

assert len(cmds) == len(outputs)

# Launch client in REPL mode
process = subprocess.Popen(
    ['../bin/client'],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    text=True
)

# Prepare all input
full_input = ""
expected_output = ""
for cmd, expect in zip(cmds, outputs):
    full_input += cmd + "\n"
    expected_output += expect
full_input += "exit\n"

# Run client and capture output
output, error = process.communicate(full_input)

# Compare
if output != expected_output:
    print("❌ Mismatch detected")
    print("---- Output ----")
    print(output)
    print("---- Expected ----")
    print(expected_output)
    raise SystemExit(1)

print("✅ All REPL tests passed.")
