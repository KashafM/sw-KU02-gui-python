import math
import os
from pymongo import MongoClient
from datetime import datetime
import gridfs
import pandas as pd

cluster = " "
client = MongoClient(cluster)
db = client.patientDb
patients = db.patients
fs = gridfs.GridFS(db)

def addData(data):
    patient = {"Patient ID": data[0], "Last Name": data[1], "First Name": data[2], "Date": datetime.utcnow(), "Age": data[4], "Gender": data[5], "Weight": data[6], "BMI": data[7], "Classification": data[8], "Features": data[9], "Notes": data[10], "Start Hour": data[11], "End Hour": data[12], "File ID": data[13]}
    patients.insert_one(patient)

def getData():
    results = patients.find({})
    resultList = list(results)
    return resultList

def getDataOne(id):
    results = patients.find({"Patient ID": int(id)})
    resultList = list(results)
    return resultList

def deleteData(id, fileID):
    patients.delete_one({"Patient ID": int(id)})
    fs.delete(fileID)

def updateData(id, data):
    patients.update_one(
        {"Patient ID": int(id)},
        {"$set": {"Last Name": data[1], "First Name": data[2], "Date": datetime.utcnow(), "Age": data[4], "Gender": data[5], "Weight": data[6], "BMI": data[7], "Classification": data[8], "Features": data[9], "Notes": data[10], "Start Hour": data[11], "End Hour": data[12], "File ID": data[13]}})

def getFile(patient, uiWidget):
    fileId = patient[0]['File ID']
    out_data = fs.get(fileId).read()
    with open("patient_data.csv", "w") as my_empty_csv:
        pass
    download_location = "patient_data.csv"
    output = open(download_location, "wb")
    output.write(out_data)
    output.close()
    createDataDict(uiWidget)
    uiWidget.table_widget.updateEogPlot()
    uiWidget.plotGraphs()

def getLatestPatient():
    result = patients.find({}).sort("_id", -1).limit(1)
    patientList = list(result)
    return patientList

def downloadFile(patient, fileLoc):
    file_id = patient[0]["File ID"]
    outputdata = fs.get(file_id).read()

    output = open(fileLoc, "wb")
    output.write(outputdata)
    output.close()

def uploadData(patientData, uiWidget, condition, name):
    file_location = name
    file_data = open(file_location, "rb")
    data = file_data.read()
    fs.put(data, filename=name)
    data = db.fs.files.find_one({'filename': name})
    fileId = data['_id']
    patientData.append(fileId)
    addData(patientData)
    uiWidget.table_widget.table.patientData = getData()
    uiWidget.table_widget.table.setPatientData()

    with condition:
        condition.notify()

def createDataDict(uiWidget):
    df_signals = pd.read_csv("patient_data.csv")
    uiWidget.data_dict["EOG 1"] = list(filter(lambda x: not math.isnan(x), df_signals.loc[:, "EOG 1"].to_numpy().tolist()))
    uiWidget.data_dict["EOG 2"] = list(filter(lambda x: not math.isnan(x), df_signals.loc[:, "EOG 2"].to_numpy().tolist()))

    # imu data
    uiWidget.data_dict["IMU X"] = list(filter(lambda x: not math.isnan(x), df_signals['IMU X'].tolist()))
    uiWidget.data_dict["IMU Y"] = list(filter(lambda x: not math.isnan(x),  df_signals['IMU Y'].tolist()))
    uiWidget.data_dict["IMU Z"] = list(filter(lambda x: not math.isnan(x), df_signals['IMU Z'].tolist()))

    # temp
    uiWidget.data_dict["Temp"] = list(filter(lambda x: not math.isnan(x),  df_signals['Temp'].tolist()))

    os.remove("patient_data.csv")

