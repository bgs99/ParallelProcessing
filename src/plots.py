import pandas as pd

# TODO: separate measurements directory from output directory
# TODO: close matplotlib figures when they are not needed
# TODO: consider typing
# TODO: create directories needed for the output automatically

curr_folder = '../'

# Remove .csv
def remove_extension(file_name):
  return file_name[:-4]

# Get dataframe from file with format ['N', 'delta_ms']
def get_df(file_name='seq.csv', sep=';'):
  col_id = remove_extension(file_name)
  df = pd.read_csv(curr_folder + f'measurements/{file_name}', sep=sep, names=['N', f'{col_id}-delta'])
  return df

def save_df_to_csv(df, path):
  df.to_csv(path, encoding='utf-8')

# Create a general dataframe for each lab experiment
def get_table(lab_filenames: str) -> pd.DataFrame:
  lab_filenames.insert(0, 'lab1-gcc-seq.csv')
  df_lab = get_df(lab_filenames[0])

  for i in range(1, len(lab_filenames)):
    df_i = get_df(lab_filenames[i])
    # Merge two dataframes into one
    col_to_add = remove_extension(lab_filenames[i])
    col_to_add = f'{col_to_add}-delta'
    df_lab[col_to_add] = df_i[col_to_add]

  df_lab.set_index('N', inplace=True)

  return df_lab

# Calculate parallel accelerations for df_lab2
def get_accelerations_table(table: pd.DataFrame) -> pd.DataFrame:
  table['div_col'] = table['lab1-gcc-seq-delta'] # temporary column
  for col in table.columns:
    table[col] = table['div_col'].div(table[col])
  
  table.drop('div_col', axis=1, inplace=True)

  return table

from matplotlib import pyplot as plt

class PlotData:
    __data_df = pd.DataFrame()
    __title = ""
    __xlabel = ""
    __ylabel = ""

    def __init__(self, data_df, title="", xlabel="", ylabel=""):
        self.data_df = data_df
        self.title = title
        self.xlabel = xlabel
        self.ylabel = ylabel

    def set_title(self, title: str):
      self.title = title

    def set_xlabel(self, xlabel: str):
      self.xlabel = xlabel

    def set_ylabel(self, ylabel: str):
      self.ylabel = ylabel

    def plot(self, kind='bar', fig_width=12, fig_height=8):
      self.data_df.plot(kind=kind, figsize=(fig_width, fig_height))
      plt.title(self.title)
      plt.xlabel(self.xlabel)
      plt.ylabel(self.ylabel)

    def save_figure(self, save_folder):
      plt.savefig(save_folder)

schedules = ['static', 'dynamic', 'guided']
chunk_sizes = ['1', 'M-1', 'M', 'M+1']
n_threads = [1, 2, 3, 4, 6, 8] # num of threads used in lab3
lab3_schedule_filenames = []
lab3_no_schedule_filenames = [f'lab3-{n}.csv' for n in n_threads]

from collections import defaultdict

lab3_schedule_filenames = defaultdict(lambda: defaultdict(dict))
list_n_threads = []

for schedule in schedules:
  for chunk_size in chunk_sizes:
    for n in n_threads:
      if (schedule == 'dynamic') and (chunk_size == 'M-1'):
        break
      elif (schedule == 'static') and (chunk_size == 'M-1') and (n == 1):
        continue
      list_n_threads.append(f'lab3-{schedule}-{chunk_size}-{n}.csv')
    lab3_schedule_filenames[schedule][chunk_size] = list_n_threads
    list_n_threads = []

lab3_schedule_filenames

df_no_sched = get_table(lab3_no_schedule_filenames)

df_static_1 = get_table(lab3_schedule_filenames['static']['1'])
df_static_M_min_1 = get_table(lab3_schedule_filenames['static']['M-1'])
df_static_M = get_table(lab3_schedule_filenames['static']['M'])
df_static_M_plus_1 = get_table(lab3_schedule_filenames['static']['M+1'])

