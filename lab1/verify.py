#!/usr/bin/python

import sys
import re
import subprocess
from typing import List

if len(sys.argv) < 3:
    raise Exception("Expected 2 executables to compare")

x_re = re.compile(rb'^X=(.*)$')


def proc_to_xs(proc: str) -> List[float]:
    results: List[float] = []
    icc_ld_lib_path = "/home/bgs99/Downloads/icc/intel/oneapi/compiler/2022.1.0/linux/compiler/lib/intel64"
    res: subprocess.CompletedProcess = subprocess.run(
        [proc, "1000"], capture_output=True, env={'LD_LIBRARY_PATH': icc_ld_lib_path})

    for line in res.stdout.splitlines():
        match: re.Match[bytes] | None = re.fullmatch(x_re, line)
        if match is None:
            continue
        results.append(float(match.group(1)))

    return results


xs_1: List[float] = proc_to_xs(sys.argv[1])
xs_2: List[float] = proc_to_xs(sys.argv[2])

total_diff: float = 0.0
max_diff: float = 0.0
for (x1, x2) in zip(xs_1, xs_2):
    diff: float = abs(x1 - x2) / x1
    if diff > max_diff:
        max_diff = diff
    total_diff += diff

average_diff: float = total_diff / len(xs_1)

print(f'Relative diff average: {average_diff}, max: {max_diff}')

if average_diff > 2e-6 or max_diff > 1e-5:
    raise Exception("Diff is too big")
