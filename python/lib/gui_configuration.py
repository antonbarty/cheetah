#
#   Functions to configure Cheetah for various locations
#   Extracts template skeleton and modifies configuration files accordingly
#


import os
import glob
import PyQt5.QtCore
import PyQt5.QtWidgets

import lib.cfel_filetools as cfel_file
import lib.gui_dialogs as gui_dialogs
import lib.gui_locations as gui_locations



#
#   Extract the LCLS template and modify to suit determined experiment
#
def extract_lcls_template(self):
    #   Deduce experiment number, etc using de-referenced paths
    #   Assumes the file path follows the pattern:   /reg/d/psdm/cxi/cxij4915/scratch/...
    realdir = os.getcwd()

    #   Now for some LCLS-specific stuff
    ss = realdir.split('/')
    ss = ss[1:]

    ss[1] = 'd'
    ss[2] = 'psdm'
    instr = ss[3]
    expt = ss[4]
    xtcdir = '/' + str.join('/', ss[0:5]) + '/xtc'
    userdir = '/' + str.join('/', ss) + '/cheetah'

    print('Deduced experiment information:')
    print('    Relative path: ', dir)
    print('    Absolute path: ', realdir)
    print('    Instrument: ', instr)
    print('    Experiment: ', expt)
    print('    XTC directory: ', xtcdir)
    print('    Output directory: ', userdir)

    #
    # QMessageBox for confirmation before proceeding
    #
    msgBox = PyQt5.QtWidgets.QMessageBox()
    str1 = []
    str1.append('<b>Instrument:</b> ' + str(instr))
    str1.append('<b>Experiment:</b> ' + str(expt))
    str1.append('<b>XTC directory:</b> ' + str(xtcdir))
    str1.append('<b>Output directory:</b> ' + str(userdir))
    # str1.append('<b>Relative path:</b> '+ str(dir))
    str2 = str.join('<br>', str1)
    msgBox.setText(str2)

    msgBox.setInformativeText('<b>Proceed?</b>')
    msgBox.addButton(PyQt5.QtWidgets.QMessageBox.Yes)
    msgBox.addButton(PyQt5.QtWidgets.QMessageBox.Cancel)
    msgBox.setDefaultButton(PyQt5.QtWidgets.QMessageBox.Yes)

    ret = msgBox.exec_();
    # app.exit()


    if ret == PyQt5.QtWidgets.QMessageBox.Cancel:
        print("So long and thanks for all the fish.")
        self.exit_gui()



    # Unpack template
    print('>---------------------<')
    print('Extracting template...')
    cmd = ['tar', '-xf', '/reg/g/cfel/cheetah/template.tar']
    cfel_file.spawn_subprocess(cmd, wait=True)
    print("Done")

    # Fix permissions
    print('>---------------------<')
    print('Fixing permissions...')
    cmd = ['chgrp', '-R', expt, 'cheetah/']
    cfel_file.spawn_subprocess(cmd, wait=True)
    cmd = ['chmod', '-R', 'g+w', 'cheetah/']
    cfel_file.spawn_subprocess(cmd, wait=True)
    print("Done")

    # Place configuration into /res
    # print, 'Placing configuration files into /res...'
    # cmd = ['/reg/g/cfel/cheetah/cheetah-stable/bin/make-labrynth']
    # cfel_file.spawn_subprocess(cmd, wait=True)
    # print("Done")


    # Modify gui/crawler.config
    print('>---------------------<')
    file = 'cheetah/gui/crawler.config'
    print('Modifying ', file)

    # Replace XTC directory with the correct location using sed
    xtcsedstr = str.replace(xtcdir, '/', '\\/')
    cmd = ["sed", "-i", "-r", "s/(xtcdir=).*/\\1" + xtcsedstr + "/", file]
    cfel_file.spawn_subprocess(cmd, wait=True)

    print('>-------------------------<')
    cmd = ['cat', file]
    cfel_file.spawn_subprocess(cmd, wait=True)
    print('>-------------------------<')

    # Modify process/process
    file = 'cheetah/process/process'
    print('Modifying ', file)

    cmd = ["sed", "-i", "-r", "s/(expt=).*/\\1\"" + expt + "\"/", file]
    cfel_file.spawn_subprocess(cmd, wait=True)

    xtcsedstr = str.replace(xtcdir, '/', '\\/')
    cmd = ["sed", "-i", "-r", "s/(XTCDIR=).*/\\1\"" + xtcsedstr + "\"/", file]
    cfel_file.spawn_subprocess(cmd, wait=True)

    h5sedstr = str.replace(userdir, '/', '\\/') + '\/hdf5'
    cmd = ["sed", "-i", "-r", "s/(H5DIR=).*/\\1\"" + h5sedstr + "\"/", file]
    cfel_file.spawn_subprocess(cmd, wait=True)

    confsedstr = str.replace(userdir, '/', '\\/') + '\/process'
    cmd = ["sed", "-i", "-r", "s/(CONFIGDIR=).*/\\1\"" + confsedstr + "\"/", file]
    cfel_file.spawn_subprocess(cmd, wait=True)

    print('>-------------------------<')
    cmd = ['head', file]
    cfel_file.spawn_subprocess(cmd, wait=True)
    print('>-------------------------<')

    print("Working directory: ")
    print(os.getcwd() + '/cheetah')
    #end extract_lcls_template()


    #
    # Modify the configuration files
    #
    modify_cheetah_config_files(self)