df_dynamic_1 = get_table(lab3_schedule_filenames['dynamic']['1'])
df_dynamic_M = get_table(lab3_schedule_filenames['dynamic']['M'])
df_dynamic_M_plus_1 = get_table(lab3_schedule_filenames['dynamic']['M+1'])

df_guided_1 = get_table(lab3_schedule_filenames['guided']['1'])
df_guided_M_min_1 = get_table(lab3_schedule_filenames['guided']['M-1'])
df_guided_M = get_table(lab3_schedule_filenames['guided']['M'])
df_guided_M_plus_1 = get_table(lab3_schedule_filenames['guided']['M+1'])

df_no_sched = get_accelerations_table(df_no_sched)

df_static_1 = get_accelerations_table(df_static_1)
df_static_M_min_1 = get_accelerations_table(df_static_M_min_1)
df_static_M = get_accelerations_table(df_static_M)
df_static_M_plus_1 = get_accelerations_table(df_static_M_plus_1)

df_dynamic_1 = get_accelerations_table(df_dynamic_1)
df_dynamic_M = get_accelerations_table(df_dynamic_M)
df_dynamic_M_plus_1 = get_accelerations_table(df_dynamic_M_plus_1)

df_guided_1 = get_accelerations_table(df_guided_1)
df_guided_M_min_1 = get_accelerations_table(df_guided_M_min_1)
df_guided_M = get_accelerations_table(df_guided_M)
df_guided_M_plus_1 = get_accelerations_table(df_guided_M_plus_1)

dfs_sched = []
dfs_sched.append(df_no_sched)
dfs_sched.append(df_static_1)
dfs_sched.append(df_static_M)
dfs_sched.append(df_static_M_plus_1)
dfs_sched.append(df_dynamic_1)
dfs_sched.append(df_dynamic_M)
dfs_sched.append(df_dynamic_M_plus_1)
dfs_sched.append(df_guided_1)
dfs_sched.append(df_guided_M)
dfs_sched.append(df_guided_M_plus_1)

# df_first:    The fist DataFrame to merge, used in the main loop
# dfs_compare: the list of DataFrames to merge with the first DataFrame
# skip_cols:  the list of df_main columns which need not to be compared
# return: list of DataFrames to plot,
#         list of filenames for each DataFrame, used for plot titles
def get_compare_data(df_first: pd.DataFrame, skip_cols: list, dfs_compare: list):
  dfs_to_plot = []
  col_names_per_df = []

  for i, col in enumerate(df_first.columns):
    if col in skip_cols:
      continue
    
    df_main = df_first.iloc[:, i].to_frame()
    col_names = []
    for df in dfs_compare:
      colname = df.iloc[:, i].name
      col_names.append(colname)
      # Merge dataframes into one
      df_main[colname] = df[colname]

    # Remove '-delta' substring
    col_names = [x.replace('-delta', '') for x in col_names]

    dfs_to_plot.append(df_main)
    col_names_per_df.append(col_names)
  
  return dfs_to_plot, col_names_per_df


# dfs_to_plot:      list of DataFrames to plot,
# col_names_per_df: list of filenames for each DataFrame, used for plot titles
# n_threads:        list of common number of threads used
# plot_info:        tuple containing the plot title, xlabel, ylabel, tuple of figure width and height
def plot_compare_data(dfs_to_plot: list, col_names_per_df: list, n_threads: list, 
                      plot_info: tuple, save_folder: str, measure_name: str):
  assert(len(dfs_to_plot) == len(n_threads))
  for i, df_main in enumerate(dfs_to_plot):
    # Bar plot
    title, xlabel, ylabel, figsize = plot_info
    # title = 'Comparison of accelerations between '
    for name in col_names_per_df[i]:
      title += name
      title += ', '
    title = title[:-2]
    
    bar_plot = PlotData(df_main, title, xlabel, ylabel)
    width, height = figsize
    bar_plot.plot('bar', width, height)

    bar_plot.save_figure(save_folder + f'/{measure_name}_par_{n_threads[i]}.png')

