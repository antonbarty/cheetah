#!/usr/bin/env python3
#
#   cheetah-gui
#   A python/Qt replacement for the IDL cheetah GUI
#

import os
import sys
import glob
import argparse
import subprocess
import PyQt4.QtCore
import PyQt4.QtGui

import UI.cheetahgui_ui
import lib.cfel_filetools as cfel_file
import lib.gui_dialogs as gui_dialogs


# TODO: Dialog for selecting experiment
# TODO: Dialog for starting Cheetah processing

#
#	Cheetah GUI code
#
class cheetah_gui(PyQt4.QtGui.QMainWindow):

    #
    # Launch a subprocess (eg: viewer or analysis script) without blocking the GUI
    # Separate routine makes it easy change this globally if needed
    #
    def spawn_subprocess(self, cmdarr):
        command = str.join(' ', cmdarr)
        print(command)
        #subprocess.Popen(cmdarr)


    #
    # Quick wrapper for viewing images (this code got repeated over-and-over)
    #
    def show_selected_images(self, filepat, field='data/data'):
        runs = self.selected_runs()
        if len(runs['run']) == 0:
            return
        file = runs['path'][0] + filepat
        cmdarr = ['cxiview.py', '-g', self.config['geometry'], '-e', field, '-i', file]
        self.spawn_subprocess(cmdarr)
    #end show_selected_powder()



    #
    #   Actions to be done each refresh cycle
    #
    def refresh_table(self):

        # Button is busy
        self.ui.button_refresh.setEnabled(False)

        # Load the table data
        status = cfel_file.csv_to_dict('crawler.txt')
        ncols = len(list(status.keys()))-1

        # Length of first list is number of rows - except when it is the fieldnames list
        #nrows = len(status[list(status.keys())[0]])
        nrows = len(status['Run'])
        self.table.setRowCount(nrows)
        self.table.setColumnCount(ncols)
        self.table.updateGeometry()

        # Populate the table
        numbercols = [0]
        for col, key in enumerate(status['fieldnames']):
            for row, item in enumerate(status[key]):

                if col in numbercols:
                    newitem = PyQt4.QtGui.QTableWidgetItem()
                    newitem.setData(PyQt4.QtCore.Qt.DisplayRole, float(item))
                else:
                    newitem = PyQt4.QtGui.QTableWidgetItem(item)

                self.table.setItem(row,col,newitem)

        # Table fiddling
        #TODO: Make columns resizeable
        self.table.setWordWrap(False)
        self.table.setHorizontalHeaderLabels(status['fieldnames'])
        self.table.verticalHeader().setVisible(False)
        #self.table.resizeColumnsToContents()
        self.table.resizeRowsToContents()
        self.table.show()

        # Button is no longer busy; set timer for next refresh
        self.ui.button_refresh.setEnabled(True)
        self.refresh_timer.start(60000)
    #end refresh()


    #
    #   Determine selected runs
    #   Return run number, datasetID, and directory
    #   Use actual table entries to handle sorting
    #
    def selected_runs(self):
        # Option 1
        # Rows where all columns are selected (entire row must be selected)
        #indexes = self.table.selectionModel().selectedRows()
        #for index in sorted(indexes):
        #    print('1: Row %d is selected' % index.row())

        # Option 2
        # Rows where at least one cell is selected (any box in row is selected)
        rows = sorted(set(index.row() for index in self.table.selectedIndexes()))
        #for row in rows:
        #    print('2: Row %d is selected' % row)


        # Extract info from selected rows
        run_out = []
        dataset_out = []
        directory_out = []
        for row in rows:
            #print("Row: ", row)
            run = self.table.item(row,0).text()
            dataset = self.table.item(row,1).text()
            directory = self.table.item(row,5).text()
            #print("Run is: ", run)
            #print("Dataset is: ", dataset)
            #print("Directory is: ", directory)
            run_out.append(run)
            dataset_out.append(dataset)
            directory_out.append(directory)

        # Relatve path for data directories (simplifies later lookup - we mostly want to know where to look for the data)
        path_out = []
        for dir in directory_out:
            path = self.config['hdf5dir'] + '/' + dir + '/'
            path_out.append(path)

        # Return value
        result = {
            'run' : run_out,
            'dataset' : dataset_out,
            'directory' : directory_out,
            'path' : path_out
        }
        return result
    #end selected_runs


    #
    #   Read list of past experiments
    #
    def list_experiments(self):

        expfile = '~/.cheetah-crawler'
        #expfile = './cheetah-crawler'

        # Does it exist?
        if os.path.exists(expfile):
            f = open(expfile)
            exptlist = f.readlines()
            f.close()
        else:
            print('File not found: ', expfile)
            exptlist = []

        # Remove carriage returns
        for i in range(len(exptlist)):
            exptlist[i] = exptlist[i].strip()

        #print("Past experiments:")
        #print(exptlist)
        return exptlist
    #end list_experiments

    #
    #   Select an experiment, or find a new one
    #
    def select_experiment(self):

        # Dialog box with list of past experiments
        past_expts = self.list_experiments()
        gui = gui_dialogs.expt_select_gui.get_expt(past_expts)

        if gui['action'] == 'goto':
            dir = gui['selected_expt']
            return dir

        elif gui['action'] == 'find':
            cfile = cfel_file.dialog_pickfile(filter='crawler.config')
            if cfile == '':
                print('Selection canceled')
                self.exit_gui()

            basename = os.path.basename(cfile)
            dir = os.path.dirname(cfile)

            # Update the past experiments list
            # expfile = '~/.cheetah-crawler'
            expfile = './cheetah-crawler'
            past_expts.insert(0,dir)
            with open(expfile, mode='w') as f:
                f.write('\n'.join(past_expts))

            return dir

        elif gui['action'] == 'setup_new':
            print('Set up new experiment not yet working :-(')
            self.exit_gui()

        else:
            print("Catch you another time.")
            self.exit_gui()

        return result


    #
    #   Action button items
    #
    def run_cheetah(self):
        # Dialog box for dataset label and ini file
        inifile_list = glob.glob('../process/*.ini')
        inifile_list = ['test1.ini','test2.ini']
        gui, ok = gui_dialogs.run_cheetah_gui.cheetah_dialog(inifile_list)
        dataset = gui['dataset']
        inifile = gui['inifile']

        # Exit if cancel was pressed
        if ok == False:
            return

        # Process all selected runs
        runs = self.selected_runs()
        for run in runs['run']:
            cmdarr = [self.config['process'], run, inifile, dataset]
            self.spawn_subprocess(cmdarr)

        #TODO: Update dataset file and Cheetah status in table

    #end run_cheetah()


    def run_crystfel(self):
        print("Run crystfel selected")

    def view_hits(self):
        file = '*.cxi'
        field = '/entry_1/data_1/data'
        self.show_selected_images(file, field)
    #end view_hits()


    def view_powder(self):
        self.show_powder_hits_det()
    #end view_powder()

    def view_peakogram(self):
        self.show_peakogram()
    #end view_peakogram()


    #
    #   Cheetah menu items
    #
    def enableCommands(self):
        self.ui.button_runCheetah.setEnabled(True)
        self.ui.menu_file_startcrawler.setEnabled(True)
        self.ui.menu_cheetah_processselected.setEnabled(True)
        self.ui.menu_cheetah_autorun.setEnabled(True)
        self.ui.menu_cheetah_relabel.setEnabled(True)
    #end enableCommands()

    def start_crawler(self):
        cmdarr = ['cheetah-crawler.py', '-l', 'LCLS', '-d', self.config['xtcdir'], '-c', self.config['hdf5dir']]
        self.spawn_subprocess(cmdarr)


    def relabel_dataset(self):
        print("Relabel dataset selected")

    def autorun(self):
        print("Autorun selected")


    #
    #   Mask menu items
    #
    def maskmaker(self):
        print("Mask maker selected")

    def badpix_from_darkcal(self):
        print("Badpix from darkcal selected")

    def badpix_from_bright(self):
        print("Badpix from bright selected")

    def combine_masks(self):
        print("Combine masks selected")

    #
    #   Analysis menu items
    #
    def show_hitrate(self):
        print("Show hitrate not yet implemented")

    def show_peakogram(self):
        runs = self.selected_runs()
        if len(runs['run']) == 0:
            return;
        pkfile = runs['path'][0] + 'peaks.txt'
        cmdarr = ['peakogram.py', '-i', pkfile]
        self.spawn_subprocess(cmdarr)
    #end show_peakogram

    def show_resolution(self):
        print("Show resolution not yet implemented")

    def show_saturation(self):
        self.show_peakogram()


    #
    #   Powder menu items
    #
    def show_powder_hits(self):
        file = '*detector0-class1-sum.h5'
        field = 'data/non_assembled_detector_and_photon_corrected'
        self.show_selected_images(file, field)

    def show_powder_blanks(self):
        file = '*detector0-class0-sum.h5'
        field = 'data/non_assembled_detector_and_photon_corrected'
        self.show_selected_images(file, field)

    def show_powder_hits_det(self):
        file = '*detector0-class1-sum.h5'
        field = 'data/non_assembled_detector_corrected'
        self.show_selected_images(file, field)

    def show_powder_blanks_det(self):
        file = '*detector0-class0-sum.h5'
        field = 'data/non_assembled_detector_corrected'
        self.show_selected_images(file, field)

    def show_powder_peaks_hits(self):
        file = '*detector0-class1-sum.h5'
        field = 'data/peakpowder'
        self.show_selected_images(file, field)

    def show_powder_peaks_blanks(self):
        file = '*detector0-class0-sum.h5'
        field = 'data/peakpowder'
        self.show_selected_images(file, field)


    #
    # Log menu actions
    #
    def view_batch_log(self):
        print("View batch log selected")

    def view_cheetah_log(self):
        print("View cheetah log selected")

    def view_cheetah_status(self):
        print("View cheetah status selected")



    #
    #   Parse crawler.config file
    #
    def parse_config(self):
        configfile = 'crawler.config'
        config= {}
        with open(configfile) as f:
            for line in f:
                name, var = line.partition("=")[::2]
                config[name.strip()] = var.strip()

        #print("Cheetah configuration:")
        #print(config)
        return config
    #end parse_config


    #
    #   Try to exit cleanly
    #
    def exit_gui(self):
        print("Bye bye.")
        app.exit()
        os._exit(1)
        sys.exit(1)



    #
    #	GUI Initialisation function
    #
    def __init__(self, args):

        # Extract info from command line arguments
        #self.hdf5_dir = args.c


        # Experiment selector
        expdir = self.select_experiment()
        print("Moving to working directory:", expdir)
        os.chdir(expdir)


        # Parse configuration file
        self.config = self.parse_config()


        #
        # Set up the UI
        #
        super(cheetah_gui, self).__init__()
        self.ui = UI.cheetahgui_ui.Ui_MainWindow()
        self.ui.setupUi(self)
        self.ui.menuBar.setNativeMenuBar(False)
        self.table = self.ui.table_status
        self.table.horizontalHeader().setResizeMode(PyQt4.QtGui.QHeaderView.Stretch)
        self.table.setSortingEnabled(True)


        # Connect front panel buttons to actions
        self.ui.button_refresh.clicked.connect(self.refresh_table)
        self.ui.button_runCheetah.clicked.connect(self.run_cheetah)
        self.ui.button_index.clicked.connect(self.run_crystfel)
        self.ui.button_viewhits.clicked.connect(self.view_hits)
        self.ui.button_virtualpowder.clicked.connect(self.view_powder)
        self.ui.button_peakogram.clicked.connect(self.view_peakogram)

        # File menu actions
        self.ui.menu_file_startcrawler.triggered.connect(self.start_crawler)
        self.ui.menu_file_refreshtable.triggered.connect(self.refresh_table)

        # Cheetah menu actions
        self.ui.menu_cheetah_processselected.triggered.connect(self.run_cheetah)
        self.ui.menu_cheetah_relabel.triggered.connect(self.relabel_dataset)
        self.ui.menu_cheetah_autorun.triggered.connect(self.autorun)

        # Mask menu actions
        self.ui.menu_mask_maker.triggered.connect(self.maskmaker)
        self.ui.menu_mask_badpixdark.triggered.connect(self.badpix_from_darkcal)
        self.ui.menu_mask_badpixbright.triggered.connect(self.badpix_from_bright)
        self.ui.menu_mask_combine.triggered.connect(self.combine_masks)
        self.ui.menu_mask_makecspadgain.setEnabled(False)
        self.ui.menu_mask_translatecspadgain.setEnabled(False)

        # Analysis menu items
        self.ui.menu_analysis_hitrate.triggered.connect(self.show_hitrate)
        self.ui.menu_analysis_peakogram.triggered.connect(self.show_peakogram)
        self.ui.menu_analysis_resolution.triggered.connect(self.show_resolution)
        self.ui.menu_analysis_saturation.triggered.connect(self.show_saturation)

        # Powder menu actions
        self.ui.menu_powder_hits.triggered.connect(self.show_powder_hits)
        self.ui.menu_powder_blank.triggered.connect(self.show_powder_blanks)
        self.ui.menu_powder_hits_det.triggered.connect(self.show_powder_hits_det)
        self.ui.menu_powder_blank_det.triggered.connect(self.show_powder_blanks_det)
        self.ui.menu_powder_peaks_hits.triggered.connect(self.show_powder_peaks_hits)
        self.ui.menu_powder_peaks_blank.triggered.connect(self.show_powder_peaks_blanks)

        # Log menu actions
        self.ui.menu_log_batch.triggered.connect(self.view_batch_log)
        self.ui.menu_log_cheetah.triggered.connect(self.view_cheetah_log)
        self.ui.menu_log_cheetahstatus.triggered.connect(self.view_cheetah_status)
        self.ui.menu_log_cheetah.setEnabled(False)
        self.ui.menu_log_cheetahstatus.setEnabled(False)


        # Disable action commands until enabled
        #self.ui.button_runCheetah.setEnabled(False)
        self.ui.button_index.setEnabled(False)
        self.ui.button_view.setEnabled(False)
        self.ui.menu_file_startcrawler.setEnabled(False)
        self.ui.menu_cheetah_processselected.setEnabled(False)
        self.ui.menu_cheetah_autorun.setEnabled(False)
        self.ui.menu_cheetah_relabel.setEnabled(False)
        self.ui.menu_file_command.triggered.connect(self.enableCommands)


        # Timer for auto-refresh
        self.refresh_timer = PyQt4.QtCore.QTimer()
        self.refresh_timer.timeout.connect(self.refresh_table)


        #
        # Populate the table
        #
        self.refresh_table()
    #end __init()__

