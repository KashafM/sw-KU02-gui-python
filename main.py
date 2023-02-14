import logging
import threading
import time
from enum import Enum
from datetime import datetime

from PyQt5.QtWidgets import *
from PyQt5.QtCore import Qt, QRunnable, QThreadPool
from PyQt5 import uic
from PyQt5.QtGui import *
import pyqtgraph as pg
import random
import database as db
import aspose.words as aw
import signalprocessing
import pandas as pd
import numpy as np
import sys
from ble import Ble

# logging
logging.basicConfig(filename='Logs/application.log', format='%(asctime)s %(message)s : %(funcName)s',
                    datefmt='%m/%d/%Y %I:%M:%S %p', encoding='utf-8', level=logging.INFO)

with open("Logs/application.log.", 'r+') as f:
    f.truncate(0)

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
"""

class UI(QMainWindow):
    def __init__(self):
        super(UI, self).__init__()
        # start ble monitoring
        self.ble = Ble()

        self.p = ProcessRunnable(target=self.run)
        self.p.start()
        uic.loadUi("Layouts/Main_Window.ui", self)

        self.connectBtnVal = False

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

        uploadMenu = QAction("&Upload", self)
        uploadMenu.triggered.connect(self.uploadPressed)
        menuBar.addAction(uploadMenu)

        # create connect widget
        self.connectWidget = QWidget()
        self.connectWidget.setWindowTitle("Connection")
        self.connectWidget.setWindowIcon(QIcon("Images\Logo_Symbol.png"))
        lblDevice = QLabel("Device", self)
        self.cmbDeviceAddr = QComboBox(self)
        self.cmbDeviceAddr.setCurrentIndex(-1)
        self.btnConnect = QPushButton('Connect', self)
        self.btnConnect.clicked.connect(self.connectBtnPressed)
        hbox = QHBoxLayout()
        hbox.addWidget(lblDevice)
        hbox.addWidget(self.cmbDeviceAddr)
        hbox.addWidget(self.btnConnect)
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

        self.subtitle_notes = QLabel("Notes", self)
        self.subtitle_notes.setStyleSheet("font-weight: bold")
        self.txtBxNotes = QPlainTextEdit(self)

        self.btnUpload = QPushButton("Upload", self)
        self.btnUpload.disconnect()
        self.btnUpload.clicked.connect(lambda upload: self.uploadBtnPressed("add", id))
        fboxUpWin = QFormLayout()
        fboxUpWin.addRow(self.subtitle_test_specific)
        fboxUpWin.addRow(self.lblStart, self.startTime)
        fboxUpWin.addRow(self.lblEnd, self.endTime)
        fboxUpWin.addRow(self.subtitle_gen_pat_info)
        fboxUpWin.addRow(self.lblLastName, self.txtEdLastName)

        fboxUpWin.addRow(self.lblFirstName, self.txtEdFirstName)
        fboxUpWin.addRow(self.lblAge, self.cmbAge)
        fboxUpWin.addRow(self.lblGender, self.hboxGender)
        fboxUpWin.addRow(self.subtitle_pat_metrics)

        fboxUpWin.addRow(self.lblWeight, self.cmbWeight)
        fboxUpWin.addRow(self.lblBMI, self.cmbBMI)
        fboxUpWin.addRow(self.subtitle_notes, self.txtBxNotes)
        fboxUpWin.addRow(self.btnUpload)

        self.uploadWidget.setLayout(fboxUpWin)
        self.uploadWidget.setWindowFlags(Qt.WindowCloseButtonHint)


        self.table_widget.tab1UI()
        self.table_widget.uiWidget = self
        self.table_widget.tab2UI(self)
        self.table_widget.tab3UI()
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
        self.txtBxNotes.clear()
        self.maleRad.setChecked(False)
        self.femaleRad.setChecked(False)
        self.btnUpload.setText("Upload")
        self.uploadWidget.show()

    def uploadBtnPressed(self, flag, id):
        lastName = self.txtEdLastName.text()
        firstName = self.txtEdFirstName.text()
        date = datetime.utcnow()
        age = self.cmbAge.currentText()
        weight = self.cmbWeight.currentText()
        BMI = self.cmbBMI.currentText()
        notes = self.txtBxNotes.toPlainText()
        if self.maleRad.isChecked():
            gender = "Male"
            self.maleRad.setChecked(False)
        elif self.femaleRad.isChecked():
            gender = "Female"
            self.femaleRad.setChecked(False)
        else:
            gender = "Other"
        if flag == "add":
            patientID = random.randint(1000, 9999)
            data = [patientID, lastName, firstName, date, age, gender, weight, BMI, notes]
            databaseThread = threading.Thread(target=db.uploadData, args=(data, self,))
            databaseThread.start()

        elif flag == "update":
            data = [lastName, firstName, date, age, gender, weight, BMI, notes]
            db.updateData(id, data)
            self.table_widget.table.patientData = db.getData()
            self.table_widget.table.setPatientData()
            self.plotGraphs()
        self.uploadWidget.hide()

    def read_biometrics(self, patient):
        # for prototype purposes ONLY
        self.data_dict = dict.fromkeys(['VEOG', 'HEOG', 'IMU x', 'IMU y', 'IMU z', 'Accel. x', 'Accel. y', 'Accel. z', 'Temp.'])

        # read file from database
        databaseThread = threading.Thread(target=db.getFile, args=(patient, self))
        databaseThread.start()

    def plotGraphs(self):
        print("plotting graphs")

        fs = 50
        t = np.arange(0, len(self.data_dict["VEOG"]) / fs, 1 / fs)

        # clear plots
        self.table_widget.chartEOG.clear()
        self.table_widget.chartAccel.clear()
        self.table_widget.chartTemp.clear()

        self.table_widget.chartEOG.addLegend()
        self.table_widget.chartEOG.plot(t, self.data_dict["VEOG"], name="VEOG", pen=pg.mkPen(color='r'))
        self.table_widget.chartEOG.plot(t, self.data_dict["HEOG"], name="HEOG", pen=pg.mkPen(color='b'))

        self.table_widget.chartAccel.addLegend()

        if(self.table_widget.radAccel.isChecked()):
            self.table_widget.chartAccel.plot(t, self.data_dict["Accel. x"], name="x", pen=pg.mkPen(color='r'))
            self.table_widget.chartAccel.plot(t, self.data_dict["Accel. y"], name="y",  pen=pg.mkPen(color='b'))
            self.table_widget.chartAccel.plot(t, self.data_dict["Accel. z"], name="z",  pen=pg.mkPen(color='y'))
            # self.table_widget.chartAccel.setTitle("Acceleration")
        else:
            #self.table_widget.chartAccel.setTitle("Rotational Motion")
            self.table_widget.chartAccel.plot(t, self.data_dict["IMU x"], name="x", pen=pg.mkPen(color='r'))
            self.table_widget.chartAccel.plot(t, self.data_dict["IMU y"], name="y",  pen=pg.mkPen(color='b'))
            self.table_widget.chartAccel.plot(t, self.data_dict["IMU z"], name="z",  pen=pg.mkPen(color='y'))

        self.table_widget.chartTemp.addLegend()
        self.table_widget.chartTemp.plot(t, self.data_dict["Temp."], name="Temperature", pen=pg.mkPen(color='r'))

    def signalProcessing(self):
        # signal processing
        data_folder = "C:/Users/kasha/Desktop/BME70B - Capstone/Dataset/EOG Raw Data/EOG Raw Data/1/28A183055AE8_20170819025357.csv"
        fs = 50
        segment_length = 30
        eog_vertical, eog_horizontal = signalprocessing.dataProcessing(data_folder)
        segmentedSignalVertical = signalprocessing.segment(eog_vertical, segment_length, fs)
        segmentedSignalHorizontal = signalprocessing.segment(eog_horizontal, segment_length, fs)
        # signalprocessing.plot(segmentedSignalVertical[0], fs, "EOG Vertical Signal Pre-Filtering")

        for i in range(len(segmentedSignalVertical)):
            filteredSignalVertical = signalprocessing.filtering(segmentedSignalVertical[0], fs)
            segmentedSignalVertical[0] = filteredSignalVertical

        filteredSignalHorizontal = signalprocessing.filtering(segmentedSignalHorizontal[0], fs)
        segmentedSignalHorizontal[0] = filteredSignalHorizontal

        # signalprocessing.plot(segmentedSignalVertical[0], fs, "EOG Vertical Signal Filtered")
        # fig, axs = plt.subplots(2)
        # fig.suptitle('Filtered vs. Unfiltered')
        # axs[0].plot(segmentedSignalVertical[0])
        # axs[1].plot(filteredSignalVertical)
        # plt.show()
        # signalprocessing.plot(segmentedSignalHorizontal[0], fs, "EOG Signal Filtered")

        return segmentedSignalVertical, segmentedSignalHorizontal

    def run(self):
        bleProcState = BleProcState.BLE_DISCONNECTED
        peripherals = self.ble.getPeripherals()
        self.cmbDeviceAddr.addItems(peripherals.keys())

        while (True):
            time.sleep(5);
            if (bleProcState == BleProcState.BLE_DISCONNECTED):
                if (self.connectBtnVal):
                    self.btnConnect.setText("Disconnect")
                    self.peripheral = peripherals.get(self.cmbDeviceAddr.currentText())
                    self.ble.connect(self.peripheral)
                    self.connectBtnVal = False
                    self.ble.setServices(self.peripheral)
                    self.service_uuid, self.characteristic_uuid = self.ble.service_characteristic_pair[0]
                    bleProcState = BleProcState.BLE_CONNECTED
                    break;

            elif (bleProcState == BleProcState.BLE_CONNECTED):
                if (UIWindow.connectBtnVal):
                    UIWindow.btnConnect.setText("Connect")
                    self.ble.disconnect(self.peripheral)
                    UIWindow.connectBtnVal = False
                    UIWindow.btnConnect.setEnabled(False)

        if(self.peripheral != None):
            contents = self.peripheral.notify(self.service_uuid, self.characteristic_uuid, lambda data: print(f"Notification: {int(str(data)[-3:-1], 16)}"))

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
        cmbView = QComboBox(self)
        cmbView.setEditable(True);
        line_edit = cmbView.lineEdit()
        line_edit.setAlignment(Qt.AlignCenter)
        radioGrp = QWidget()
        hbox = QHBoxLayout()
        self.radIMU = QRadioButton("Rotational", self)
        self.radIMU.setStyleSheet("color: white")
        self.radAccel = QRadioButton("Acceleration", self)
        self.radAccel.setStyleSheet("color: white")
        self.radAccel.setChecked(True)
        cmbView.addItems(['Week', 'Day 1', 'Day 2', 'Day 3', 'Day 4', 'Day 5', 'Day 6', 'Day 7'])
        self.btnApply = QPushButton("Apply", self)
        self.btnApply.setStyleSheet("background-color : #ffffff;" "border-radius: 6px;" "padding: 3px;")
        self.btnApply.clicked.connect(self.btnApplyPressed)
        hbox.addWidget(self.radIMU)
        hbox.addWidget(self.radAccel)
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
        self.chartAccel.setTitle("Acceleration/Rotational")
        self.chartAccel.setLabel('bottom', 'Time (sec)')
        self.chartAccel.setLabel('left', 'm/s<sup>2')
        self.chartTemp = pg.PlotWidget()
        self.chartTemp.setBackground('black')
        self.chartTemp.setTitle("Ambient Temperature")
        self.chartTemp.setLabel('bottom', 'Time (sec)')
        t = u"\u00b0"
        self.chartTemp.setLabel('left', t+' C')
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
        data = db.getData() #comment out if database not working
        self.table = TableView(data, 20, 5)
        self.table.setPatientData() #comment out if database not working
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
        btnView.clicked.connect(lambda viewBtnPressed: self.table.viewBtnPressed(self.uiWidget))
        vbox.addWidget(btnView)
        layout.addWidget(self.table)
        vbox.addStretch()
        layout.addLayout(vbox)
        self.tab2.setLayout(layout)

    def tab3UI(self):
        self.horizontalGroupBox = QGroupBox("Grid")
        layout_Grid = QGridLayout()
        layout_Grid.setColumnStretch(0, 1)
        layout_Grid.setColumnStretch(1, 2)
        layout_Grid.setRowStretch(0, 2)
        layout_Grid.setRowStretch(1, 1)
        featurePlot = pg.PlotWidget()
        featurePlot.setBackground('black')
        featurePlot.setTitle("Feature Plot")
        featurePlot.setLabel('left', 'Feature 2')
        featurePlot.setLabel('bottom', 'Feature 1')
        eogPlot = pg.PlotWidget()
        eogPlot.setBackground('black')
        eogPlot.setTitle("EOG")
        eogPlot.setLabel('left', 'mV')
        eogPlot.setLabel('bottom', 'Time (sec)')

        tabsClassification = tabsLayout2(self)
        tabsClassification.setStyleSheet("QTabBar::tab:selected{background: #ffffff; color: #000000;} QTabBar::tab {color: #ffffff;} QTabWidget>QWidget>QWidget{background: #ffffff;}")
        layout_Grid.setRowStretch(0, 1)
        layout_Grid.addWidget(featurePlot, 0, 0)
        layout_Grid.addWidget(tabsClassification, 0, 1)
        layout_Grid.addWidget(eogPlot, 1, 0, 1, 2)
        self.horizontalGroupBox.setLayout(layout_Grid)
        self.tab3.setLayout(layout_Grid)

    def btnApplyPressed(self):
        print("Apply button pressed")
        self.uiWidget.plotGraphs()

class tabsLayout2(QTabWidget):
    def __init__(self, parent):
        super(QWidget, self).__init__(parent)
        self.classificationData = {'Overall': ['1','2','5','4', '1', '2', '4', '2'],
        'Morning': ['1','3','2','5', '1', '4', '1', '2'],
        'Afternoon': ['3','2','5','4', '3', '2', '1', '2'],
        'Evening': ['3','2','1','4','5','3', '1', '2']}
        self.performanceData = {'Accuracy': ['1', '2', '5', '4', '1', '2', '4', '2'],
                                   'Recall': ['1', '3', '2', '5', '1', '4', '1', '2'],
                                   'Precision': ['3', '2', '5', '4', '3', '2', '1', '2'],
                                   'AUC': ['3', '2', '1', '4', '5', '3', '1', '2'],
                                   'Epochs': ['3', '2', '1', '4', '5', '3', '1', '2']}

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
        table = TableView(self.classificationData, 8, 4)
        table.setData()
        layout = QVBoxLayout()
        widget = QWidget()
        widget.setLayout(layout)
        layout.addWidget(table)
        self.tab1.setLayout(layout)

    def tabPerformance(self):
        table = TableView(self.performanceData, 4, 5)
        table.setData()
        table.setVerticalHeaderLabels(['Morning', 'Afternoon', 'Evening', 'Overall'])
        layout = QVBoxLayout()
        widget = QWidget()
        widget.setLayout(layout)
        layout.addWidget(table)

        self.tab2.setLayout(layout)

    def tabFeatureAnalysis(self):
        layout_Grid = QGridLayout()
        plotParam = QGroupBox("Plot Parameters")
        layout_Grid.addWidget(plotParam, 0, 0)
        fbox = QFormLayout()
        # layout.setContentsMargins(left, top, right, bottom)
        fbox.setContentsMargins(150, 80, 150, 80)
        plotParam.setLayout(fbox)
        feature1Combo = QComboBox(self)
        labelFeatCombo1 = QLabel("Feature 1")
        feature2Combo = QComboBox(self)
        labelFeatCombo2 = QLabel("Feature 2")
        graphButton = QPushButton('Graph', self)
        fbox.addRow(labelFeatCombo1, feature1Combo)
        fbox.addRow(labelFeatCombo2, feature2Combo)
        featureEpoch = QGroupBox("Features Per Epoch")
        fbox.addRow(graphButton)
        fbox.setAlignment(Qt.AlignVCenter)
        vbox = QVBoxLayout()
        listWidget = QListWidget()
        vbox.addWidget(listWidget)
        featureEpoch.setLayout(vbox)
        layout_Grid.addWidget(featureEpoch, 0, 1)
        layout_Grid.setColumnStretch(0, 1)
        layout_Grid.setColumnStretch(1, 1)
        self.tab3.setLayout(layout_Grid)

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
            self.setItem(row, 4, QTableWidgetItem("n/a"))
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
        for n, key in enumerate(sorted(self.patientData.keys())):
            horHeaders.append(key)
            for m, item in enumerate(self.patientData[key]):
                newitem = QTableWidgetItem(item)
                self.setItem(m, n, newitem)
        self.setHorizontalHeaderLabels(horHeaders)

    def deleteBtnPressed(self):
        currentRow = self.currentRow()

        if self.rowCount() > 0:
            id = self.item(currentRow, 1).text()
            self.removeRow(currentRow)
            db.deleteData(id)

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

        # uiWidget.startTime
        # uiWidget.endTime = QDateTimeEdit(self)
        # patient = {"Patient ID": data[0], "Last Name": data[1], "First Name": data[2], "Date": datetime.utcnow(),
        #            "Age": data[4], "Gender": data[5], "Weight": data[6], "BMI": data[7], "Notes": data[8]}
        uiWidget.txtEdLastName.insert(patient[0]["Last Name"])
        uiWidget.txtEdFirstName.insert(patient[0]["First Name"])
        uiWidget.cmbAge.setCurrentText(patient[0]["Age"])
        uiWidget.btnUpload.setText("Update")
        uiWidget.btnUpload.disconnect()
        uiWidget.btnUpload.clicked.connect(lambda upload: uiWidget.uploadBtnPressed("update", id))
        if patient[0]["Gender"] == "Male":
            uiWidget.maleRad.setChecked(True)
        elif patient[0]["Gender"] == "Female":
            uiWidget.femaleRad.setChecked(True)
        uiWidget.cmbWeight.setCurrentText(patient[0]["Weight"])
        uiWidget.cmbBMI.setCurrentText(patient[0]["BMI"])
        uiWidget.txtBxNotes.setPlainText(patient[0]["Notes"])

    def downEOGBtnPressed(self):
        option = QFileDialog.Options()
        saveLocation = QFileDialog.getSaveFileName(self, "Save File Window Title", "Report.csv", options=option)
        self.saveFile = saveLocation[0]

        currentRow = self.currentRow()
        id = self.item(currentRow, 1).text()
        download_location = self.saveFile
        patient = db.getDataOne(id)
        databaseThread = threading.Thread(target=db.downloadFile, args=(patient, download_location,))
        databaseThread.start()

    def viewBtnPressed(self, uiWidget):
        currentRow = self.currentRow()
        id = self.item(currentRow, 1).text()
        patient = db.getDataOne(id)
        uiWidget.read_biometrics(patient)

class ProcessRunnable(QRunnable):
    def __init__(self, target):
        QRunnable.__init__(self)
        self.t = target
        #self.args = args

    def run(self):
        self.t()

    def start(self):
        QThreadPool.globalInstance().start(self)

if __name__ == '__main__':
    app = QApplication(sys.argv)
    sansFont = QFont("Helvetica", 10)
    app.instance().setFont(sansFont)
    app.setStyleSheet(CSS)
    UIWindow = UI()
    app.exec_()
