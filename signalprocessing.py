from pylab import *
from scipy.signal import filtfilt, cheby2, butter, freqz, medfilt, savgol_filter, detrend
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import scipy
import pyedflib

def get_data_horizontal():
    channel_name = 'Lefteye'
    channel_name_2 = 'RightEye'

    with pyedflib.EdfReader('MCU Data/ucddb007.rec') as edf_file:
        channel_index_left = edf_file.getSignalLabels().index(channel_name)
        channel_data_left = edf_file.readSignal(channel_index_left)

        channel_index_right = edf_file.getSignalLabels().index(channel_name_2)
        channel_data_right = edf_file.readSignal(channel_index_right)

    HEOG = (channel_data_right - channel_data_left)

    return HEOG

def get_data_vertical():
    with open('MCU Data/EOGVerticalSignal_TEST.txt', 'r') as file:
        # Read the contents of the file
        contents = file.read()

        # Split the contents into individual values
        data = contents.split()

        # Convert the values from string to their respective data types
        data = [float(v) for v in data]

        return list(data)

def dataProcessing(data_folder):
    data = pd.read_csv(data_folder)
    eog_vertical = data.loc[:, "EOG_V2"].to_numpy() - data.loc[:, "EOG_V1"].to_numpy()
    eog_horizontal = data.loc[:, "EOG_H1"].to_numpy() - data.loc[:, "EOG_H2"].to_numpy()
    return eog_vertical, eog_horizontal

def segment(signal, segment_length, fs):
    length_segment = segment_length*fs
    num_segments = math.floor((len(signal)/length_segment))
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

def cheby2_bandpass(data, lowcut, highcut, fs, order=4, rs=30):
    nyq = 0.5 * fs
    low = lowcut / nyq
    high = highcut / nyq
    b, a = cheby2(order, rs, [high, low], 'bandpass')
    #freqResponse(a, b, fs)
    filtered_data = filtfilt(b, a, data)

    return filtered_data

def median_filter(data, window_size):
    # Pad the data array to handle edge cases
    pad_width = window_size // 2
    padded_data = np.pad(data, pad_width, mode='edge')

    # Apply the median filter
    filtered_data = medfilt(padded_data, kernel_size=window_size)

    return filtered_data[pad_width:-pad_width]

def polynomialFitting(data, time):
    # Fit a polynomial to the signal using a 15th-degree polynomial

    poly_degree = 15
    signal_detrended = detrend(data, type='linear')
    poly = np.polyfit(time, signal_detrended, poly_degree)
    signal_baseline = np.polyval(poly, time)
    filtered_signal = signal_detrended - signal_baseline
    #filtered_signal = data - np.mean(data)
    return filtered_signal

def derivative_filter(data):
    b_der = np.array([1, -1])
    a_der = np.array([1, -0.997])
    filtered_data = scipy.signal.filtfilt(b_der, a_der, data)
    #filtered_data = savgol_filter(data, window_length=100, polyorder=1, deriv=1)
    return filtered_data

def normalization(data):
    data = np.array(data)
    data = (data - min(data)) / (max(data) - min(data))
    #data = data/max(data)
    return data

def signalProcessingPipeline(data):
    fs_eog = 200
    lowcut = 50
    highcut = 0.01
    time = np.arange(len(data)) / fs_eog
    filteredSignalCheby = cheby2_bandpass(data, lowcut, highcut, fs_eog)
    filteredSignalMedian = median_filter(filteredSignalCheby, 9)
    filteredSignalDeriv = derivative_filter(filteredSignalMedian)
    filteredSignalPoly = polynomialFitting(filteredSignalDeriv, time)
    filteredSignalNorm = normalization(filteredSignalPoly)

    # fig, axs = plt.subplots(5)
    # fig.suptitle('Vertical EOG Signal Processing')
    # axs[0].plot(time, data)
    # axs[0].set_title("Original Signal")
    # axs[0].set_xlabel("Time (sec)")
    # axs[0].set_ylabel("Ampl. (mV)")
    # axs[1].plot(time, filteredSignalCheby)
    # axs[1].set_title("Chebyshev Type II Filtering (Order = 4)")
    # axs[1].set_xlabel("Time (sec)")
    # axs[1].set_ylabel("Ampl. (mV)")
    # axs[2].plot(time, filteredSignalMedian)
    # axs[2].set_title("Median Filtering")
    # axs[2].set_xlabel("Time (sec)")
    # axs[2].set_ylabel("Ampl. (mV)")
    # axs[3].plot(time, filteredSignalPoly)
    # axs[3].set_title("Derivative Filter & Polynomial Fitting")
    # axs[3].set_xlabel("Time (sec)")
    # axs[3].set_ylabel("Ampl. (mV)")
    # axs[4].plot(time, filteredSignalNorm)
    # axs[4].set_title("Normalization")
    # axs[4].set_xlabel("Time (sec)")
    # axs[4].set_ylabel("Ampl. (mV)")
    # fig.subplots_adjust(hspace=1.5, wspace=1.5)
#    plt.savefig("Figures/Signal Processing/SignalProcessingVerticalEOG.png")
#     plt.show()

    return [filteredSignalNorm, filteredSignalPoly]

def plot(signal, signalFiltered, fs, title):
    t = np.arange(0, len(signal)/fs, 1/fs)
    fig, axs = plt.subplots(2)
    fig.suptitle(title)
    axs[0].plot(t, signal, color='C0')
    axs[0].set_xlabel("Time (sec)")
    axs[0].set_ylabel("Amplitude (microvolts)")
    axs[1].plot(t, signalFiltered, color='C0')
    axs[1].set_xlabel("Time (sec)")
    axs[1].set_ylabel("Amplitude (microvolts)")
    plt.savefig('Figures/' + title + ".png")
    plt.show()

def freqResponse(a, b, fs):
    # Plot frequency response
    w, h = freqz(b, a)
    freq = w / (2 * np.pi) * fs
    gain = 20 * np.log10(abs(h))
    phase = np.unwrap(np.angle(h))

    fig, ax = plt.subplots(2, 1, figsize=(8, 6))
    ax[0].plot(freq, gain)
    ax[0].set_ylabel('Gain (dB)')
    ax[0].set_title('Frequency Response')
    ax[0].grid(True)

    ax[1].plot(freq, phase)
    ax[1].set_xlabel('Frequency (Hz)')
    ax[1].set_ylabel('Phase (radians)')
    ax[1].grid(True)

    plt.show()
