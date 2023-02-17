import os

from pymongo import MongoClient
from datetime import datetime
import gridfs
import pandas as pd

cluster = "" ## insert cluster link
# client = motor.motor_asyncio.AsyncIOMotorClient(cluster)
client =  MongoClient(cluster)
db = client.patientDb
patients = db.patients
fs = gridfs.GridFS(db)

def addData(data):
    patient = {"Patient ID": data[0], "Last Name": data[1], "First Name": data[2], "Date": datetime.utcnow(), "Age": data[4], "Gender": data[5], "Weight": data[6], "BMI": data[7], "Notes": data[8], "File ID": data[9]}
    patients.insert_one(patient)

def getData():
    results = patients.find({})
    resultList = list(results)
    return resultList

def getDataOne(id):
    results = patients.find({"Patient ID": int(id)})
    resultList = list(results)
    return resultList

def deleteData(id):
    patients.delete_one({"Patient ID": int(id)})

def updateData(id, data):
    # data = [lastName, firstName, date, age, gender, weight, BMI, notes]
    patients.update_one(
        {"Patient ID": int(id)},
        {"$set": {"Last Name": data[0], "First Name": data[1], "Date": data[2], "Age": data[3], "Gender": data[4], "Weight": data[5], "BMI": data[6], "Notes": data[7]}})

def getFile(patient, uiWidget):
    fileId = patient[0]['File ID']
    out_data = fs.get(fileId).read()
    download_location = "MCU Data/patientData.csv"
    output = open(download_location, "wb")
    output.write(out_data)
    output.close()

    create_data_dict(uiWidget)
    uiWidget.plotGraphs()

def downloadFile(patient, fileLoc):
    file_id = patient[0]["File ID"]
    outputdata = fs.get(file_id).read()

    output = open(fileLoc, "wb")
    output.write(outputdata)
    output.close()

def uploadData(patientData, uiWidget):
    name = "eog.csv"
    file_location = "MCU Data/" + name
    file_data = open(file_location, "rb")
    data = file_data.read()
    fs.put(data, filename=name)
    data = db.fs.files.find_one({'filename': name})
    fileId = data['_id']
    patientData.append(fileId)
    addData(patientData)
    uiWidget.table_widget.table.patientData = getData()
    uiWidget.table_widget.table.setPatientData()
    uiWidget.plotGraphs()

def create_data_dict(uiWidget):
    print("creating data dict")

    df_signals = pd.read_csv('MCU Data/patientData.csv')

    # eog data
    uiWidget.data_dict["VEOG"] = (df_signals.loc[:, "EOG_V2"].to_numpy() - df_signals.loc[:, "EOG_V1"].to_numpy()).tolist()
    uiWidget.data_dict["HEOG"] = (df_signals.loc[:, "EOG_H1"].to_numpy() - df_signals.loc[:, "EOG_H2"].to_numpy()).tolist()

    # acceleration data
    uiWidget.data_dict["IMU x"] = df_signals['ACC_X'].tolist()
    uiWidget.data_dict["IMU y"] = df_signals['ACC_Y'].tolist()
    uiWidget.data_dict["IMU z"] = df_signals['ACC_Z'].tolist()

    # imu data
    uiWidget.data_dict["Accel. x"] = df_signals['ACC_X'].tolist()
    uiWidget.data_dict["Accel. y"] = df_signals['ACC_Y'].tolist()
    uiWidget.data_dict["Accel. z"] = df_signals['ACC_Z'].tolist()

    # temperature
    uiWidget.data_dict["Temp."] = df_signals['Temp'].tolist()

    os.remove('MCU Data/patientData.csv')


