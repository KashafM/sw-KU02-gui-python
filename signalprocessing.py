import math
from inspect import unwrap

from pylab import *
from scipy.signal import filtfilt
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import scipy

def dataProcessing(data_folder):
    data = pd.read_csv(data_folder)
    eog_vertical = data.loc[:, "EOG_V2"].to_numpy() - data.loc[:, "EOG_V1"].to_numpy()
    eog_horizontal = data.loc[:, "EOG_H1"].to_numpy() - data.loc[:, "EOG_H2"].to_numpy()
    return eog_vertical, eog_horizontal

def segment(signal, segment_length, fs):
    length_segment = segment_length*fs  # 1500 samples
    num_segments = math.floor((signal.size/length_segment))
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

def filtering(signal, fs):
    try:
        # chebyshev bandpass filter
        # filteredSignal = [0]

        fc = 15  # Cutoff frequency
        fstop = 16  # stop-band at 1000
        ripple = 1  # we'll allow 3 dB ripple in the passband
        attenuation = 100  # we'll require 60 dB attenuation in the stop band

        # Get the order and discard the second output (natural frequency)
        order, _ = scipy.signal.cheb1ord(fc, fstop, ripple, attenuation, fs=fs)

        # Build the filter
        b, a = scipy.signal.cheby2(order, ripple, fc, fs=fs)

        w, h = scipy.signal.freqz(b, a)
        # plt.semilogx(w / np.pi, 20 * np.log10(abs(h)))
        # plt.title('Chebyshev II Bandpass Filter Fit to Constraints')
        # plt.xlabel('Normalized Frequency')
        # plt.ylabel('Amplitude [dB]')
        # plt.grid(which='both', axis='both')
        # plt.fill([.01, .1, .1, .01], [-3, -3, -99, -99], '0.9', lw=0)  # stop
        # plt.fill([.2, .2, .5, .5], [9, -60, -60, 9], '0.9', lw=0)  # pass
        # plt.fill([.6, .6, 2, 2], [-99, -3, -3, -99], '0.9', lw=0)  # stop
        # plt.axis([0.06, 1, -80, 3])
        # plt.show()

        filteredSignal = scipy.signal.filtfilt(b, a, signal)

        # # hanning lowpass filter
        # b_lp = np.array([1, -1])
        # a_lp = np.array([1])
        # filteredSignal = scipy.signal.filtfilt(b_lp, a_lp, signal)

        # derivative filter
        b_der = np.array([1, -1])
        a_der = np.array([1, -0.995])

        filteredSignal = scipy.signal.filtfilt(b_der, a_der, filteredSignal)

    except BaseException as ex:
        print("Error occurred while filtering signals. " + str(ex))

    return filteredSignal