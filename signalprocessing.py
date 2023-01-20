import math
from scipy.signal import filtfilt
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import scipy

def dataProcessing(data_folder):
    data = pd.read_csv(data_folder)
    eog_vertical = data.loc[:, "EOG_V2"].to_numpy() - data.loc[:, "EOG_V1"].to_numpy()
    eog_horizontal = data.loc[:, "EOG_H2"].to_numpy() - data.loc[:, "EOG_H1"].to_numpy()
    return eog_vertical, eog_horizontal

def segment(signal, segment_length, fs):
    length_segment = segment_length*fs  # 1500 samples
    num_segments = math.floor((signal.size/1500))
    start = 0
    end = length_segment
    rows, cols = (num_segments, length_segment)
    segmentedSignal = [[0] * cols] * rows

    for i in range(num_segments):
        segment = signal[start:end]
        start = start + length_segment
        end = end + length_segment
        segmentedSignal[i] = segment
    return segmentedSignal

def plot(signal, fs, title):
    t = np.arange(0, len(signal)/fs, 1/fs)
    fig, axs = plt.subplots()
    axs.set_title(title)
    axs.plot(t, signal, color='C0')
    axs.set_xlabel("Time (sec)")
    axs.set_ylabel("Amplitude (microvolts)")
    plt.show()

def bandPassFilter(signal, fs):
    try:
        lowcut = 0.5
        highcut = 50
        nyq = 0.5*fs
        low = lowcut / nyq
        high = highcut / nyq
        order = 2
        b, a = scipy.signal.butter(order, [low, high], btype='band', fs=fs)
        filteredSignal = scipy.signal.filtfilt(b, a, signal, axis=0)

    except BaseException as ex:
        print("Error occurred while filtering signals. " + str(ex))

    return filteredSignal