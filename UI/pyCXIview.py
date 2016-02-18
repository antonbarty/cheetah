# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'pyCXIview.ui'
#
# Created by: PyQt4 UI code generator 4.11.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_MainWindow(object):
    def setupUi(self, MainWindow):
        MainWindow.setObjectName(_fromUtf8("MainWindow"))
        MainWindow.resize(800, 600)
        self.centralwidget = QtGui.QWidget(MainWindow)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.gridLayout = QtGui.QGridLayout(self.centralwidget)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.verticalLayout = QtGui.QVBoxLayout()
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.horizontalLayout4 = QtGui.QHBoxLayout()
        self.horizontalLayout4.setObjectName(_fromUtf8("horizontalLayout4"))
        self.timeStampLabel = QtGui.QLabel(self.centralwidget)
        self.timeStampLabel.setObjectName(_fromUtf8("timeStampLabel"))
        self.horizontalLayout4.addWidget(self.timeStampLabel)
        self.verticalLayout.addLayout(self.horizontalLayout4)
        self.imageView = ImageView(self.centralwidget)
        self.imageView.setObjectName(_fromUtf8("imageView"))
        self.verticalLayout.addWidget(self.imageView)
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        self.previousPushButton = QtGui.QPushButton(self.centralwidget)
        self.previousPushButton.setObjectName(_fromUtf8("previousPushButton"))
        self.horizontalLayout.addWidget(self.previousPushButton)
        self.nextPushButton = QtGui.QPushButton(self.centralwidget)
        self.nextPushButton.setObjectName(_fromUtf8("nextPushButton"))
        self.horizontalLayout.addWidget(self.nextPushButton)
        self.peaksCheckBox = QtGui.QCheckBox(self.centralwidget)
        self.peaksCheckBox.setObjectName(_fromUtf8("peaksCheckBox"))
        self.horizontalLayout.addWidget(self.peaksCheckBox)
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.horizontalLayout2 = QtGui.QHBoxLayout()
        self.horizontalLayout2.setObjectName(_fromUtf8("horizontalLayout2"))
        self.pixelLabel = QtGui.QLabel(self.centralwidget)
        self.pixelLabel.setObjectName(_fromUtf8("pixelLabel"))
        self.horizontalLayout2.addWidget(self.pixelLabel)
        self.horizontalLayout3 = QtGui.QHBoxLayout()
        self.horizontalLayout3.setObjectName(_fromUtf8("horizontalLayout3"))
        self.jumpToLabel = QtGui.QLabel(self.centralwidget)
        self.jumpToLabel.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.jumpToLabel.setObjectName(_fromUtf8("jumpToLabel"))
        self.horizontalLayout3.addWidget(self.jumpToLabel)
        self.jumpToLineEdit = QtGui.QLineEdit(self.centralwidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.jumpToLineEdit.sizePolicy().hasHeightForWidth())
        self.jumpToLineEdit.setSizePolicy(sizePolicy)
        self.jumpToLineEdit.setObjectName(_fromUtf8("jumpToLineEdit"))
        self.horizontalLayout3.addWidget(self.jumpToLineEdit)
        self.horizontalLayout2.addLayout(self.horizontalLayout3)
        self.verticalLayout.addLayout(self.horizontalLayout2)
        self.gridLayout.addLayout(self.verticalLayout, 1, 0, 1, 1)
        MainWindow.setCentralWidget(self.centralwidget)

        self.retranslateUi(MainWindow)
        QtCore.QMetaObject.connectSlotsByName(MainWindow)

    def retranslateUi(self, MainWindow):
        MainWindow.setWindowTitle(_translate("MainWindow", "MainWindow", None))
        self.timeStampLabel.setText(_translate("MainWindow", "Time Stamp: -", None))
        self.previousPushButton.setText(_translate("MainWindow", "Previous Pattern", None))
        self.nextPushButton.setText(_translate("MainWindow", "Next Pattern", None))
        self.peaksCheckBox.setText(_translate("MainWindow", "Show/Hide Peaks", None))
        self.pixelLabel.setText(_translate("MainWindow", "Last clicked pixel:     x: -     y: -     value: -     resolution: -", None))
        self.jumpToLabel.setText(_translate("MainWindow", "Jump To Frame:", None))

from pyqtgraph import ImageView
