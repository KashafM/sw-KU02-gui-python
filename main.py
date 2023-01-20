import signalprocessing

if __name__ == '__main__':

    # signal processing
    data_folder = "C:/Users/kasha/Desktop/BME70B - Capstone/Dataset/EOG Raw Data/EOG Raw Data/1/28A183055AE8_20170819025357.csv"
    fs = 50
    segment_length = 30
    eog_vertical, eog_horizontal = signalprocessing.dataProcessing(data_folder)
    segmentedSignalVertical = signalprocessing.segment(eog_vertical, segment_length, fs)
    segmentedSignalHorizontal = signalprocessing.segment(eog_horizontal, segment_length, fs)
    signalprocessing.plot(segmentedSignalVertical[0], fs, "EOG Signal Pre-Filtering")
    for i in range(len(segmentedSignalVertical)):
        filteredSignalVertical = signalprocessing.bandPassFilter(segmentedSignalVertical[0], fs)
        segmentedSignalVertical[i] = filteredSignalVertical

        filteredSignalHorizontal = signalprocessing.bandPassFilter(segmentedSignalHorizontal[0], fs)
        segmentedSignalHorizontal[i] = filteredSignalHorizontal

    signalprocessing.plot(segmentedSignalVertical[0], fs, "EOG Signal Filtered")

