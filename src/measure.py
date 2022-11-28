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

thread_nums: list[int] = [1, 2, 3, 4, 6, 8]

delta_re = re.compile(rb'^.*Milliseconds passed: (.+)$')
timing_re = re.compile(rb'^.*time: (.+) sec$')

timing_names = ['Generation', 'Map', 'Merge', 'Sort', 'Reduce']

@dataclass
class ProcessResult:
    total: float
    timings: list[float]

def process_single(exe: str, thread_num: int, data_len: int) -> ProcessResult:
    res: subprocess.CompletedProcess = subprocess.run(
        [exe, str(thread_num), str(data_len)], capture_output=True)

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
            f"No delta found when running '{exe} {thread_num} {data_len}'")
    if len(timings) != 5:
        raise Exception(
            f"Incorrect number of timings in output of '{exe} {thread_num} {data_len}': expected 5, but got {len(timings)}")

    return ProcessResult(total, timings)


def process(exe: str, thread_num: int):
    print(f"Measuring {exe} with {thread_num} threads...")

    total_results: list[pd.DataFrame] = []

    for point in points:
        results: list[ProcessResult] = [
            process_single(exe, thread_num, point)
            for _i in range(10)
        ]
        min_idx = np.argmin([result.total for result in results])
        min_result = results[min_idx]
        min_delta = min_result.total
        print(f"{point};{min_delta}")

        results_map: dict[str, float] = {
            'N': point,
            'min': min_result.total,
        }

        for name, timing in zip(timing_names, min_result.timings):
            results_map[name] = timing

        results_df: pd.DataFrame = pd.DataFrame([results_map])

        total_results.append(results_df)

    df: pd.DataFrame = pd.concat(total_results)
    df.set_index('N', inplace=True)
    print(df)
    df.to_csv(f"{exe}-{thread_num}.csv")


for thread_num in thread_nums:
    process(exe, thread_num)
