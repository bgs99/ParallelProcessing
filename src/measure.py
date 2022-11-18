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


def process_single(exe: str, thread_num: int, data_len: int) -> float:
    res: subprocess.CompletedProcess = subprocess.run(
        [exe, str(thread_num), str(data_len)], capture_output=True)
    for line in res.stdout.splitlines():
        match: re.Match[bytes] | None = re.fullmatch(delta_re, line)
        if match is None:
            continue
        return float(match.group(1))
    raise Exception(
        f"No delta found when running '{exe} {thread_num} {data_len}'")


def process(exe: str, thread_num: int):
    print(f"Measuring {exe} with {thread_num} threads...")

    results: list[pd.DataFrame] = []

    for point in points:
        deltas: list[float] = [
            process_single(exe, thread_num, point)
            for _i in range(10)
        ]
        min_deltas: float = min(deltas)
        print(f"{point};{min_deltas}")

        deltas_df: pd.DataFrame = pd.DataFrame([deltas])
        deltas_df['min'] = [min_deltas]

        results.append(deltas_df)

    df: pd.DataFrame = pd.concat(results)
    df.insert(0, 'N', points)
    print(df)
    df.to_csv(f"{exe}-{thread_num}.csv")


for thread_num in thread_nums:
    process(exe, thread_num)