#
#   Modify a bunch of configuration files
#   Separate function so that it can be called from the menu as well as on startup
#
def modify_cheetah_config_files(self):

    # Dialog box
    result, ok = gui_dialogs.configure_cheetah_lcls_gui.configure_cheetah_dialog()

    if ok == False:
        print('Configuration scripts remain unchanged')
        return

    # This is needed because on startup we are one directory up from cheetah/gui, but when running we are in cheetah/gui
    dir = os.getcwd()
    if "cheetah/gui" in dir:
        prefix = '..'
    else:
        prefix = 'cheetah'


    # Modify detector type string
    print("Modifying detector type...")
    detectorType = result["detectorType"]
    for file in glob.iglob(prefix+'/process/*.ini'):
        cmd = ["sed", "-i", "-r", "/^detectorType/s/(detectorType=).*/\\1" + detectorType + "/", file]
        cfel_file.spawn_subprocess(cmd, wait=True)

    # Modify detector Z encoder string
    print("Modifying detector Z encoder...")
    detectorZencoder = result["detectorZEncoder"]
    for file in glob.iglob(prefix+'/process/*.ini'):
        cmd = ["sed", "-i", "-r", "/^detectorZpvname/s/(detectorZpvname=).*/\\1" + detectorZencoder + "/", file]
        cfel_file.spawn_subprocess(cmd, wait=True)


    # Modify data source in psana.cfg
    # Unfortunately we gave different detectors different fields so we have to do some guessing
    print("Modifying data source...")
    file = prefix+'/process/psana.cfg'
    dataSource = result["dataSource"]
    if 'cspad' in dataSource.lower():
        print('Cspad = ', dataSource)
        cmd = ["sed", "-i", "-r", "/^cspadSource0/s/(cspadSource0 = ).*/\\1DetInfo(" + dataSource + ")/", file]
        cfel_file.spawn_subprocess(cmd, wait=True)
    if 'rayonix' in dataSource.lower():
        print('Rayonix = ', dataSource)
        cmd = ["sed", "-i", "-r", "/^rayonixSource0/s/(rayonixSource0 = ).*/\\1DetInfo(" + dataSource + ")/", file]
        cfel_file.spawn_subprocess(cmd, wait=True)
    if 'pnccd' in dataSource.lower():
        print('pnCCD = ', dataSource)
        cmd = ["sed", "-i", "-r", "/^pnccdSource0/s/(pnccdSource0 = ).*/\\1DetInfo(" + dataSource + ")/", file]
        cfel_file.spawn_subprocess(cmd, wait=True)


    print("Done modifying configuration scripts")

    return
    #end modify_cheetah_config_files()
