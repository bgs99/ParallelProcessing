import os
import pandas as pd
from plot_data import PlotData
import matplotlib.pyplot as plt
from pathlib import Path

# TODO: consider typing

# Change to your src folder
src_folder = '../'

# Remove .csv


def remove_extension(file_name):
    return file_name[:-4]

# Get dataframe from file


def get_df(file_name='seq.csv', file_dir='measurements', sep=',', names=[]) -> pd.DataFrame:
    script_dir = os.path.dirname(src_folder)
    results_dir = os.path.join(script_dir, file_dir)
    skiprows = 1

    if not names:
        col_id = remove_extension(file_name)
        names = ['N', f'{col_id}-delta']
        skiprows = 0

    df = pd.read_csv(results_dir + '/' + file_name,
                        skiprows=skiprows, sep=sep, names=names)
    return df



def get_time_plot(df_lab5: pd.DataFrame, df_lab4: pd.DataFrame, num_thread: int):
    df = pd.DataFrame()
    df['N'] = df_lab5['N']
    df['delta_ms_lab4'] = df_lab4['min']
    df['delta_ms_lab5'] = df_lab5['min']
    df.set_index('N', inplace=True)

    title = f'Time complexity lab4 and lab5 (num threads={num_thread})'
    xlabel = 'N'
    ylabel = 'Time (ms)'
    bar_plot = PlotData(df, title, xlabel, ylabel)
    bar_plot.plot('bar', 20, 10)
    bar_plot.save_figure(file_dir=src_folder + '/build/graphs/lab5/task3.1',
                         filename=f'Time_complexity_threads_{num_thread}')


def get_accelerations_plot(df_lab5: pd.DataFrame, df_lab4: pd.DataFrame, num_thread: int):
    df = pd.DataFrame()
    df['N'] = df_lab5['N']
    df['acceleration_lab4'] = df_lab4['acc']
    df['acceleration_lab5'] = df_lab5['acc']
    df.set_index('N', inplace=True)

    title = f'Accelelrations lab4 and lab5 (num threads={num_thread})'
    xlabel = 'N'
    ylabel = 'Acceleration'

    bar_plot = PlotData(df, title, xlabel, ylabel)
    bar_plot.plot('bar', 20, 10)
    bar_plot.save_figure(file_dir=src_folder + '/build/graphs/lab5/task3.2',
                         filename=f'Compare_accelerations_threads_{num_thread}')


def get_exec_time_plot(df_lab5: pd.DataFrame, df_lab4: pd.DataFrame, num_thread: int):
    part_names = ['Generation', 'Map', 'Merge', 'Sort', 'Reduce']

    def get_norm_acc(df: pd.DataFrame) -> pd.DataFrame:
        result = df.loc[:, part_names]
        result = result.div(result.sum(axis=1), axis=0)
        result['N'] = df['N']
        return result

    def plot_exec_time(dfs: list[pd.DataFrame], num_thread: int):
        df = pd.concat(dfs).sort_values('N')
        df['N'] = df['N'].astype(str) + ', ' + df['label']
        df.drop('label', axis=1)
        df.plot(x='N', kind='bar', stacked=True,
                     title=f'Execution time by steps by N (threads={num_thread})', figsize=(20, 15))
        plt.ylabel("Time (ms)")
        results_dir = src_folder + \
            f'/build/graphs/lab5/task3.3/'
        path = Path(results_dir)
        path.mkdir(parents=True, exist_ok=True)

        plt.savefig(results_dir + '/' +
                    f'exec_time_steps_threads_{num_thread}')
        plt.close()

    df_exec_5 = get_norm_acc(df_lab5)
    df_exec_5['label'] = 'lab5'
    df_exec_4 = get_norm_acc(df_lab4)
    df_exec_4['label'] = 'lab4'


    plot_exec_time([df_exec_5, df_exec_4], num_thread)


def main():
    n_threads = [1, 2, 3, 4, 6, 8]
    names = ['N', 'min', 'Generation', 'Map', 'Merge', 'Sort', 'Reduce']

    for n in n_threads:
        df_lab5 = get_df(file_name=f'lab5-{n}.csv', names=names)
        df_lab4 = get_df(file_name=f'lab5_4-{n}.csv', names=names)

        # Calculate parallel accelerations
        base = get_df('lab1-gcc-seq.csv', sep=';')['lab1-gcc-seq-delta']
        df_lab5['acc'] = base /df_lab5['min']
        df_lab4['acc'] = base / df_lab4['min']

        get_time_plot(df_lab5, df_lab4, n)
        get_accelerations_plot(df_lab5, df_lab4, n)
        get_exec_time_plot(df_lab5, df_lab4, n)


if __name__ == "__main__":
    main()
