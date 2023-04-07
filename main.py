import math
import os
import threading
from enum import Enum
from datetime import datetime
import time
import joblib
from PyQt5.QtWidgets import *
from PyQt5.QtCore import Qt
from PyQt5 import uic
from PyQt5.QtGui import *
import pyqtgraph as pg
import random
import database as db
import aspose.words as aw
import signalprocessing
import featureextraction
import numpy as np
import sys
from ble import Ble
import bledataextraction
import csv
import pandas as pd
from threading import Thread
import shutil

CSS = """
    QTabBar::tab:selected
    {
        background: #2c3652; color: #ffffff;
    }
    QTabWidget>QWidget>QWidget
    {
        background: #2c3652;
    }

    QTabWidget::pane
    {
        border: 0px;
    }
    QTabBar::tab
    {
        padding: 6px;
        border-top-left-radius: 6px;
        border-top-right-radius: 6px;
    }
    QGroupBox
    {
        border: 2px solid black;
        margin-top: 3ex; /* leave space at the top for the title */
    }
    QGroupBox::title
    {
        subcontrol-origin: margin;
        subcontrol-position: top center; /* position at the top center */
        padding: 0 3px;
    }
    QComboBox::AdjustToContents
"""

class UI(QMainWindow):
    def __init__(self):
        super(UI, self).__init__()
        # start ble monitoring
        self.ble = Ble()

        self.fileSelectMethod = 0

        bleMonitorThread = Thread(target=self.bleMonitorThread)
        bleMonitorThread.start()

        uic.loadUi("Layouts/Main_Window.ui", self)

        self.connectBtnVal = False
        self.dataList = []
        # create a placeholder widget to hold our toolbar and canvas.
        self.setWindowTitle('Nodsense')
        self.setWindowIcon(QIcon("Images\Logo_Symbol.png"))
        self.resize(1200, 750)
        self.label = QLabel(self)
        self.pixmap = QPixmap('Images/Logo.png')
        self.label.setPixmap(self.pixmap)
        self.label.resize(self.pixmap.width(),
                          self.pixmap.height())
        self.table_widget = tabsLayout(self)

        mainWidget = QWidget(self)
        self.setCentralWidget(mainWidget)
        self.destination_path = ""
        layout = QVBoxLayout()
        layout.addWidget(self.label, alignment=Qt.AlignHCenter)
        self.label.setContentsMargins(0, 10, 10, 0)
        layout.addWidget(self.table_widget)
        layout.setContentsMargins(0, 0, 0, 0)
        self.table_widget.setContentsMargins(0, 0, 0, 0)

        menuBar = QMenuBar(self)
        self.setMenuBar(menuBar)
        connectMenu = QAction("&Connect", self)
        connectMenu.triggered.connect(self.connectPressed)
        menuBar.addAction(connectMenu)

        self.uploadMenu = QAction("&Upload", self)
        self.uploadMenu.triggered.connect(self.uploadPressed)
        menuBar.addAction(self.uploadMenu)

        # create connect widget
        self.connectWidget = QWidget()
        self.connectWidget.setWindowTitle("Connection")
        self.connectWidget.setWindowIcon(QIcon("Images\Logo_Symbol.png"))
        lblDevice = QLabel("Device", self)
        self.cmbDeviceAddr = QComboBox(self)
        self.cmbDeviceAddr.setCurrentIndex(-1)
        self.cmbDeviceAddr.setFixedWidth(400)
        self.btnConnect = QPushButton('Connect', self)
        self.btnConnect.clicked.connect(self.connectBtnPressed)
        self.iconRefresh = QIcon('Images/refresh_icon.png')
        self.btnRefresh = QPushButton(self.iconRefresh, "", self)
        self.btnRefresh.clicked.connect(self.refreshBleDevices)
        hbox = QHBoxLayout()
        hbox.addWidget(lblDevice)
        hbox.addWidget(self.cmbDeviceAddr)
        hbox.addWidget(self.btnConnect)
        hbox.addWidget(self.btnRefresh)
        hbox.setContentsMargins(50, 25, 50, 25)
        self.connectWidget.setLayout(hbox)
        self.connectWidget.setWindowFlags(Qt.WindowCloseButtonHint)

        # create upload panel
        self.id = 0

        self.uploadWidget = QWidget()
        self.uploadWidget.setWindowTitle("Upload")
        self.uploadWidget.setWindowIcon(QIcon("Images\Logo_Symbol.png"))
        self.subtitle_test_specific = QLabel("Test Specific Information", self)
        self.subtitle_test_specific.setStyleSheet("font-weight: bold")
        self.lblStart = QLabel("Start")
        self.startTime = QDateTimeEdit(self)
        self.lblEnd = QLabel("End")
        self.endTime = QDateTimeEdit(self)

        self.subtitle_gen_pat_info = QLabel("General Patient Information", self)
        self.subtitle_gen_pat_info.setStyleSheet("font-weight: bold")
        self.lblLastName = QLabel("Last Name", self)
        self.txtEdLastName = QLineEdit()
        self.lblFirstName = QLabel("First Name", self)
        self.txtEdFirstName = QLineEdit()
        ageList = map(str, [*range(0, 120, 1)])
        self.lblAge = QLabel("Age", self)
        self.cmbAge = QComboBox()
        self.cmbAge.addItems(ageList)
        self.hboxGender = QHBoxLayout()
        self.maleRad = QRadioButton("Male", self)
        self.femaleRad = QRadioButton("Female", self)
        self.hboxGender.addWidget(self.maleRad)
        self.hboxGender.addWidget(self.femaleRad)
        self.lblGender = QLabel("Gender", self)

        self.subtitle_pat_metrics = QLabel("Patient Metrics", self)
        self.subtitle_pat_metrics.setStyleSheet("font-weight: bold")
        self.lblWeight = QLabel("Weight (kg)", self)
        weightList = map(str, [*range(0, 442, 1)])
        self.cmbWeight = QComboBox()
        self.cmbWeight.addItems(weightList)
        BMIList = map(str, [*range(0, 60, 1)])
        self.lblBMI = QLabel("BMI", self)
        self.cmbBMI = QComboBox()
        self.cmbBMI.addItems(BMIList)

        self.lblDatafile = QLabel("Data File", self)
        self.cmbDatafile = QComboBox()
        self.subtitle_notes = QLabel("Notes", self)
        self.subtitle_notes.setStyleSheet("font-weight: bold")
        self.txtBxNotes = QPlainTextEdit(self)

        self.btnUploadFile = QPushButton("Choose", self)
        self.btnUploadFile.clicked.connect(self.uploadFileDialog)
        self.lblUpload = QLabel('No file selected', self)

        self.btnUpload = QPushButton("Upload", self)
        self.btnUpload.disconnect()
        self.btnUpload.clicked.connect(lambda upload: self.uploadBtnPressed("add", id))

        self.lblProgress = QLabel("Progress")
        self.lblProgress.hide()

        self.fboxUpWin = QFormLayout()
        self.fboxUpWin.addRow(self.subtitle_test_specific)
        self.fboxUpWin.addRow(self.lblStart, self.startTime)
        self.fboxUpWin.addRow(self.lblEnd, self.endTime)
        self.fboxUpWin.addRow(self.subtitle_gen_pat_info)
        self.fboxUpWin.addRow(self.lblLastName, self.txtEdLastName)

        self.fboxUpWin.addRow(self.lblFirstName, self.txtEdFirstName)
        self.fboxUpWin.addRow(self.lblAge, self.cmbAge)
        self.fboxUpWin.addRow(self.lblGender, self.hboxGender)
        self.fboxUpWin.addRow(self.subtitle_pat_metrics)

        self.fboxUpWin.addRow(self.lblWeight, self.cmbWeight)
        self.fboxUpWin.addRow(self.lblBMI, self.cmbBMI)
        self.fboxUpWin.addRow(self.subtitle_notes, self.txtBxNotes)
        self.fboxUpWin.addRow(self.lblDatafile, self.cmbDatafile)
        self.fboxUpWin.addRow(self.btnUploadFile, self.lblUpload)
        self.fboxUpWin.addRow(self.btnUpload)
        self.uploadWidget.setLayout(self.fboxUpWin)
        self.uploadWidget.setWindowFlags(Qt.WindowCloseButtonHint)
        self.fboxUpWin.addRow(self.lblProgress)

        self.table_widget.tab1UI()
        self.table_widget.uiWidget = self
        self.table_widget.tab2UI(self)
        self.table_widget.tab3UI(self)
        mainWidget.setLayout(layout)

        self.show()

    def connectBtnPressed(self):
        self.connectBtnVal = True

    def connectPressed(self):
        self.connectWidget.show()

    def uploadPressed(self):
        self.btnUpload.disconnect()
        self.btnUpload.clicked.connect(lambda upload: self.uploadBtnPressed("add", self.id))
        self.txtEdLastName.clear()
        self.txtEdFirstName.clear()
        self.cmbAge.setCurrentIndex(-1)
        self.cmbWeight.setCurrentIndex(-1)
        self.cmbBMI.setCurrentIndex(-1)
        self.cmbDatafile.setCurrentIndex(-1)
        self.txtBxNotes.clear()
        self.maleRad.setChecked(False)
        self.femaleRad.setChecked(False)
        self.btnUpload.setText("Upload")
        self.uploadWidget.show()

    def uploadBtnPressed(self, flag, patient):
        self.patientID = 0
        self.lblUpload.setText("No file selected")
        self.lastName = self.txtEdLastName.text()
        self.firstName = self.txtEdFirstName.text()
        self.date = datetime.utcnow()
        self.age = self.cmbAge.currentText()
        self.weight = self.cmbWeight.currentText()
        self.BMI = self.cmbBMI.currentText()
        self.notes = self.txtBxNotes.toPlainText()
        dataFileSelected = self.cmbDatafile.currentText()
        self.startTimeVal = self.startTime.time()
        self.startHour = (int)(self.startTimeVal.toString("HH"))
        self.endTimeVal = self.endTime.time()
        self.endHour = (int)(self.endTimeVal.toString("HH"))

        if self.maleRad.isChecked():
            self.gender = "Male"
            self.maleRad.setChecked(False)
        elif self.femaleRad.isChecked():
            self.gender = "Female"
            self.femaleRad.setChecked(False)
        else:
            self.gender = "Other"

        self.classification = "n/a"

        if flag == "add":
            if self.fileSelectMethod == 0:  # ble mode
                self.peripheral.write_request(self.service_uuidWrite, self.characteristic_uuidWrite,
                                              str.encode(dataFileSelected))

            self.patientID = random.randint(1000, 9999)
            self.fileUpload = "patient_" + str(self.patientID) + "_data.csv"

            with open(self.fileUpload, 'w', newline='') as file:
                csv.writer(file).writerow([])

            if(self.fileSelectMethod == 0):
                eogAcqThread = Thread(target=self.eogAcquisitionThread, args=())
                eogAcqThread.start()
            else:
                self.processDataFile()

        elif flag == "update":
            data = [patient[0]['Patient ID'], self.lastName, self.firstName, self.date, self.age, self.gender, self.weight, self.BMI, patient[0]['Classification'], patient[0]['Features'], self.notes, self.startHour, self.endHour, patient[0]['File ID']]
            id = patient[0]['Patient ID']
            db.updateData(id, data)
            self.table_widget.table.patientData = db.getData()
            self.table_widget.table.setPatientData()

    def read_biometrics(self, patient):
        self.data_dict = dict.fromkeys(['EOG 1', 'EOG 2', 'Temp', 'IMU X', 'IMU Y', 'IMU Z'])
        self.downloadedFeatures = patient[0]['Features']
        databaseThread = threading.Thread(target=db.getFile, args=(patient, self))
        databaseThread.start()

    def plotGraphs(self):
        fs_eog = 200
        fs_temp = 1
        fs_imu = 100
        time_eog1 = np.arange(0, len(self.data_dict["EOG 1"]) / fs_eog, 1 / fs_eog)
        time_eog2 = np.arange(0, len(self.data_dict["EOG 2"]) / fs_eog, 1 / fs_eog)
        time_imux = np.arange(0, len(self.data_dict["IMU X"]) / fs_imu, 1 / fs_imu)
        time_imuy = np.arange(0, len(self.data_dict["IMU Y"]) / fs_imu, 1 / fs_imu)
        time_imuz = np.arange(0, len(self.data_dict["IMU Z"]) / fs_imu, 1 / fs_imu)
        time_temp = np.arange(0, len(self.data_dict["Temp"]) / fs_temp, 1 / fs_temp)

        self.table_widget.chartEOG.clear()
        self.table_widget.chartAccel.clear()
        self.table_widget.chartTemp.clear()

        self.table_widget.chartEOG.addLegend()
        self.table_widget.chartEOG.plot(time_eog1, self.data_dict["EOG 1"], name="EOG 1", pen=pg.mkPen(color='r'))
        self.table_widget.chartEOG.plot(time_eog2, self.data_dict["EOG 2"], name="EOG 2", pen=pg.mkPen(color='b'))

        self.table_widget.chartAccel.addLegend()

        self.table_widget.chartAccel.plot(time_imux, self.data_dict["IMU X"], name="x", pen=pg.mkPen(color='r'))
        self.table_widget.chartAccel.plot(time_imuy, self.data_dict["IMU Y"], name="y",  pen=pg.mkPen(color='b'))
        self.table_widget.chartAccel.plot(time_imuz, self.data_dict["IMU Z"], name="z",  pen=pg.mkPen(color='y'))

        self.table_widget.chartTemp.addLegend()
        self.table_widget.chartTemp.plot(time_temp, self.data_dict["Temp"], name="Temperature", pen=pg.mkPen(color='r'))

    def signalProcessing(self, eog_1, eog_2):
        # eog_1 = horizontal, eog_2 = vertical
        [filteredEog1Norm, filteredEog1] = signalprocessing.signalProcessingPipeline(eog_1)
        [filteredEog2Norm, filteredEog2] = signalprocessing.signalProcessingPipeline(eog_2)

        filteredSignalHorizontal = filteredEog1.tolist()
        filteredSignalVertical = filteredEog2.tolist()

        filteredSignalHorizontalNorm = filteredEog1Norm.tolist()
        filteredSignalVerticalNorm = filteredEog2Norm.tolist()

        return [filteredSignalHorizontal, filteredSignalVertical, filteredSignalHorizontalNorm, filteredSignalVerticalNorm]

    def featureExtraction(self, eog_1, eog_2, eog_1_norm, eog_2_norm, temp, imu_x, imu_y, imu_z):
        saccade_features = featureextraction.extract_saccade_features(eog_1)
        blink_features = featureextraction.extract_blink_features(eog_2)
        temp_features = featureextraction.extract_temp_features(temp)
        imu_features = featureextraction.extract_imu_features(imu_x, imu_y, imu_z)
        self.features = (saccade_features + blink_features + [temp_features] + [imu_features])
    def machineLearning(self):
        model = joblib.load('model.sav')
        featuresList = ["Min Saccade", "Mean Saccade Rate", "Power Saccade Amp", "Power Blink Amplitude",
                 "Mean Blink Duration", "Max Blink Duration", "Average Temp", "Magnitude Angular Vel"]
        X_test = pd.DataFrame(np.array(self.features).reshape(1, -1), columns=featuresList)
        self.classification = str(model.predict(X_test)[0])

    def bleMonitorThread(self):
        bleProcState = BleProcState.BLE_DISCONNECTED
        self.peripherals = self.ble.getPeripherals()
        self.cmbDeviceAddr.addItems(self.peripherals.keys())

        while True:
            if (bleProcState == BleProcState.BLE_DISCONNECTED):
                if (self.connectBtnVal and self.cmbDeviceAddr.currentText() != ""):
                    self.btnConnect.setText("Disconnect")
                    self.peripheral = self.peripherals.get(self.cmbDeviceAddr.currentText())
                    self.ble.connect(self.peripheral)
                    self.connectBtnVal = False
                    self.ble.setServices(self.peripheral)
                    self.service_uuidNotify, self.characteristic_uuidNotify = self.ble.service_characteristic_pair[0]
                    self.service_uuidRead, self.characteristic_uuidRead = self.ble.service_characteristic_pair[1]
                    self.service_uuidWrite, self.characteristic_uuidWrite = self.ble.service_characteristic_pair[2]
                    self.readDataFiles()
                    bleProcState = BleProcState.BLE_CONNECTED

            if (bleProcState == BleProcState.BLE_CONNECTED):
                if(self.peripheral.is_connected() == 0 or self.connectBtnVal == 1):
                    self.btnConnect.setText("Connect")
                    self.ble.disconnect(self.peripheral)
                    self.connectBtnVal = False
                    self.refreshBleDevices()
                    bleProcState = BleProcState.BLE_DISCONNECTED
            time.sleep(0.2)

    def eogAcquisitionThread(self):
        self.lblProgress.show()
        self.lblProgress.setText("Transmitting...")
        self.progCounter = 0

        if(self.peripheral.is_connected()):
            self.peripheral.notify(self.service_uuidNotify, self.characteristic_uuidNotify, lambda data: self.bleData(data, ))

    def bleData(self, data):
        self.dataList.append(data[1:])
        binary_string = bledataextraction.convert_to_binary(data)
        self.progCounter = self.progCounter + 1

        if((int)(binary_string[0])):
            self.end_time = time.time()
            self.lblProgress.setText("Processing...")
            self.processDataFile()

    def processDataFile(self):
        self.lblProgress.show()
        self.lblProgress.setText("Processing...")

        try:
            eog_1, eog_2, temp, imu_x, imu_y, imu_z = bledataextraction.get_data(self.dataList, self.fileSelectMethod, self.destination_path)
            [eog_1, eog_2, eog_1_norm, eog_2_norm] = self.signalProcessing(eog_1, eog_2)
            self.createDataCsv(eog_1, eog_2, temp, imu_x, imu_y, imu_z)
            self.featureExtraction(eog_1, eog_2, eog_1_norm, eog_2_norm, temp, imu_x, imu_y, imu_z)
            self.machineLearning()

            condition = threading.Condition()
            with condition:
                self.lblProgress.setText("Uploading...")
                data = [self.patientID, self.lastName, self.firstName, self.date, self.age, self.gender, self.weight,
                        self.BMI, self.classification, self.features, self.notes, self.startHour, self.endHour]
                databaseThread = threading.Thread(target=db.uploadData, args=(data, self, condition, self.fileUpload))
                databaseThread.start()
                condition.wait()
            os.remove(self.fileUpload)

            self.lblProgress.hide()
            self.uploadWidget.hide()

        except BaseException as ex:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = exc_tb.tb_frame.f_code.co_filename
            line_no = exc_tb.tb_lineno
            print(f"An error occurred in {fname} at line {line_no}: {ex}")

    def readDataFiles(self):
        contents = self.peripheral.read(self.service_uuidRead, self.characteristic_uuidRead)
        binary_string = bledataextraction.convert_to_binary(contents)
        fileList = []
        for i, bit in enumerate(binary_string):
            if bit == '1':
                fileList.append(str(i))
        self.cmbDatafile.addItems(fileList)

    def refreshBleDevices(self):
        self.peripherals = {}
        self.peripherals = self.ble.getPeripherals()
        self.cmbDeviceAddr.clear()
        self.cmbDeviceAddr.addItems(self.peripherals.keys())

    def createDataCsv(self, eog_1, eog_2, temp, imu_x, imu_y, imu_z):
        max_len = max(len(eog_1), len(eog_2), len(temp), len(imu_x), len(imu_y), len(imu_z))
        eog_1 += [None] * (max_len - len(eog_1))
        eog_2 += [None] * (max_len - len(eog_2))
        temp += [None] * (max_len - len(temp))
        imu_x += [None] * (max_len - len(imu_x))
        imu_y += [None] * (max_len - len(imu_y))
        imu_z += [None] * (max_len - len(imu_z))

        df = pd.DataFrame({'EOG 1': eog_1, 'EOG 2': eog_2, 'Temp': temp, 'IMU X': imu_x, 'IMU Y': imu_y, 'IMU Z': imu_z})
        df.to_csv(self.fileUpload, index=False)

    def uploadFileDialog(self):
        self.fileSelectMethod = 1
        filename, _ = QFileDialog.getOpenFileName(self, 'Select file', os.getenv('HOME'))
        if filename:
            self.fileSelectMethod = 1
            basename = os.path.basename(filename)
            self.lblUpload.setText(basename)
            self.uploadFile(filename)

    def uploadFile(self, file_path):
        filename = os.path.basename(file_path)
        self.destination_path = os.path.join("", filename)
        shutil.copy2(file_path, self.destination_path)

