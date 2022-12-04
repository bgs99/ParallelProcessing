# import os
from pathlib import Path
from matplotlib import pyplot as plt
import pandas as pd

class PlotData:
    __data_df = pd.DataFrame()
    __title = ""
    __xlabel = ""
    __ylabel = ""
    __fig, __ax = plt.subplots(1, 1)

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

    def save_figure(self, file_dir, filename):
        path = Path(file_dir)
        path.mkdir(parents=True, exist_ok=True)

        plt.savefig(file_dir + '/' + filename)
        plt.close()