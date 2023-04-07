from scipy.signal import find_peaks
import numpy as np
import math
import signalprocessing
import matplotlib.pyplot as plt

def removeNaN(data):
    return list(filter(lambda x: x is not None, data))

def extract_saccade_features(data):
    global minimum_saccade_amp, mean_saccade_rate, power_saccade_amp
    try:
        data = removeNaN(data)
        data_norm = signalprocessing.normalization(data)
        time = np.arange(len(data))/200
        saccade_amp = []
        peaks_amp, properties = find_peaks(data_norm, height=0.4, width=(55, 150))
        peaks, properties = find_peaks(1-data_norm, height=0.2, width=(55, 150))

        peaks_all = sorted(peaks_amp.tolist() + peaks.tolist())
        if(len(peaks_all) % 2 != 0):
            peaks_all.append(peaks_all[-1])

        fg, ax = plt.subplots()
        ax.plot(time, data_norm)
        ax.plot(time[peaks_amp], (data_norm)[peaks_amp], "o")
        ax.plot(time[peaks], (data_norm)[peaks], "x")
        plt.title("Saccade Detection")
        plt.ylabel("Amplitude (mV)")
        plt.xlabel("Time (sec)")
        plt.show()

        saccade_durations = properties['widths']/200

        for index in peaks:
            saccade_amp.append(data[index])

        if (len(saccade_amp) > 0):
            maximum_saccade_amp = max(saccade_amp)
            minimum_saccade_amp = min(saccade_amp)
            mean_saccade_amp = np.mean(saccade_amp)
            power_saccade_amp = np.sum((saccade_amp - mean_saccade_amp)**2) / len(saccade_amp)
        else:
            maximum_saccade_amp = 0
            minimum_saccade_amp = 0
            mean_saccade_amp = 0
            power_saccade_amp = 0

        mean_saccade_rate = np.mean(len(peaks) / time[-1])

        if(len(saccade_durations) > 0):
            mean_saccade_duration = np.mean(saccade_durations)
            min_saccade_duration = min(saccade_durations)
            max_saccade_duration = max(saccade_durations)
        else:
            mean_saccade_duration = 0
            min_saccade_duration = 0
            max_saccade_duration = 0

        saccade_number = len(peaks)

    except BaseException as ex:
        print(ex)
        print("In saccade features function")
        print("Error occurred on line:", ex.__traceback__.tb_lineno)

    return [minimum_saccade_amp, mean_saccade_rate, power_saccade_amp]

def extract_blink_features(data):
    global min_blink_amp, max_blink_amp, mean_blink_amp, power_blink_amp, blink_number, mean_blink_duration, min_blink_duration, max_blink_duration
    data = removeNaN(data)
    try:
        time = np.arange(len(data)) / 200
        data_norm = signalprocessing.normalization(data)
        blink_amp = []
        flipped = 1 - data_norm
        peaks_amp, properties = find_peaks(flipped, height=0.6, distance=150, width=1)
        blink_durations = properties['widths']

        for index in peaks_amp:
            blink_amp.append(data[index])

        # indicesRemove = []
        # for i in range(0, len(blink_durations)):
        #     if blink_durations[i] < 120:
        #         indicesRemove.append(i)

        fg, ax = plt.subplots()
        ax.plot(time, flipped)
        ax.plot(time[peaks_amp], (flipped)[peaks_amp], "o")
        plt.title("Blink Detection")
        plt.ylabel("Amplitude (mV)")
        plt.xlabel("Time (sec)")
        plt.show()

        if (len(blink_amp) > 0):
            max_blink_amp = max(blink_amp)
            min_blink_amp = min(blink_amp)
            mean_blink_amp = np.mean(blink_amp)
            power_blink_amp = np.sum((blink_amp - mean_blink_amp)**2) / len(blink_amp)
        else:
            max_blink_amp = 0
            min_blink_amp = 0
            mean_blink_amp = 0
            power_blink_amp = 0

        blink_number = len(peaks_amp)

        blink_durations = blink_durations/200
        if (len(blink_durations) > 0):
            mean_blink_duration = np.mean(blink_durations)
            max_blink_duration = max(blink_durations)
            min_blink_duration = min(blink_durations)
        else:
            mean_blink_duration = 0
            max_blink_duration = 0
            min_blink_duration = 0

    except BaseException as ex:
        print(ex)

    return [power_blink_amp, mean_blink_duration, max_blink_duration]

def extract_temp_features(data):
    data = removeNaN(data)
    return np.mean(data)

def extract_imu_features(imu_x, imu_y, imu_z):
    imu_x = removeNaN(imu_x)
    imu_y = removeNaN(imu_y)
    imu_z = removeNaN(imu_z)
    magnitude = math.sqrt(np.mean(imu_x)**2 + np.mean(imu_y)**2 + np.mean(imu_z)**2)
    return magnitude