# Client - get compare data
dfs_to_plot, col_names_per_df = get_compare_data(df_no_sched, 
                                                ['lab1-gcc-seq-delta', 'lab3-1-delta'], 
                                                dfs_sched)
# Client - plot compare data
title = 'Comparison of accelerations between '
xlabel = 'N'
ylabel = 'Parallel acceleration S(p)'
plot_compare_data(dfs_to_plot, 
                  col_names_per_df,
                  [2, 3, 4, 6, 8],
                  (title, xlabel, ylabel, (20, 10)),
                  curr_folder + 'tasks/task6',
                  'acceleration')

Ns = [1, 2, 4, 6] # common number of threads between lab1 and lab2 experiments
lab1_filenames = [f'lab1-gcc-par-{n}.csv' for n in Ns]
lab2_filenames = [f'lab2-{n}.csv' for n in Ns]
lab3_filenames = [f'lab3-{n}.csv' for n in Ns]

df_lab1 = get_accelerations_table(get_table(lab1_filenames))
df_lab2 = get_accelerations_table(get_table(lab2_filenames))
df_lab3 = get_accelerations_table(get_table(lab3_filenames))

df_labs = [df_lab1, df_lab2, df_lab3]

# Client - get compare data
dfs_to_plot, col_names_per_df = get_compare_data(df_lab1, 
                                                ['lab1-gcc-seq-delta', 'lab3-1-delta'], 
                                                df_labs)
# Client - plot compare data
title = 'Comparison of accelerations between '
xlabel = 'N'
ylabel = 'Parallel acceleration S(p)'
plot_compare_data(dfs_to_plot, 
                  col_names_per_df,
                  [1, 2, 4, 6],
                  (title, xlabel, ylabel, (12, 8)),
                  curr_folder + 'tasks/task5',
                  'acceleration')

# Построить график парал. ускорения для точек N < N2.
# Найти значения N, при которых накладные расходы на распараллеливание превышают выигрыш от распараллеливания (независимо от различных видов расписания)
total_dfs = dfs_sched
total_dfs.append(df_static_M_min_1)
total_dfs.append(df_guided_M_min_1)

for df in total_dfs:
  df_name = df.columns[1][:-8]
  title = 'Paralell acceleration depending from thread number - ' + df_name
  xlabel = 'Number of Threads'
  ylabel = 'Parallel acceleration S(p)'
  line_plot = PlotData(df.T, title, xlabel, ylabel)
  line_plot.plot('line', 25, 10)

  line_plot.save_figure(curr_folder + f'tasks/task13/acc_over_thread_{df_name}.png')

# Ex. 9
n_threads = [1, 2, 3, 4, 6, 8]
lab3_filenames = [f'lab3-{n}.csv' for n in n_threads]

df_lab3 = get_table(lab3_filenames)
title = 'Time complexity before and after parallelization'
xlabel = 'N'
ylabel = 'Time (ms)'
bar_plot = PlotData(df_lab3, title, xlabel, ylabel)
bar_plot.plot('bar', 20, 10)

bar_plot.save_figure(curr_folder + 'tasks/task9/time_complexity.png')

# Ex. 14
n_threads = [1, 2, 3, 4, 6, 8]
dfs_compare = []

for i in range(0, 3):
  filename = [f'lab3--O{i}-{n}.csv' for n in n_threads]
  df = get_table(filename)
  dfs_compare.append(df)

# Client - get compare data
dfs_to_plot, col_names_per_df = get_compare_data(df_lab3, 
                                                ['lab1-gcc-seq-delta'], 
                                                dfs_compare)
# Client - plot compare data
title = 'Time complexity before and after parallelization '
xlabel = 'N'
ylabel = 'Time (ms)'
plot_compare_data(dfs_to_plot, 
                  col_names_per_df,
                  n_threads,
                  (title, xlabel, ylabel, (20, 10)),
                  curr_folder + 'tasks/task14',
                  'time_complexity')