class BleProcState(Enum):
        BLE_CONNECTED = 1
        BLE_DISCONNECTED = 2

class tabsLayout(QTabWidget):
    def __init__(self, parent):
        super(QWidget, self).__init__(parent)
        self.layout = QVBoxLayout(self)
        self.fileSave = " "
        self.uiWidget = None
        # Initialize tab screen
        self.tab1 = QWidget()
        self.tab2 = QWidget()
        self.tab3 = QWidget()
        self.resize(300, 200)

        # Add tabs
        self.addTab(self.tab1, "Graphs")
        self.addTab(self.tab2, "Database")
        self.addTab(self.tab3, "Classification")

    def tab1UI(self):
        t = u"\u00b0"
        cmbView = QComboBox(self)
        cmbView.setEditable(True);
        line_edit = cmbView.lineEdit()
        line_edit.setAlignment(Qt.AlignCenter)
        radioGrp = QWidget()
        hbox = QHBoxLayout()
        cmbView.addItems(['Week', 'Day 1', 'Day 2', 'Day 3', 'Day 4', 'Day 5', 'Day 6', 'Day 7'])
        self.btnApply = QPushButton("Apply", self)
        self.btnApply.setStyleSheet("background-color : #ffffff;" "border-radius: 6px;" "padding: 3px;")
        self.btnApply.clicked.connect(self.btnApplyPressed)
        hbox.addWidget(cmbView)
        hbox.addWidget(self.btnApply)
        pg.setConfigOption('foreground', 'w')
        self.chartEOG = pg.PlotWidget()
        self.chartEOG.setBackground('black')
        self.chartEOG.setTitle("EOG")
        self.chartEOG.setLabel('bottom', 'Time (sec)')
        self.chartEOG.setLabel('left', 'Amplitude (mV)')
        self.chartAccel = pg.PlotWidget()
        self.chartAccel.setBackground('black')
        self.chartAccel.setTitle("Rotational Motion")
        self.chartAccel.setLabel('bottom', 'Time (sec)')
        self.chartAccel.setLabel('left', t+'/s')
        self.chartTemp = pg.PlotWidget()
        self.chartTemp.setBackground('black')
        self.chartTemp.setTitle("Ambient Temperature")
        self.chartTemp.setLabel('bottom', 'Time (sec)')
        self.chartTemp.setLabel('left', t+'C')
        layout = QVBoxLayout()
        widget = QWidget()
        widget.setLayout(layout)
        layout.addWidget(radioGrp, alignment=Qt.AlignRight)
        layout.addWidget(self.chartEOG)
        radioGrp.setLayout(hbox)
        layout.addWidget(self.chartAccel)
        layout.addWidget(self.chartTemp)
        self.tab1.setLayout(layout)

    def tab2UI(self, uiWidget):
        self.uiWidget = uiWidget
        data = db.getData()
        self.table = TableView(data, 20, 5)
        self.table.setPatientData()
        layout = QHBoxLayout()
        widget = QWidget()
        widget.setLayout(layout)
        vbox = QVBoxLayout()
        btnEdit = QPushButton("Edit", self)
        btnEdit.clicked.connect(lambda check: self.table.editBtnPressed(self.uiWidget))
        btnEdit.setStyleSheet("background-color : #ffffff;" "border-radius: 6px;" "padding: 3px;")
        btnDelete = QPushButton("Delete", self)
        btnDelete.setStyleSheet("background-color : #ffffff;" "border-radius: 6px;" "padding: 3px;")
        btnDelete.clicked.connect(self.table.deleteBtnPressed)
        vbox.addWidget(btnEdit)
        vbox.addWidget(btnDelete)
        btnReport = QPushButton("Save Report", self)
        btnReport.setStyleSheet("background-color : #ffffff;" "border-radius: 6px;" "padding: 3px;")
        btnReport.clicked.connect(self.table.reportBtnPressed)
        vbox.addWidget(btnReport)
        btnDownloadSig = QPushButton("Download EOG", self)
        btnDownloadSig.setStyleSheet("background-color : #ffffff;" "border-radius: 6px;" "padding: 3px;")
        btnDownloadSig.clicked.connect(self.table.downEOGBtnPressed)
        vbox.addWidget(btnDownloadSig)
        btnView = QPushButton("View Patient", self)
        btnView.setStyleSheet("background-color : #ffffff;" "border-radius: 6px;" "padding: 3px;")
        btnView.clicked.connect(lambda viewBtnPressed: self.table.viewBtnPressed(self.uiWidget, self))
        vbox.addWidget(btnView)
        layout.addWidget(self.table)
        vbox.addStretch()
        layout.addLayout(vbox)
        self.tab2.setLayout(layout)

    def tab3UI(self, uiWidget):
        self.horizontalGroupBox = QGroupBox("Grid")
        layout_Grid = QGridLayout()
        layout_Grid.setColumnStretch(0, 1)
        layout_Grid.setColumnStretch(1, 2)
        layout_Grid.setRowStretch(0, 2)
        layout_Grid.setRowStretch(1, 1)
        self.featurePlot = pg.PlotWidget()
        self.featurePlot.setBackground('black')
        self.featurePlot.setTitle("Feature Plot")
        self.featurePlot.setLabel('left', 'Feature 2')
        self.featurePlot.setLabel('bottom', 'Feature 1')
        self.eogPlot = pg.PlotWidget()
        self.eogPlot.setBackground('black')
        self.eogPlot.setTitle("EOG")
        self.eogPlot.setLabel('left', 'mV')
        self.eogPlot.setLabel('bottom', 'Time (sec)')

        self.tabsClassification = tabsLayout2(self, uiWidget)
        self.tabsClassification.uiWidget = uiWidget
        self.tabsClassification.setStyleSheet("QTabBar::tab:selected{background: #ffffff; color: #000000;} QTabBar::tab {color: #ffffff;} QTabWidget>QWidget>QWidget{background: #ffffff;}")
        layout_Grid.setRowStretch(0, 1)
        layout_Grid.addWidget(self.featurePlot, 0, 0)
        layout_Grid.addWidget(self.tabsClassification, 0, 1)
        layout_Grid.addWidget(self.eogPlot, 1, 0, 1, 2)
        self.horizontalGroupBox.setLayout(layout_Grid)
        self.tab3.setLayout(layout_Grid)
        self.tabsClassification.graphButton.clicked.connect(self.updateFeaturePlot)
        self.scatter1 = pg.ScatterPlotItem()
        self.scatter2 = pg.ScatterPlotItem()
        self.featurePlot.addItem(self.scatter1)
        self.featurePlot.addItem(self.scatter2)

    def updateFeaturePlot(self):
        self.featurePlot.removeItem(self.scatter1)
        self.featurePlot.removeItem(self.scatter2)
        feature1Label, feature2Label, feature1List, feature2List, patient_feat1, patient_feat2 = self.tabsClassification.getFeatureList()
        self.scatter1 = pg.ScatterPlotItem(x=feature1List, y=feature2List, size=10, pen=pg.mkPen(None), brush=pg.mkBrush(255, 0, 0, 120))
        self.scatter2 = pg.ScatterPlotItem(x=[patient_feat1, math.nan], y=[patient_feat2, math.nan], size=10, pen=pg.mkPen(None),brush=pg.mkBrush(0, 255, 0, 120))
        #line = pg.PlotItem(x=lda_bounds, y=, pen=pg.mkPen(color='r', width=2))

        self.featurePlot.addItem(self.scatter1)
        self.featurePlot.addItem(self.scatter2)
        self.featurePlot.setLabel('left', feature1Label)
        self.featurePlot.setLabel('bottom', feature2Label)

    def updateEogPlot(self):
        fs = 200
        time_eog1 = np.arange(0, len(self.uiWidget.data_dict["EOG 1"])/fs, 1/fs)
        time_eog2 = np.arange(0, len(self.uiWidget.data_dict["EOG 2"]) / fs, 1 / fs)
        self.eogPlot.clear()
        self.eogPlot.addLegend()
        self.eogPlot.plot(time_eog1, self.uiWidget.data_dict["EOG 1"], name="EOG 1", pen=pg.mkPen(color='r'))
        self.eogPlot.plot(time_eog2, self.uiWidget.data_dict["EOG 2"], name="EOG 2", pen=pg.mkPen(color='b'))

    def updateEpochValue(self):
        numEpochs, numFeatures = self.tabsClassification.getFeatureValues()
        self.tabsClassification.listWidgetEpoch.clear()
        for i in range(1, numEpochs + 1):
            item = QListWidgetItem("Epoch: " + str(i) + ": Number of Features: " + str(numFeatures))
            self.tabsClassification.listWidgetEpoch.addItem(item)

    def btnApplyPressed(self):
        self.uiWidget.plotGraphs()

