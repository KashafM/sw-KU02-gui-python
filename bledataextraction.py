import numpy as np
import os
import matplotlib.pyplot as plt

def read_list(dataList):
    hex_string = ""
    for data in dataList:
        hex_string = hex_string + data[2:-1] + "\\"

def read_bin_file(file):
    with open(file, 'rb') as binary_file:
        hex_string = binary_file.read()
    return hex_string

def convert_to_binary(hex_string):
    bytes_object = bytes(hex_string)
    binary_strings = [bin(byte)[2:].zfill(8) for byte in bytes_object]
    binary_string = "".join(binary_strings)
    return binary_string

def splice_binary(binary_string, splice): #11256, 11400
    binary_values = [binary_string[i:i + splice] for i in range(0, len(binary_string), splice)]
    return binary_values

def extract_bits(binary_data, start_index, end_index):
    sub_bit_string = binary_data[start_index: end_index]
    return sub_bit_string

def convert_to_decimal(binary_string):
    decimal_values = []
    binary_values = [binary_string[i:i + 16] for i in range(0, len(binary_string), 16)]
    for binary_value in binary_values:
        decimal_value = int(binary_value, 2)
        decimal_values.append(decimal_value)
    return decimal_values

def temp_conversion(binary_string):
    temp_values = []
    binary_value = binary_string[1:12]

    if binary_value[0] == '1':
        flipped_bits = ''.join(['1' if bit == '0' else '0' for bit in binary_string[1:]])
        decimal_value = (int(flipped_bits, 2) + 1)
        temp_values.append(-decimal_value)
    else:
        temp_values.append(int(binary_string, 2))

    temp_values_adjusted = [x / 256 for x in temp_values]
    return temp_values_adjusted

def imu_conversion(binary_string):
    imu_values = []
    binary_values = [binary_string[i:i + 16] for i in range(0, len(binary_string), 16)]
    for binary in binary_values:
        if binary[0] == '1':
            flipped_bits = ''.join(['1' if bit == '0' else '0' for bit in binary[1:]])
            decimal_value = (int(flipped_bits, 2) + 1)
            imu_values.append(-decimal_value)
        else:
            imu_values.append(int(binary, 2))

    imu_values_adjusted = [x / 32.8 for x in imu_values]
    if(len(imu_values_adjusted) != 0):
        imu_values_adjusted = imu_values_adjusted - np.mean(imu_values_adjusted)
    return imu_values_adjusted

def get_data(dataList, fileSelectMethod, filename):
    eog_1 = []
    eog_2 = []
    temp = []
    imu_x = []
    imu_y = []
    imu_z = []

    binary_string = ""
    if fileSelectMethod == 0:
        for hex_string in dataList:
            binary_string = binary_string + convert_to_binary(hex_string)
        splice = 11400
    else:
        hex_string = read_bin_file(filename)
        binary_string = convert_to_binary(hex_string)
        os.remove(filename)
        splice = 11256

    chunks = splice_binary(binary_string, splice)

    for chunk in chunks:
        eog_1_string = extract_bits(chunk, 56, 3256)
        eog_1.extend(convert_to_decimal(eog_1_string))
        eog_2_string = extract_bits(chunk, 3256, 6456)
        eog_2.extend(convert_to_decimal(eog_2_string))
        temp_string = extract_bits(chunk, 40, 56)
        temp.extend(temp_conversion(temp_string))
        imu_x_string = extract_bits(chunk, 6456, 8056)
        imu_x.extend(imu_conversion(imu_x_string))

        imu_y_string = extract_bits(chunk, 8056, 9656)
        imu_y.extend(imu_conversion(imu_y_string))

        imu_z_string = extract_bits(chunk, 9656, 11256)
        imu_z.extend(imu_conversion(imu_z_string))

    return eog_1, eog_2, temp, imu_x, imu_y, imu_z
