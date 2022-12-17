from dataclasses import dataclass
import numpy as np
import pandas as pd
import re
import subprocess
import sys
from typing import Any

exe: str = sys.argv[1]
min_point: int = 900
max_point: int = 25000

points: np.ndarray[Any, np.dtype[np.int32]] = np.linspace(
    min_point, max_point, 11, dtype=np.int32)


thread_nums: list[int] = [10, 20, 50, 100, 200]

cl_file: str = 'main.cl'

delta_re = re.compile(rb'^.*Milliseconds passed: (.+)$')
timing_re = re.compile(rb'^.*time: (.+) sec$')

timing_names = ['Generation', 'Map', 'Merge', 'Sort', 'Reduce']
iterations: int = 10

@dataclass
class ProcessResult:
    total: float
    timings: list[float]

def process_single(exe: str, thread_num: int, data_len: int) -> ProcessResult:
    res: subprocess.CompletedProcess = subprocess.run(
        [exe, str(thread_num), cl_file, str(data_len)], capture_output=True)

    total: float | None = None
    timings: list[float] = []

    for line in res.stdout.splitlines():
        match: re.Match[bytes] | None = re.fullmatch(delta_re, line)
        if match is not None:
            total = float(match.group(1))
        match = re.fullmatch(timing_re, line)
        if match is not None:
            timings.append(float(match.group(1)))

    if total is None:
        raise Exception(
                f"No delta found when running '{exe} {thread_num} {cl_file} {data_len}': {res.stdout.splitlines()}")
    if len(timings) != 5:
        raise Exception(
            f"Incorrect number of timings in output of '{exe} {thread_num} {cl_file} {data_len}': expected 5, but got {len(timings)}")

    return ProcessResult(total, timings)


def process(exe: str, thread_num: int):
    print(f"Measuring {exe} with {thread_num} threads...")

    total_results: list[pd.DataFrame] = []

    for point in points:
        for iteration in range(iterations):
            result: ProcessResult = process_single(exe, thread_num, point)
            print(f"{point};{result.total}")

            results_map: dict[str, float] = {
                'N': point,
                'T': result.total,
                'iter': iteration,
            }

            for name, timing in zip(timing_names, result.timings):
                results_map[name] = timing

            results_df: pd.DataFrame = pd.DataFrame([results_map])

            total_results.append(results_df)

    df: pd.DataFrame = pd.concat(total_results)
    df.set_index('N', inplace=True)
    print(df)
    df.to_csv(f"{exe}-{thread_num}.csv")


for thread_num in thread_nums:
    process(exe, thread_num)
