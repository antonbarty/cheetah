# -*- coding: utf-8 -*-
#
#	CFEL image handling tools
#	Anton Barty
#

import os
import glob
import shutil
import lib.cfel_filetools as cfel_file
import lib.gui_dialogs as gui_dialogs


#
#   Index nolatt
#   This could be combined later with the more general indexing methods..
#
def index_nolatt(dirbase = 'temp', geomfile = None):

    # Data location and destination location
    h5dir = "../hdf5/" + dirbase
    indexdir = "../indexing/" + dirbase
    print("Launching mosflm-nolatt")
    print(h5dir)
    print(indexdir)
    print(os.path.relpath(geomfile))

    # Remove contents of any existing directory then recreate directory
    shutil.rmtree(indexdir, ignore_errors=True)
    os.makedirs(indexdir, exist_ok=True)

    # Use find to create file list
    cmd = 'find '+ os.path.abspath(h5dir) + ' -name \*.cxi > ' + indexdir+'/files.lst'
    print(cmd)
    os.system(cmd)

    # Copy scripts and calibrations to target directory
    crystfel_scriptname = 'index_nocell.sh'
    cmdarr = ['cp', '../process/'+crystfel_scriptname, indexdir+'/.']
    cfel_file.spawn_subprocess(cmdarr, wait=True)
    cmdarr = ['cp', geomfile, indexdir + '/.']
    cfel_file.spawn_subprocess(cmdarr, wait=True)

    # Send indexing command to batch farm
    qlabel = 'indx-'+dirbase[1:5]
    logfile = 'bsub.log'
    abspath = os.path.abspath(indexdir)
    bsub_cmd = ['bsub', '-q', 'psanaq', '-x', '-J', qlabel, '-o', logfile, '-cwd', abspath, 'source', './index_nocell.sh', dirbase, os.path.basename(geomfile)]


    # Submit it
    cfel_file.spawn_subprocess(bsub_cmd)
    return


#
#   Launch indexing with known unit cell (with dialog)
#
def index_pdb(dirs = None, geomfile = None):

    # Just some info
    print("Will process the following directories:")
    print(dirs)

    # Launch dialog box for CrystFEL options
    dialog_in = {
        'pdb_files' : glob.glob('../calib/pdb/*.pdb'),
        'geom_files' : glob.glob('../calib/geometry/*.geom'),
        'recipe_files' : glob.glob('../process/index*.sh'),
        'default_geom' : geomfile
    }
    dialog_out, ok = gui_dialogs.run_crystfel_dialog.dialog_box(dialog_in)

    print(dialog_out)
    pdbfile = dialog_out['pdbfile']
    geomfile = dialog_out['geomfile']
    recipefile = dialog_out['recipefile']
    geomfile = os.path.abspath(geomfile)

    # Exit if cancel was pressed
    if ok == False:
        return

    #
    # Loop through selected directories
    # Much of this repeats what is in index-nolatt.... simplify later
    #
    for dirbase in dirs:
        # Data location and destination location
        print(dir)
        h5dir = "../hdf5/" + dirbase
        indexdir = "../indexing/" + dirbase

        # Remove contents of any existing directory then recreate directory
        shutil.rmtree(indexdir, ignore_errors=True)
        os.makedirs(indexdir, exist_ok=True)

        # Use find to create file list
        cmd = 'find ' + os.path.abspath(h5dir) + ' -name \*.cxi > ' + indexdir + '/files.lst'
        print(cmd)
        os.system(cmd)

        # Copy scripts and calibrations to target directory
        cmdarr = ['cp', recipefile , indexdir + '/.']
        cfel_file.spawn_subprocess(cmdarr, wait=True)
        cmdarr = ['cp', pdbfile, indexdir + '/.']
        cfel_file.spawn_subprocess(cmdarr, wait=True)
        cmdarr = ['cp', geomfile, indexdir + '/.']
        cfel_file.spawn_subprocess(cmdarr, wait=True)


        # Send indexing command to batch farm
        qlabel = 'indx-'+dirbase[1:5]
        logfile = 'bsub.log'
        abspath = os.path.abspath(indexdir)
        bsub_cmd = ['bsub', '-q', 'psanaq', '-x', '-J', qlabel, '-o', logfile, '-cwd', abspath, 'source', os.path.basename(recipefile), dirbase, os.path.basename(pdbfile), os.path.basename(geomfile)]

        # Submit it
        cfel_file.spawn_subprocess(bsub_cmd)
        return


#
#   Merge stream files
#
def merge_streams(qtmainwin=None):

    # Files to merge
    stream_in = cfel_file.dialog_pickfile(path='../indexing/streams', filter='*.stream', multiple=True, qtmainwin=qtmainwin)
    if len(stream_in) is 0:
        return

    # Output stream name
    stream_out = cfel_file.dialog_pickfile(path='../indexing/streams', filter='*.stream', write=True, qtmainwin=qtmainwin)
    if stream_out is '':
        return

    # Remove destination (if it exists)
    try:
        os.remove(stream_out)
    except:
        pass

    # Concatenate stream files
    for stream in stream_in:
        cmd = 'cat ' + stream + ' >> ' + stream_out
        print(cmd)
        os.system(cmd)

    return

