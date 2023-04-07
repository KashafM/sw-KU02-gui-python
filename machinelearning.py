import pickle

from sklearn.metrics import roc_auc_score, accuracy_score, precision_score, recall_score
from sklearn.model_selection import train_test_split
from sklearn.discriminant_analysis import LinearDiscriminantAnalysis
import numpy as np
import matplotlib.pyplot as plt

def calculate_performance_metrics(y_true, y_pred):
    #auc = roc_auc_score(y_true, y_pred, multi_class='ovo') #check this parameter more evaluated for features and classifier
    accuracy = accuracy_score(y_true, y_pred)
    precision = precision_score(y_true, y_pred, average='macro')
    recall = recall_score(y_true, y_pred, average='macro')
    return [accuracy, precision, recall]

def save_LDA_model(X, y):
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.3, random_state=42)
    model = LinearDiscriminantAnalysis()
    model.fit(X_train, y_train)
    y_pred = model.predict(X_test)
    filename = 'LDA_model_2.sav'
    pickle.dump(model, open(filename, 'wb'))
    return y_pred, y_test

def get_predicted_value(X_test, model):
    y_pred = model.predict(X_test)
    return y_pred

def get_temp_class(temp_data): # determine the best temperature range according to papers, check if these values are correlated
    avg_temp = np.mean(temp_data)
    if (15.6 <= avg_temp <= 19.4): # search this paper
        temp_class = 1
    else:
        temp_class = 0
    return temp_class

def get_movement_class(imu_x, imu_y, imu_z):
    pass

