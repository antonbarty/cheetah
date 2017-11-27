#!/usr/bin/env python3
#
#   cheetah-gui
#   A python/Qt replacement for the IDL cheetah GUI
#

import os
import sys
import glob
import argparse
import datetime
import PyQt5.QtCore
import PyQt5.QtWidgets

import UI.cheetahgui_ui
import lib.cfel_filetools as cfel_file
import lib.cfel_detcorr as cfel_detcorr
import lib.gui_dialogs as gui_dialogs
import lib.gui_locations as gui_locations
import lib.gui_configuration as gui_configuration
import lib.gui_crystfel_bridge as gui_crystfel


#TODO: Cheetah GUI
#TODO: Calibration table (darkcal, mask, .ini for each run)

#TODO: cxiview.py
#TODO: Take file list as an argument



#
#	Cheetah GUI code
#
class cheetah_gui(PyQt5.QtWidgets.QMainWindow):


    #
    # Quick wrapper for viewing images (this code got repeated over-and-over)
    #
    def show_selected_images(self, filepat, field='data/data'):
        runs = self.selected_runs()

        # No run selected
        if len(runs['run']) == 0:
            return
        file = runs['path'][0] + filepat

        # Display if some file matches pattern (avoids error when no match)
        if len(glob.glob(file)) != 0:
            cmdarr = ['cxiview.py', '-g', self.config['geometry'], '-e', field, '-i', file]
            cfel_file.spawn_subprocess(cmdarr)
        else:
            print("File does not seem to exist:")
            print(file)
    #end show_selected_images()



    #
    #   Actions to be done each refresh cycle
    #
    def refresh_table(self):
        print('Table refreshed at ', str(datetime.datetime.now()))

        # Button is busy
        self.ui.button_refresh.setEnabled(False)

        # Load the table data
        status = cfel_file.csv_to_dict('crawler.txt')

        # Fix some legacy issues with old crawler.txt file format (different key names in crawler.txt)
        if not 'Run' in status.keys() and '#Run' in status.keys():
            status.update({'Run': status['#Run']})
            status.update({'H5Directory': status['H5 Directory']})
            del status['#Run']
            del status['H5 Directory']
            status['fieldnames'][status['fieldnames'].index('#Run')] = 'Run'
            status['fieldnames'][status['fieldnames'].index('H5 Directory')] = 'H5Directory'

        # Remember table for later use in other functions
        ncols = len(list(status.keys()))-1
        self.crawler_txt = status

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

                #if col in numbercols:
                if item.isnumeric():
                    newitem = PyQt5.QtWidgets.QTableWidgetItem()
                    newitem.setData(PyQt5.QtCore.Qt.DisplayRole, float(item))
                else:
                    newitem = PyQt5.QtWidgets.QTableWidgetItem(item)

                self.table.setItem(row,col,newitem)

                # Coloring of table elements
                self.table.item(row,col).setBackground(PyQt5.QtGui.QColor(255,255,255))
                if key=='XTC':
                    if item=='Ready':
                        self.table.item(row, col).setBackground(PyQt5.QtGui.QColor(200, 255, 200))
                    if item == 'Copying' or item == 'Restoring':
                        self.table.item(row, col).setBackground(PyQt5.QtGui.QColor(255, 255, 100))

                if key=='Cheetah':
                    if item=='Finished':
                        self.table.item(row, col).setBackground(PyQt5.QtGui.QColor(200, 255, 200))
                    if item=='Not finished' or item=='Started':
                        self.table.item(row, col).setBackground(PyQt5.QtGui.QColor(0, 255, 255))
                    if item=='Submitted':
                        self.table.item(row, col).setBackground(PyQt5.QtGui.QColor(255, 255, 100))
                    if item=='Terminated':
                        self.table.item(row, col).setBackground(PyQt5.QtGui.QColor(255, 200, 200))
                    if item=='Error':
                        self.table.item(row, col).setBackground(PyQt5.QtGui.QColor(255, 100, 100))


        # Table fiddling
        #TODO: Make columns resizeable
        self.table.setWordWrap(False)
        self.table.setHorizontalHeaderLabels(status['fieldnames'])
        self.table.verticalHeader().setVisible(False)
        #self.table.resizeColumnsToContents()
        #self.table.horizontalHeader().setSectionResizeMode(PyQt5.QtWidgets.QHeaderView.Interactive)
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
            'row' : rows,
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

        expfile = os.path.expanduser('~/.cheetah-crawler')

        # Does it exist?
        if os.path.isfile(expfile):
            f = open(expfile)
            exptlist = f.readlines()
            f.close()
        else:
            print('File not found: ', expfile)
            exptlist = []

        # Remove carriage returns
        for i in range(len(exptlist)):
            exptlist[i] = exptlist[i].strip()

        return exptlist
    #end list_experiments


    #
    #   Set up a new experiment based on template and selected directory
    #   Uses LCLS schema; will need modification to run anywhere else; do this later
    #
    def setup_new_experiment(self):
        dir = cfel_file.dialog_pickfile(directory=True, qtmainwin=self)
        if dir == '':
            self.exit_gui()
        print('Selected directory: ')
        os.chdir(dir)

        # Use LCLS schema for now; will need modification to run anywhere else; do this later
        gui_configuration.extract_lcls_template(self)

    #end setup_new_experiment


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
            cfile = cfel_file.dialog_pickfile(filter='crawler.config', qtmainwin=self)
            if cfile == '':
                print('Selection canceled')
                self.exit_gui()

            basename = os.path.basename(cfile)
            dir = os.path.dirname(cfile)

            # Update the past experiments list
            past_expts.insert(0,dir)
            expfile = os.path.expanduser('~/.cheetah-crawler')
            with open(expfile, mode='w') as f:
                f.write('\n'.join(past_expts))
            return dir


        elif gui['action'] == 'setup_new':
            self.setup_new_experiment()
            cwd = os.getcwd()
            dir = cwd + '/cheetah/gui'

            # Update the past experiments list
            past_expts.insert(0,dir)
            expfile = os.path.expanduser('~/.cheetah-crawler')
            with open(expfile, mode='w') as f:
                f.write('\n'.join(past_expts))
            return dir

        else:
            print("Catch you another time.")
            self.exit_gui()
            return ''




    #
    #   Action button items
    #
    def run_cheetah(self):

        # Find .ini files for dropdown list
        inifile_list = []
        for file in glob.iglob('../process/*.ini'):
            basename = os.path.basename(file)
            inifile_list.append(basename)
        #inifile_list = ['test1.ini','test2.ini']

        # Info needed for the dialog box
        dialog_info = {
            'inifile_list' : inifile_list,
            'lastini' : self.lastini,
            'lasttag' : self.lasttag
        }
        # Dialog box for dataset label and ini file
        gui, ok = gui_dialogs.run_cheetah_gui.cheetah_dialog(dialog_info)

        # Exit if cancel was pressed
        if ok == False:
            return

        # Extract values from return dict
        dataset = gui['dataset']
        inifile = gui['inifile']
        self.lasttag = dataset
        self.lastini = inifile


        dataset_csv = cfel_file.csv_to_dict('datasets.csv')

        # Failing to read the dataset file looses all information (bad)
        if len(dataset_csv['DatasetID']) is 0:
            print("Error reading datasets.csv (blank file)")
            print("Try again...")
            return


        # Process all selected runs
        runs = self.selected_runs()
        for i, run in enumerate(runs['run']):
            print('------------ Start Cheetah process script ------------')
            cmdarr = [self.config['process'], run, inifile, dataset]
            cfel_file.spawn_subprocess(cmdarr, shell=True)

            # Format output directory string
            # This clumsily selects between using run numbers and using directory names
            # We need to fix this up sometime
            print("Location: ", self.compute_location['location'])
            if 'LCLS' in self.compute_location['location']:
                dir = 'r{:04d}'.format(int(run))
            elif 'max-exfl' in self.compute_location['location']:
                dir = 'r{:04d}'.format(int(run))
            elif 'max-cfel' in self.compute_location['location']:
                dir = 'r{:04d}'.format(int(run))
            else:
                dir = run
            dir += '-'+dataset
            print('Output directory: ', dir)


            #Update Dataset and Cheetah status in table
            table_row = runs['row'][i]
            self.table.setItem(table_row, 1, PyQt5.QtWidgets.QTableWidgetItem(dataset))
            self.table.setItem(table_row, 5, PyQt5.QtWidgets.QTableWidgetItem(dir))
            self.table.setItem(table_row, 3, PyQt5.QtWidgets.QTableWidgetItem('Submitted'))
            self.table.item(table_row, 3).setBackground(PyQt5.QtGui.QColor(255, 255, 100))

            # Update dataset file
            if run in dataset_csv['Run']:
                ds_indx = dataset_csv['Run'].index(run)
                dataset_csv['DatasetID'][ds_indx] = dataset
                dataset_csv['Directory'][ds_indx] = dir
                dataset_csv['iniFile'][ds_indx] = inifile
            else:
                dataset_csv['Run'].append(run)
                dataset_csv['DatasetID'].append(dataset)
                dataset_csv['Directory'].append(dir)
                dataset_csv['iniFile'].append(inifile)
            print('------------ Finish Cheetah process script ------------')

        # Sort dataset file to keep it in order


        # Save datasets file
        keys_to_save = ['Run', 'DatasetID', 'Directory', 'iniFile']
        cfel_file.dict_to_csv('datasets.csv', dataset_csv, keys_to_save)
    #end run_cheetah()


    #
    #   Action button items
    #
    def run_XFEL_detectorcalibration(self):
        runs = self.selected_runs()
        if len(runs) is 0:
            print('No runs selected')
            return

        for i, run in enumerate(runs['run']):
            print('------------ Start XFEL detector calibration script ------------')

            #cmdarr = [self.config['process'], run]
            cmdarr = ["../process/calibrate_euxfel.sh", run]
            cfel_file.spawn_subprocess(cmdarr, shell=True)




    def view_hits(self):
        file = '*.cxi'
        field = '/entry_1/data_1/data'
        # Quick hack for P11 data
        if 'p11' in self.config['xtcdir']:
            file = '*.cbf*.h5'
            field = '/data/data'

        self.show_selected_images(file, field)
    #end view_hits()



    def view_peakogram(self):
        self.show_peakogram()
    #end view_peakogram()


    #
    #   Cheetah menu items
    #
    def enableCommands(self):
        self.ui.button_runCheetah.setEnabled(True)
        self.ui.button_index.setEnabled(True)
        self.ui.menu_file_startcrawler.setEnabled(True)
        self.ui.menu_cheetah_processselected.setEnabled(True)
        self.ui.menu_cheetah_autorun.setEnabled(True)
        self.ui.menu_cheetah_relabel.setEnabled(True)
    #end enableCommands()

    def start_crawler(self):
        cmdarr = ['cheetah-crawler.py', '-l', self.compute_location['location'], '-d', self.config['xtcdir'], '-c', self.config['hdf5dir'], '-i', '../indexing/']
        cfel_file.spawn_subprocess(cmdarr) #, shell=True)

    def modify_beamline_config(self):
        gui_configuration.modify_cheetah_config_files(self)

    def relabel_dataset(self):

        # Simple dialog box: http: // www.tutorialspoint.com / pyqt / pyqt_qinputdialog_widget.htm
        text, ok = PyQt5.QtWidgets.QInputDialog.getText(self, 'Change dataset label', 'New label:')
        if ok == False:
            return
        newlabel = str(text)
        print('New label is: ', newlabel)

        dataset_csv = cfel_file.csv_to_dict('datasets.csv')

        # Label all selected runs
        runs = self.selected_runs()
        for i, run in enumerate(runs['run']):

            # Format directory string
            olddir = runs['directory'][i]
            newdir = '---'

            if olddir != '---':
                if 'LCLS' in self.compute_location['location']:
                    newdir = 'r{:04d}'.format(int(run))
                elif 'max-exfl' in self.compute_location['location']:
                    newdir = 'r{:04d}'.format(int(run))
                else:
                    newdir = run
                newdir += '-' + newlabel

            # Update Dataset in table
            table_row = runs['row'][i]
            self.table.setItem(table_row, 1, PyQt5.QtWidgets.QTableWidgetItem(newlabel))
            self.table.setItem(table_row, 5, PyQt5.QtWidgets.QTableWidgetItem(newdir))

            # Update dataset file
            if run in dataset_csv['Run']:
                ds_indx = dataset_csv['Run'].index(run)
                dataset_csv['DatasetID'][ds_indx] = newlabel
                dataset_csv['Directory'][ds_indx] = newdir
            else:
                dataset_csv['Run'].append(run)
                dataset_csv['DatasetID'].append(newlabel)
                dataset_csv['Directory'].append(newdir)
                dataset_csv['iniFile'].append('---')

            # Rename the directory
            if olddir != '---':
                if os.path.exists(self.config['hdf5dir'] + '/' + olddir):
                    try:
                        cmdarr = ['mv', self.config['hdf5dir']+'/'+olddir, self.config['hdf5dir']+'/'+newdir]
                        cfel_file.spawn_subprocess(cmdarr)
                    except:
                        pass

                if os.path.exists('../indexing/' + olddir):
                    try:
                        cmdarr = ['mv', '../indexing/'+olddir, '../indexing/'+newdir]
                        cfel_file.spawn_subprocess(cmdarr)
                    except:
                        pass



        # Sort dataset file to keep it in order
        # Save datasets file
        keys_to_save = ['Run', 'DatasetID', 'Directory', 'iniFile']
        cfel_file.dict_to_csv('datasets.csv', dataset_csv, keys_to_save)



    def autorun(self):
        print("Autorun selected")

    def set_new_geometry(self):
        gfile = cfel_file.dialog_pickfile(path='../calib/geometry', filter='Geometry files (*.h5 *.geom);;All files (*.*)', qtmainwin=self)
        if gfile == '':
            return
        gfile = os.path.relpath(gfile)
        print('Selected geometry file:')
        print(gfile)
        self.config['geometry'] = gfile


    #
    #   Mask menu items
    #
    def maskmaker(self):
        print("Mask maker selected")
        print("Talk to Andrew Morgan to add his mask maker")

    def badpix_from_darkcal(self):
        runs = self.selected_runs()
        if len(runs['run']) == 0:
            return;

        path = runs['path'][0]
        path += '*detector?-class0-sum.h5'
        for dkcal in glob.iglob(path):
            cfel_detcorr.badpix_from_darkcal(dkcal, self)



    def badpix_from_bright(self):
        print("Badpix from bright selected")

    def combine_masks(self):
        print("Combine masks selected")

    def show_mask_file(self):
        file = cfel_file.dialog_pickfile(path='../calib/mask', filter='*.h5', qtmainwin=self)
        field = 'data/data'
        if file != '':
            file = os.path.relpath(file)
            self.show_selected_images(file, field)

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
        cfel_file.spawn_subprocess(cmdarr)
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

    def show_powder_all_det(self):
        file = '*detector0-class*-sum.h5'
        field = 'data/non_assembled_detector_corrected'
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
    #   Calibration menu items
    #
    def show_darkcal(self):
        file = '*detector0-darkcal.h5'
        field = 'data/data'
        self.show_selected_images(file, field)

    def copy_darkcal(self):
        runs = self.selected_runs()
        if len(runs['run']) == 0:
            return;
        for path in runs['path']:
            path += '*detector0-darkcal.h5'
            for dkcal in glob.iglob(path):
                cmdarr = ['cp', dkcal, '../calib/darkcal/.']
                cfel_file.spawn_subprocess(cmdarr)

    def set_darkrun(self):
        # This bit repeats copy_darkcal, but allows only one run and remembers filename for symlink
        runs = self.selected_runs()
        if len(runs['run']) == 0:
            return;
        if len(runs['run']) > 1:
            print('Can only select one run')
            return;

        # Copy files
        path = runs['path'][0]
        path += '*detector0-darkcal.h5'
        for dkcal in glob.iglob(path):
            cmdarr = ['cp', dkcal, '../calib/darkcal/.']
            cfel_file.spawn_subprocess(cmdarr)

        # Create symbolic link
        file = os.path.relpath(dkcal)
        basename = os.path.basename(file)
        cmdarr = ['ln', '-fs', basename, '../calib/darkcal/current-darkcal.h5']
        cfel_file.spawn_subprocess(cmdarr)


    def set_current_darkcal(self):
        file = cfel_file.dialog_pickfile(path='../calib/darkcal', filter='*.h5', qtmainwin=self)
        if file != '':
            file = os.path.relpath(file)
            basename = os.path.basename(file)
            dirname = os.path.dirname(file)
            cmdarr = ['ln', '-fs', basename, '../calib/darkcal/current-darkcal.h5']
            cfel_file.spawn_subprocess(cmdarr)

    def set_current_badpix(self):
        file = cfel_file.dialog_pickfile(path='../calib/mask', filter='*.h5', qtmainwin=self)
        if file != '':
            file = os.path.relpath(file)
            basename = os.path.basename(file)
            dirname = os.path.dirname(file)
            cmdarr = ['ln', '-fs', basename, '../calib/mask/current-badpix.h5']
            cfel_file.spawn_subprocess(cmdarr)

    def set_current_peakmask(self):
        file = cfel_file.dialog_pickfile(path='../calib/mask', filter='*.h5', qtmainwin=self)
        if file != '':
            file = os.path.relpath(file)
            basename = os.path.basename(file)
            dirname = os.path.dirname(file)
            cmdarr = ['ln', '-fs', basename, '../calib/mask/current-peakmask.h5']
            cfel_file.spawn_subprocess(cmdarr)

    def set_current_geometry(self):
        file = cfel_file.dialog_pickfile(path='../calib/geometry', filter='*.h5', qtmainwin=self)
        if file != '':
            file = os.path.relpath(file)
            basename = os.path.basename(file)
            dirname = os.path.dirname(file)
            cmdarr = ['ln', '-fs', basename, '../calib/geometry/current-geometry.h5']
            cfel_file.spawn_subprocess(cmdarr)

    def set_cxiview_geom(self):
        file = cfel_file.dialog_pickfile(path='../calib/geometry', filter='*.geom', qtmainwin=self)
        if file != '':
            file = os.path.relpath(file)
            basename = os.path.basename(file)
            dirname = os.path.dirname(file)
            cmdarr = ['ln', '-fs', basename, '../calib/geometry/current-geometry.geom']
            cfel_file.spawn_subprocess(cmdarr)

    def geom_to_pixelmap(self):
        file = cfel_file.dialog_pickfile(path='../calib/geometry', filter='*.geom', qtmainwin=self)
        if file != '':
            print('Sorry - not yet implemented...')


    #
    #   CrystFEL menu items
    #
    def crystfel_indexpdb(self):
        # Selected runs
        runs = self.selected_runs()
        if len(runs['run']) == 0:
            return

        dirs = runs['directory']
        gui_crystfel.index_runs(self, dirs)


    def crystfel_mosflmnolatt(self):
        # Selected runs
        runs = self.selected_runs()
        if len(runs['run']) == 0:
            return

        dirs = runs['directory']
        gui_crystfel.index_runs(self, dirs, nocell=True)


    def crystfel_viewindex(self):
        runs = self.selected_runs()
        if len(runs['run']) == 0:
            return;
        dir = '../indexing/'+runs['directory'][0]
        stream = glob.glob(dir+'/*.stream')
        if len(stream) is 0:
            return
        cmdarr = ['cxiview', '-s', stream[0]]
        cfel_file.spawn_subprocess(cmdarr)


    def crystfel_cellexplore(self):
        runs = self.selected_runs()
        if len(runs['run']) == 0:
            return;
        dir = '../indexing/'+runs['directory'][0]
        stream = glob.glob(dir+'/*.stream')
        if len(stream) is 0:
            return
        cmdarr = ['cell_explorer', stream[0]]
        cfel_file.spawn_subprocess(cmdarr)


    def crystfel_viewindex_pick(self):
        file = cfel_file.dialog_pickfile(path='../indexing/streams', filter='*.stream', qtmainwin=self)
        if file != '':
            cmdarr = ['cxiview', '-s', file]
            cfel_file.spawn_subprocess(cmdarr)


    def crystfel_cellexplore_pick(self):
        file = cfel_file.dialog_pickfile(path='../indexing/streams', filter='*.stream', qtmainwin=self)
        if file != '':
            cmdarr = ['cell_explorer', file]
            cfel_file.spawn_subprocess(cmdarr)



    def crystfel_mergestreams(self):
        gui_crystfel.merge_streams(qtmainwin=self)


    def crystfel_listevents(self):
        print("Not yet implemented")


    def crystfel_listfiles(self):
        print("Not yet implemented")





    #
    #   Parse crawler.config file
    #
    def parse_config(self):
        configfile = 'crawler.config'

        # Double-check again that the file exists
        if not os.path.isfile(configfile):
            print("Uh oh - there does not seem to be a crawler.config file in this directory.")
            print("Please check")
            self.exit_gui()

        config= {}
        with open(configfile) as f:
            for line in f:
                name, var = line.partition("=")[::2]
                config[name.strip()] = var.strip()


        # Trap some legacy case with fewer keys in file
        if not 'cheetahini' in config.keys():
            config.update({'cheetahini': 'darkcal.ini'})
        if not 'cheetahtag' in config.keys():
            config.update({'cheetahtag': 'darkcal'})


        return config
    #end parse_config



    #
    #   Try to avoid the worst of the ugly Linux GUI layouts
    #
    def change_skins(self):
        styles = PyQt5.QtWidgets.QStyleFactory.keys()
        style = styles[-1]
        #print("Available Qt5 styles: ", styles)
        #print("Setting Qt5 style: ", style)
        try:
            PyQt5.QtWidgets.QApplication.setStyle(PyQt5.QtWidgets.QStyleFactory.create(style))
        except:
            print("Qt style not available:", style)
    #end change_skins


    #
    #   Exit cleanly
    #
    def exit_gui(self):
        print("Bye bye.")
        app.exit()
        os._exit(1)
        sys.exit(1)
    #end exit_gui




    #
    #	GUI Initialisation function
    #
    def __init__(self, args):

        # Extract info from command line arguments
        #self.hdf5_dir = args.c

        #
        #   Where are we?
        #
        compute_location = gui_locations.determine_location()
        self.compute_location = gui_locations.set_location_configuration(compute_location )


        #
        # Set up the UI
        #
        super(cheetah_gui, self).__init__()
        self.ui = UI.cheetahgui_ui.Ui_MainWindow()
        self.ui.setupUi(self)
        self.ui.menuBar.setNativeMenuBar(False)
        self.table = self.ui.table_status
        self.table.horizontalHeader().setDefaultSectionSize(75)
        self.table.horizontalHeader().setResizeMode(PyQt5.QtWidgets.QHeaderView.Interactive)
        #self.table.horizontalHeader().setResizeMode(PyQt5.QtWidgets.QHeaderView.Stretch)
        self.table.setSortingEnabled(True)
        #self.change_skins()


        # Experiment selector (if not already in a gui directory)
        if not os.path.isfile('crawler.config'):
            expdir = self.select_experiment()
            print("Thank you.")
            print("Moving to working directory:")
            print(expdir)
            try:
                os.chdir(expdir)
            except:
                print("Uh oh - it looks like that directory does not exist any more.")
                print("It may have been moved or deleted.  Plesae check it still exists.")
                self.exit_gui()


        # Parse configuration file
        print("Loading configuration file: ./crawler.config")
        self.config = self.parse_config()
        self.lastini = self.config['cheetahini']
        self.lasttag = self.config['cheetahtag']
        self.lastindex = '../process/index_cell.sh'
        self.lastcell = None
        self.lastgeom = None

        # Update window title
        dir = os.getcwd()
        title = 'Cheetah GUI:  ' + dir
        self.setWindowTitle(title)


        # Connect front panel buttons to actions
        self.ui.button_refresh.clicked.connect(self.refresh_table)
        self.ui.button_runCheetah.clicked.connect(self.run_cheetah)
        self.ui.button_index.clicked.connect(self.crystfel_indexpdb)
        self.ui.button_viewhits.clicked.connect(self.view_hits)
        self.ui.button_blanksum.clicked.connect(self.show_powder_blanks_det)
        self.ui.button_virtualpowder.clicked.connect(self.show_powder_peaks_hits)
        self.ui.button_peakogram.clicked.connect(self.view_peakogram)

        # File menu actions
        self.ui.menu_file_startcrawler.triggered.connect(self.start_crawler)
        self.ui.menu_file_refreshtable.triggered.connect(self.refresh_table)
        self.ui.menu_file_newgeometry.triggered.connect(self.set_new_geometry)
        self.ui.menu_file_modify_beamline_configuration.triggered.connect(self.modify_beamline_config)

        # Cheetah menu actions
        self.ui.menu_cheetah_processselected.triggered.connect(self.run_cheetah)
        self.ui.menu_cheetah_relabel.triggered.connect(self.relabel_dataset)
        self.ui.menu_cheetah_autorun.triggered.connect(self.autorun)

        # Calibrations
        self.ui.menu_calib_darkcal.triggered.connect(self.show_darkcal)
        self.ui.menu_calib_copydarkcal.triggered.connect(self.copy_darkcal)
        self.ui.menu_calib_currentdarkcal.triggered.connect(self.set_current_darkcal)
        self.ui.menu_calib_setdarkrun.triggered.connect(self.set_darkrun)
        self.ui.menu_calib_currentbadpix.triggered.connect(self.set_current_badpix)
        self.ui.menu_calib_currentpeakmask.triggered.connect(self.set_current_peakmask)
        self.ui.menu_calib_currentgeom.triggered.connect(self.set_current_geometry)
        self.ui.menu_calib_cxiviewgeom.triggered.connect(self.set_cxiview_geom)
        self.ui.menu_calib_geomtopixmap.triggered.connect(self.geom_to_pixelmap)
        self.ui.menu_calib_XFELdetcorr.triggered.connect(self.run_XFEL_detectorcalibration)

        # CrystFEL actions
        self.ui.menu_crystfel_mosflmnolatt.triggered.connect(self.crystfel_mosflmnolatt)
        self.ui.menu_crystfel_indexpdb.triggered.connect(self.crystfel_indexpdb)
        self.ui.menu_crystfel_viewindexingresults.triggered.connect(self.crystfel_viewindex)
        self.ui.menu_crystfel_viewindexing_pick.triggered.connect(self.crystfel_viewindex_pick)
        self.ui.menu_crystfel_cellexplorer.triggered.connect(self.crystfel_cellexplore)
        self.ui.menu_crystfel_cellexplorer_pick.triggered.connect(self.crystfel_cellexplore_pick)
        self.ui.menu_crystfel_mergestreams.triggered.connect(self.crystfel_mergestreams)
        self.ui.menu_crystfel_listevents.triggered.connect(self.crystfel_listevents)
        self.ui.menu_crystfel_listfiles.triggered.connect(self.crystfel_listfiles)


        # Mask menu actions
        self.ui.menu_mask_maker.triggered.connect(self.maskmaker)
        self.ui.menu_mask_badpixdark.triggered.connect(self.badpix_from_darkcal)
        self.ui.menu_mask_badpixbright.triggered.connect(self.badpix_from_bright)
        self.ui.menu_mask_combine.triggered.connect(self.combine_masks)
        self.ui.menu_mask_view.triggered.connect(self.show_mask_file)
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
        self.ui.menu_powder_all.triggered.connect(self.show_powder_all_det)
        self.ui.menu_powder_all_det.triggered.connect(self.show_powder_all_det)

        # Log menu actions
        self.ui.menu_log_batch.triggered.connect(self.view_batch_log)
        self.ui.menu_log_cheetah.triggered.connect(self.view_cheetah_log)
        self.ui.menu_log_cheetahstatus.triggered.connect(self.view_cheetah_status)
        self.ui.menu_log_cheetah.setEnabled(False)
        self.ui.menu_log_cheetahstatus.setEnabled(False)


        # Disable action commands until enabled
        self.ui.button_runCheetah.setEnabled(False)
        self.ui.button_index.setEnabled(False)
        self.ui.menu_file_startcrawler.setEnabled(False)
        self.ui.menu_cheetah_processselected.setEnabled(False)
        self.ui.menu_cheetah_autorun.setEnabled(False)
        self.ui.menu_cheetah_relabel.setEnabled(False)
        self.ui.menu_file_command.triggered.connect(self.enableCommands)


        # Timer for auto-refresh
        self.refresh_timer = PyQt5.QtCore.QTimer()
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
    parser = argparse.ArgumentParser(description='CFEL cheetah GUI')
    #parser.add_argument("-l", default="none", help="Location (LCLS, P11)")
    #parser.add_argument("-d", default="none", help="Data directory (XTC, CBF, etc)")
    #parser.add_argument("-c", default="none", help="Cheetah HDF5 directory")
    args = parser.parse_args()
    
    print("----------")    
    print("Parsed command line arguments")
    print(args)
    print("----------")    
    

    #
    #   Spawn the viewer
    #
    app = PyQt5.QtWidgets.QApplication(sys.argv)
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
