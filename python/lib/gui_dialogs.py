
import os
import sys
import PyQt5
import PyQt5.QtWidgets
import PyQt5.QtCore



qtApp = PyQt5.QtWidgets.QApplication(sys.argv)


#
#   Dialog box to select experiment from drop-down list
#
class expt_select_gui(PyQt5.QtWidgets.QDialog):
    def __init__(self, explist, parent=None):
        super(expt_select_gui, self).__init__(parent)

        layout = PyQt5.QtWidgets.QVBoxLayout(self)
        self.setWindowTitle("Cheetah GUI experiment selector")

        # Add a label
        self.label1 = PyQt5.QtWidgets.QLabel()
        self.label1.setText("Previous experiments")
        layout.addWidget(self.label1)

        # Combo box for experiment list
        self.cb = PyQt5.QtWidgets.QComboBox()
        #self.cb.addItem("C")
        #self.cb.addItems(["Java", "C#", "Python"])
        self.cb.addItems(explist)
        layout.addWidget(self.cb)

        # Add some user defined buttons
        layout2 = PyQt5.QtWidgets.QHBoxLayout()
        self.button1 = PyQt5.QtWidgets.QPushButton("Go to selected experiment",self)
        self.button1.clicked.connect(self.button1_function)
        self.button2 = PyQt5.QtWidgets.QPushButton("Set up new experiment",self)
        self.button2.clicked.connect(self.button2_function)
        self.button3 = PyQt5.QtWidgets.QPushButton("Find a different experiment",self)
        self.button3.clicked.connect(self.button3_function)
        self.button4 = PyQt5.QtWidgets.QPushButton("Cancel",self)
        self.button4.clicked.connect(self.button4_function)
        layout2.addWidget(self.button1)
        layout2.addWidget(self.button3)
        layout2.addWidget(self.button2)
        layout2.addWidget(self.button4)
        layout.addLayout(layout2)

        # Default OK and Cancel buttons
        #self.buttonBox = PyQt5.QtWidgets.QDialogButtonBox(self)
        #self.buttonBox.setOrientation(PyQt5.QtCore.Qt.Horizontal)
        #self.buttonBox.setStandardButtons(PyQt5.QtWidgets.QDialogButtonBox.Ok | PyQt5.QtWidgets.QDialogButtonBox.Cancel)
        #layout.addWidget(self.buttonBox)
        #self.buttonBox.accepted.connect(self.accept)
        #self.buttonBox.rejected.connect(self.reject)


    # Can't use predefined buttons, so need to figure out and return selected action for ourselves
    def button1_function(self):
        self.action = 'goto'
        self.close()

    def button2_function(self):
        self.action = 'setup_new'
        self.close()

    def button3_function(self):
        self.action = 'find'
        self.close()

    def button4_function(self):
        self.action = 'cancel'
        self.close()


    # get selected text from drop down menu
    def getExpt(self):
        action = self.action
        selected_expt = self.cb.currentText()
        result = {
            'action' : action,
            'selected_expt' : selected_expt
        }
        return result

    # static method to create the dialog and return
    @staticmethod
    def get_expt(explist, parent=None):
        dialog = expt_select_gui(explist, parent=parent)
        result = dialog.exec_()
        result = dialog.getExpt()
        return result