class tabsLayout2(QTabWidget):
    def __init__(self, uiWidget, parent):
        super(QWidget, self).__init__(parent)
        self.uiWidget = uiWidget
        self.classificationData = {'Overall': ['-','-','-','-', '-', '-', '-'],
        'Morning': ['-','-','-','-', '-', '-', '-'],
        'Afternoon': ['-','-','-','-', '-', '-', '-'],
        'Evening': ['-','-','-','-','-','-', '-']}

        self.performanceData = {'Accuracy': ['90.91%'],
                                   'Recall': ['91.67%'],
                                   'Precision': ['91.67%']}

        self.layout = QVBoxLayout(self)

        # Initialize tab screen
        self.tab1 = QWidget()
        self.tab2 = QWidget()
        self.tab3 = QWidget()
        self.resize(300, 200)

        # Add tabs
        self.addTab(self.tab1, "Classification")
        self.addTab(self.tab2, "Performance")
        self.addTab(self.tab3, "Feature Analysis")

        self.tabClassification()
        self.tabPerformance()
        self.tabFeatureAnalysis()

    def tabClassification(self):
        self.table = TableView(self.classificationData, 7, 4)
        self.table.setData()
        self.layoutClass = QVBoxLayout()
        widget = QWidget()
        widget.setLayout(self.layoutClass)
        self.layoutClass.addWidget(self.table)
        self.tab1.setLayout(self.layoutClass)

    def updateTabClassification(self, overall, morning, afternoon, evening):
        classificationData = {'Overall': [str(overall),'-','-','-', '-', '-', '-'],
        'Morning': [str(morning),'-','-','-', '-', '-', '-'],
        'Afternoon': [str(afternoon),'-','-','-', '-', '-', '-'],
        'Evening': [str(evening),'-','-','-','-','-', '-']}

        self.layoutClass.removeWidget(self.table)
        self.table = TableView(classificationData, 7, 4)
        self.table.setData()
        self.layoutClass.addWidget(self.table)
        self.tab1.setLayout(self.layoutClass)

    def tabPerformance(self):
        table = TableView(self.performanceData, 1, 3)
        table.setData()
        layout = QVBoxLayout()
        widget = QWidget()
        widget.setLayout(layout)
        layout.addWidget(table)

        self.tab2.setLayout(layout)

    def tabFeatureAnalysis(self):
        featuresList = ["Min Saccade", "Mean Saccade Rate", "Power Saccade Amp", "Power Blink Amplitude",
                 "Mean Blink Duration", "Max Blink Duration", "Average Temp", "Magnitude Angular Vel"]

        layout_Grid = QGridLayout()
        plotParam = QGroupBox("Plot Parameters")
        layout_Grid.addWidget(plotParam, 0, 0)
        fbox = QFormLayout()
        fbox.setContentsMargins(150, 80, 150, 80)
        plotParam.setLayout(fbox)
        self.feature1Combo = QComboBox(self)
        self.feature1Combo.addItems(featuresList)
        labelFeatCombo1 = QLabel("Feature 1")
        self.feature2Combo = QComboBox(self)
        self.feature2Combo.addItems(featuresList)
        labelFeatCombo2 = QLabel("Feature 2")
        self.graphButton = QPushButton('Graph', self)
        fbox.addRow(labelFeatCombo1, self.feature1Combo)
        fbox.addRow(labelFeatCombo2, self.feature2Combo)
        featureEpoch = QGroupBox("Features Per Epoch")
        fbox.addRow(self.graphButton)
        fbox.setAlignment(Qt.AlignVCenter)
        vbox = QVBoxLayout()
        self.listWidgetEpoch = QListWidget()
        vbox.addWidget(self.listWidgetEpoch)
        featureEpoch.setLayout(vbox)
        layout_Grid.addWidget(featureEpoch, 0, 1)
        layout_Grid.setColumnStretch(0, 1)
        layout_Grid.setColumnStretch(1, 1)
        self.tab3.setLayout(layout_Grid)

    def getFeatureList(self):
        featuresListDf = pd.read_csv('features.csv')
        feature1 = self.feature1Combo.currentText()
        feature2 = self.feature2Combo.currentText()
        patient_feat1 = self.uiWidget.downloadedFeatures[self.feature1Combo.currentIndex()]
        patient_feat2 = self.uiWidget.downloadedFeatures[self.feature2Combo.currentIndex()]
        feature1List = featuresListDf[feature1].to_numpy().tolist()
        feature2List = featuresListDf[feature2].to_numpy().tolist()
        return feature1, feature2, feature1List, feature2List, patient_feat1, patient_feat2

    def getFeatureValues(self):
        featuresListDf = pd.read_csv('features.csv')
        numEpochs = featuresListDf.shape[0]
        numFeatures = len(featuresListDf.columns) - 1
        return numEpochs, numFeatures

