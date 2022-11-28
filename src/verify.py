#!/usr/bin/python

import math
import sys
import re
import os
import subprocess
from typing import List

if len(sys.argv) < 3:
    raise Exception("Expected 2 executables to compare")

x_re = re.compile(rb'^X=(.*)$')
arg_len = 1000;

def proc_to_xs(proc: str, additional_arg: str | None) -> List[float]:
    results: List[float] = []
    to_run: List[str] = [proc, str(arg_len)] if additional_arg is None else [proc, additional_arg, str(arg_len)]
    res: subprocess.CompletedProcess = subprocess.run(
        to_run, capture_output=True)

    for line in res.stdout.splitlines():
        match: re.Match[bytes] | None = re.fullmatch(x_re, line)
        if match is None:
            continue
        results.append(float(match.group(1)))

    return results


xs_1: List[float] = proc_to_xs(sys.argv[1], None)
xs_2: List[float] = proc_to_xs(sys.argv[2], sys.argv[3])

if len(xs_1) != len(xs_2):
    raise Exception(f"Length mismatch: {len(xs_1)} vs {len(xs_2)}")

total_diff: float = 0.0
max_diff: float = 0.0
for (i, (x1, x2)) in enumerate(zip(xs_1, xs_2)):
    if math.isinf(x1) and not math.isinf(x2):
        print(f"inf mismatch on line {i}: {x1} vs {x2}")
        total_diff += math.inf
        max_diff += math.inf
        continue
    diff: float = abs(x1 - x2) / x1
    if math.isnan(diff):
        continue
    if diff != 0:
        print(f"mismatch on line {i} of {diff}: {x1} vs {x2}")
    if diff > max_diff:
        max_diff = diff
    total_diff += diff

average_diff: float = total_diff / len(xs_1)

print(f'Relative diff average: {average_diff}, max: {max_diff}')