#end cheetah_gui()

        
#
#	Main function defining this as a program to be called
#
if __name__ == '__main__':
    
    #
    #   Use parser to process command line arguments
    #    
    parser = argparse.ArgumentParser(description='CFEL cheetah crawler')
    parser.add_argument("-l", default="none", help="Location (LCLS, P11)")
    parser.add_argument("-d", default="none", help="Data directory (XTC, CBF, etc)")
    parser.add_argument("-c", default="none", help="Cheetah HDF5 directory")
    args = parser.parse_args()
    
    print("----------")    
    print("Parsed command line arguments")
    print(args)
    print("----------")    
    
    # This bit may be irrelevent if we can make parser.parse_args() require this field    
    #if args.l == "none" or args.d == "none" or args.c == "none":
    #    print('Usage: cheetah-gui.py')
    #    sys.exit()
    #endif        


    #
    #   Spawn the viewer
    #        
    app = PyQt4.QtGui.QApplication(sys.argv)    
        
    ex = cheetah_gui(args)
    ex.show()
    ret = app.exec_()

    
    #
    # Cleanup on exit    
    #
    app.exit()
    
    # This function does the following in an attempt to ‘safely’ terminate the process:
    #   Invoke atexit callbacks
    #   Close all open file handles
    os._exit(ret)
    sys.exit(ret)
#end __main__