class TableView(QTableWidget):
    def __init__(self, data, *args):
        QTableWidget.__init__(self, *args)
        self.patientData = data
        self.resizeColumnsToContents()
        self.resizeRowsToContents()
        self.horizontalHeader().setStretchLastSection(True)
        self.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.saveFile = " "
        self.uploadWidget = 0

    def setPatientData(self):
        horHeaders = ["Date", "Patient ID", "Last Name", "First Name", "Overall Classification"]
        row = 0
        for patient in self.patientData:
            self.setItem(row, 0, QTableWidgetItem(patient["Date"].strftime("%m/%d/%Y")))
            self.setItem(row, 1, QTableWidgetItem(str(patient["Patient ID"])))
            self.setItem(row, 2, QTableWidgetItem(patient["Last Name"]))
            self.setItem(row, 3, QTableWidgetItem(patient["First Name"]))
            self.setItem(row, 4, QTableWidgetItem(patient["Classification"]))
            row = row + 1
        self.setHorizontalHeaderLabels(horHeaders)

    def createReport(self):
        currentRow = self.currentRow()
        id = self.item(currentRow, 1).text()
        patient = db.getDataOne(id)
        # create document object
        doc = aw.Document()
        # create a document builder object
        builder = aw.DocumentBuilder(doc)
        # add text to the document
        builder.write("PATIENT REPORT \n")
        #builder.write("Date: " + patient["Date"])
        builder.write("Patient ID: " + str(patient[0]["Patient ID"]) + "\n")
        builder.write("Last Name: " + patient[0]["Last Name"] + "\n")
        builder.write("First Name: " + patient[0]["First Name"] + "\n")
        builder.write("Age: " + str(patient[0]["Age"]) + "\n")
        builder.write("Weight: " + str((patient[0]["Weight"])) + "\n")
        builder.write("BMI: " + str(patient[0]["BMI"]) + "\n")
        builder.write("Overall Classification: " + str(patient[0]["Classification"]) + "\n")
        builder.write("Notes: " + patient[0]["Notes"] + "\n")

        # save document
        doc.save(self.saveFile)

    def reportBtnPressed(self):
        option = QFileDialog.Options()
        saveLocation = QFileDialog.getSaveFileName(self, "Save File Window Title", "Report.docx", options=option)
        self.saveFile = saveLocation[0]
        self.createReport()

    def setData(self):
        horHeaders = []
        for n, key in enumerate(self.patientData.keys()):
            horHeaders.append(key)
            for m, item in enumerate(self.patientData[key]):
                newitem = QTableWidgetItem(item)
                self.setItem(m, n, newitem)
        self.setHorizontalHeaderLabels(horHeaders)

    def deleteBtnPressed(self):
        currentRow = self.currentRow()
        if self.rowCount() > 0:
            id = self.item(currentRow, 1).text()
            fileID = db.getDataOne(id)[0]["File ID"]
            self.removeRow(currentRow)
            db.deleteData(id, fileID)

    def editBtnPressed(self, uiWidget):
        uiWidget.txtEdLastName.clear()
        uiWidget.txtEdFirstName.clear()
        uiWidget.cmbAge.setCurrentIndex(-1)
        uiWidget.cmbWeight.setCurrentIndex(-1)
        uiWidget.cmbBMI.setCurrentIndex(-1)
        uiWidget.txtBxNotes.clear()

        uiWidget.uploadWidget.show()
        currentRow = self.currentRow()
        id = self.item(currentRow, 1).text()
        patient = db.getDataOne(id)

        uiWidget.txtEdLastName.insert(patient[0]["Last Name"])
        uiWidget.txtEdFirstName.insert(patient[0]["First Name"])
        uiWidget.cmbAge.setCurrentText(patient[0]["Age"])
        uiWidget.btnUpload.setText("Update")
        uiWidget.btnUpload.disconnect()
        uiWidget.btnUpload.clicked.connect(lambda upload: uiWidget.uploadBtnPressed("update", patient))
        if patient[0]["Gender"] == "Male":
            uiWidget.maleRad.setChecked(True)
        elif patient[0]["Gender"] == "Female":
            uiWidget.femaleRad.setChecked(True)
        uiWidget.cmbWeight.setCurrentText(patient[0]["Weight"])
        uiWidget.cmbBMI.setCurrentText(patient[0]["BMI"])
        uiWidget.txtBxNotes.setPlainText(patient[0]["Notes"])

    def downEOGBtnPressed(self):
        option = QFileDialog.Options()
        saveLocation = QFileDialog.getSaveFileName(self, "Save File Window Title", "EOG.csv", options=option)
        self.saveFile = saveLocation[0]

        currentRow = self.currentRow()
        id = self.item(currentRow, 1).text()
        download_location = self.saveFile
        patient = db.getDataOne(id)
        databaseThread = threading.Thread(target=db.downloadFile, args=(patient, download_location,))
        databaseThread.start()

    def viewBtnPressed(self, uiWidget, tabs):
        currentRow = self.currentRow()
        id = self.item(currentRow, 1).text()
        patient = db.getDataOne(id)
        uiWidget.read_biometrics(patient)
        uiWidget.table_widget.updateEpochValue()

        startHour = patient[0]['Start Hour']
        overall = patient[0]['Classification']
        self.morning = '-'
        self.afternoon = '-'
        self.evening = '-'
        self.getTimeDay(startHour, overall)
        tabs.tabsClassification.updateTabClassification(overall, self.morning, self.afternoon, self.evening)

    def getTimeDay(self, hour, classification):
        if hour >= 1 and hour < 12:
            self.morning = classification
        elif hour >= 12 and hour < 18:
            self.afternoon = classification
        else:
            self.evening = classification

if __name__ == '__main__':
    app = QApplication(sys.argv)
    sansFont = QFont("Helvetica", 10)
    app.instance().setFont(sansFont)
    app.setStyleSheet(CSS)
    UIWindow = UI()
    app.exec_()