from dataclasses import dataclass
from typing import Any, Iterable, Literal
from matplotlib import pyplot as plt
from collections import defaultdict
import pandas as pd
import scipy.stats as st
import numpy as np

# TODO: separate measurements directory from output directory
# TODO: close matplotlib figures when they are not needed
# TODO: consider typing
# TODO: create directories needed for the output automatically

# Remove .csv


def remove_extension(file_name):
    return file_name[:-4]


def get_lab4_df(name: str, threads: int) -> pd.DataFrame:
    return pd.read_csv(f'../measurements/{name}-{threads}.csv', sep=',', header=0, index_col=0)


min_point: int = 900
max_point: int = 25000

points: np.ndarray[Any, np.dtype[np.int32]] = np.linspace(
    min_point, max_point, 11, dtype=np.int32)

n_threads: list[int] = [1, 2, 3, 4, 6, 8]

lab4_dfs: dict[int, pd.DataFrame] = {
    threads: get_lab4_df('lab4', threads)
    for threads in n_threads}


def calculate_parallel_accelerations(base_df: pd.DataFrame, df: pd.DataFrame, columns: dict[str, str] = {'min': 'acceleration'}):
    for key, value in columns.items():
        df[value] = base_df['min'] / df[key]


for df in lab4_dfs.values():
    calculate_parallel_accelerations(lab4_dfs[1], df)

def plot_threads_comparison(
        data_to_plot: dict[int, pd.DataFrame],
        column_to_plot: str,
        title: str,
        save_folder: str,
        measure_name: str):

    dfs: dict[int, pd.Series] = {threads: df[column_to_plot] for threads, df in  data_to_plot.items()}
    pd.DataFrame(dfs).T.plot.line()
    plt.title(title)
    plt.xlabel('Number of threads')
    plt.ylabel('Parallel acceleration')
    plt.savefig(f'{save_folder}/{measure_name}.png')
    plt.close()

        

plot_threads_comparison(
    lab4_dfs,
    'acceleration',
    'Parallelisation overhead for different N',
    '../build/graphs/lab4',
    'losses'
)

def plot_compare_data(
    dfs_to_plot: dict[int, pd.DataFrame],
        col_name: str,
        title: str,
        save_folder: str,
        measure_name: str):

    combined_df = pd.DataFrame(data=[], index=points)

    for name, df in dfs_to_plot.items():
        combined_df[name] = df[col_name]

    combined_df.plot.bar(width=0.75)

    plt.title(title)
    plt.xlabel('N')
    plt.ylabel('Parallel acceleration')
    plt.savefig(f'{save_folder}/{measure_name}.png')
    plt.close()


plot_compare_data(lab4_dfs, 'acceleration',
                  'Comparison of accelerations',
                  '../build/graphs/lab4',
                  'acceleration')

lab4_int_dfs: dict[int, pd.DataFrame] = {
    threads: get_lab4_df('lab4-int', threads)
    for threads in n_threads}


def calculate_student(df: pd.DataFrame):
    data: pd.DataFrame = df.loc[:, '0':'9'].T
    mean: pd.Series = data.mean(axis=0)
    scale: pd.Series = st.sem(data)
    low, high = st.t.interval(
        alpha=0.95, df=10,
        loc=mean,
        scale=scale)
    df['low'] = low
    df['high'] = high


def plot_line_data(
        data_to_plot: dict[int, pd.DataFrame],
        columns_to_plot: list[str],
        title: str,
        save_folder: str,
        measure_name: str):

    for point in points:
        point = int(point)
        dfs: dict[int, pd.Series] = {threads: df[columns_to_plot].loc[point] for threads, df in  data_to_plot.items()}

        pd.DataFrame(dfs).T.plot.line()
        plt.title(f'{title}, N={point}')
        plt.xlabel('Number of threads')
        plt.ylabel('Parallel acceleration')
        plt.savefig(f'{save_folder}/{measure_name}-{point}.png')
        plt.close()

for threads, df in lab4_int_dfs.items():
    calculate_student(df)
    calculate_parallel_accelerations(
        lab4_int_dfs[1], df, {'min': 'max_acc', 'low': 'high_acc', 'high': 'low_acc'})

plot_line_data(
    lab4_int_dfs,
    ['max_acc', 'low_acc', 'high_acc'],
    f'Comparison of methods to measure time',
    '../build/graphs/lab4',
    f'time_comparison')