#
#   Dialog box for launching Cheetah options
#
class run_cheetah_gui(PyQt5.QtWidgets.QDialog):
    def __init__(self, dialog_info, parent=None):
        super(run_cheetah_gui, self).__init__(parent)

        inifile_list = dialog_info['inifile_list']
        inifile_list = sorted(inifile_list)
        lastini = dialog_info['lastini']
        lasttag = dialog_info['lasttag']


        layout = PyQt5.QtWidgets.QVBoxLayout(self)
        self.setWindowTitle("Run Cheetah")

        # Add some useful labels
        #self.label1a = PyQt5.QtWidgets.QLabel()
        #self.label1a.setText("Label")
        #self.label1b = PyQt5.QtWidgets.QLabel()
        #self.label1b.setText("Command: ../process/process")
        #layout.addWidget(self.label1a)
        #layout.addWidget(self.label1b)


        # Text box for dataset entry
        layout2 = PyQt5.QtWidgets.QHBoxLayout()
        self.label2 = PyQt5.QtWidgets.QLabel()
        self.label2.setText("Dataset tag: ")
        self.le = PyQt5.QtWidgets.QLineEdit(self)
        self.le.setText(lasttag)
        layout2.addWidget(self.label2)
        layout2.addWidget(self.le)
        layout.addLayout(layout2)


        # Combo box for list of .ini files
        layout3 = PyQt5.QtWidgets.QHBoxLayout()
        self.label3 = PyQt5.QtWidgets.QLabel()
        self.label3.setText("cheetah.ini file: ")
        self.cb = PyQt5.QtWidgets.QComboBox()
        self.cb.addItem(lastini)
        self.cb.addItems(inifile_list)
        layout3.addWidget(self.label3)
        layout3.addWidget(self.cb)
        layout.addLayout(layout3)


        # Default OK and Cancel buttons
        self.buttonBox = PyQt5.QtWidgets.QDialogButtonBox(self)
        self.buttonBox.setOrientation(PyQt5.QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(PyQt5.QtWidgets.QDialogButtonBox.Ok | PyQt5.QtWidgets.QDialogButtonBox.Cancel)
        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        layout.addWidget(self.buttonBox)


    # get selected text
    def getCheetahIni(self):
        selection = {
            'dataset' : self.le.text(),
            'inifile': self.cb.currentText()
        }
        return selection

    # static method to create the dialog and return
    @staticmethod
    def cheetah_dialog(dialog_info, parent=None):
        dialog = run_cheetah_gui(dialog_info, parent=parent)
        result = dialog.exec_()
        selection = dialog.getCheetahIni()
        return (selection, result == PyQt5.QtWidgets.QDialog.Accepted)



#
#   Dialog box for launching CrystFEL
#
class run_crystfel_dialog(PyQt5.QtWidgets.QDialog):
    def __init__(self, dialog_in, parent=None):
        super(run_crystfel_dialog, self).__init__(parent)

        # Get input values
        pdb_files = dialog_in['pdb_files']
        geom_files = dialog_in['geom_files']
        recipe_files = dialog_in['recipe_files']
        default_geom = dialog_in['default_geom']
        default_recipe = dialog_in['default_recipe']
        default_cell = dialog_in['default_cell']

        # Empty dialog box
        layout = PyQt5.QtWidgets.QVBoxLayout(self)
        self.setWindowTitle("Run CrystFEL indexing")


        # List of CrystFEL recipes
        layout1 = PyQt5.QtWidgets.QHBoxLayout()
        #layout1.setAlignment(PyQt5.QtCore.Qt.AlignLeft)
        self.label1 = PyQt5.QtWidgets.QLabel()
        self.label1.setText("Indexing recipe: ")
        self.cb1 = PyQt5.QtWidgets.QComboBox()
        self.cb1.addItem(os.path.relpath(default_recipe))
        self.cb1.addItems(recipe_files)
        layout1.addWidget(self.label1)
        layout1.addWidget(self.cb1)
        layout.addLayout(layout1)

        # Geometry files
        layout3 = PyQt5.QtWidgets.QHBoxLayout()
        # layout3.setAlignment(PyQt5.QtCore.Qt.AlignLeft)
        self.label3 = PyQt5.QtWidgets.QLabel()
        self.label3.setText("Geometry file: ")
        self.cb3 = PyQt5.QtWidgets.QComboBox()
        self.cb3.addItem(os.path.relpath(default_geom))
        self.cb3.addItems(geom_files)
        layout3.addWidget(self.label3)
        layout3.addWidget(self.cb3)
        layout.addLayout(layout3)

        # List of cell files
        layout2 = PyQt5.QtWidgets.QHBoxLayout()
        #layout2.setAlignment(PyQt5.QtCore.Qt.AlignLeft)
        self.label2 = PyQt5.QtWidgets.QLabel()
        self.cb2 = PyQt5.QtWidgets.QComboBox()
        self.label2.setText("Unit cell file (*.cell,*.pdb): ")
        self.cb2.addItem(default_cell)
        self.cb2.addItems(pdb_files)
        layout2.addWidget(self.label2)
        layout2.addWidget(self.cb2)
        layout.addLayout(layout2)

        # Default OK and Cancel buttons
        self.buttonBox = PyQt5.QtWidgets.QDialogButtonBox(self)
        self.buttonBox.setOrientation(PyQt5.QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(PyQt5.QtWidgets.QDialogButtonBox.Ok | PyQt5.QtWidgets.QDialogButtonBox.Cancel)
        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        layout.addWidget(self.buttonBox)


    # get selected text
    def getCrystfelParams(self):
        dialog_out = {
            'recipefile' : self.cb1.currentText(),
            'pdbfile' : self.cb2.currentText(),
            'geomfile': self.cb3.currentText()
        }
        return dialog_out

    # static method to create the dialog and return
    @staticmethod
    def dialog_box(dialog_info, parent=None):
        dialog = run_crystfel_dialog(dialog_info, parent=parent)
        result = dialog.exec_()
        dialog_out = dialog.getCrystfelParams()
        return (dialog_out , result == PyQt5.QtWidgets.QDialog.Accepted)


