# -*- coding: utf-8 -*-
#
#	CFEL image handling tools
#	Anton Barty
#

import os
import glob
import shutil
import lib.cfel_filetools as cfel_file



def index_nolatt(dirbase = 'temp', geomfile = None):

    # Data location and destination location
    h5dir = "../hdf5/" + dirbase
    indexdir = "../indexing/" + dirbase
    print("Launching mosflm-nolatt")
    print(h5dir)
    print(indexdir)
    print(os.path.relpath(geomfile))


    # Remove contents of any existing directory
    shutil.rmtree(indexdir, ignore_errors=True)

    # Create target directory
    os.makedirs(indexdir, exist_ok=True)

    # Find data files and save to files.lst
    #files = glob.glob(h5dir+"/*.cxi", recursive=True)
    #f = open(indexdir+"/files.lst", "w")
    #for i in files:
    #    abspath = os.path.abspath(i)
    #    print(abspath)
    #    f.write(str(abspath) + '\n')
    #f.close()

    # Use find to create file list
    cmd = 'find '+ os.path.abspath(h5dir) + ' -name \*.cxi > ' + indexdir+'/files.lst'
    print(cmd)
    os.system(cmd)


    # Copy indexing script from ../process/ to target directory
    crystfel_scriptname = 'index_nocell.sh'
    cmdarr = ['cp', '../process/'+crystfel_scriptname, indexdir+'/.']
    cfel_file.spawn_subprocess(cmdarr, wait=True)
    #os.system(str.join(' ', cmdarr))

    #
    #   Directly send indexamajig command to bsub
    #   Downside: Any change requires users to edit Python code rather than shell command
    #   Less intimidating to call an indexing script instead
    #
    # CrystFEL command
    #dir_cmd = 'cd '+ os.path.abspath(indexdir)
    #cf_cmd = 'indexamajig -i files.lst -o nolatt.stream -g ' + geomfile + ' --indexing=mosflm-raw-nolatt-nocell  --peaks=cxi --check-hdf5-snr --min-snr=5 -j 32'
    #cf_cmd_split = cf_cmd.split(' ')

    # Make a record of the CrystFEL command
    #f = open(indexdir+"/index.cmd", "w")
    #f.write(cf_cmd + '\n')
    #f.close

    # Send indexing command to batch farm
    qlabel = 'indx'+dirbase[1:5]
    logfile = 'bsub.log'
    abspath = os.path.abspath(indexdir)
    #bsub_cmd = ['bsub', '-q', 'psanaq', '-x', '-J', qlabel, '-o', logfile, '-cwd', abspath]
    bsub_cmd = ['bsub', '-q', 'psanaq', '-x', '-J', qlabel, '-o', logfile, '-cwd', abspath, 'source', './index_nocell.sh', dirbase, geomfile]


    # Submit it
    cfel_file.spawn_subprocess(bsub_cmd)
    return
