#
#   Crawler routines for P11
#   Tested using Anaconda / Python 3.4
#

import sys
import os
import glob
import csv
import lib.cfel_filetools as cfel_file



def scan_data(data_dir):
    #print("Crawler data: ", data_dir)

    debug = False

    # Create sorted file list (glob seems to return files in random order)
    run_dirs = os.listdir(data_dir)
    run_dirs.sort()

    if debug:
        print(run_dirs)


    # Find unique run values (due to multiple XTC files per run)
    run_list = list(sorted(set(run_dirs)))
    nruns = len(run_list)


    # Default status for each is ready (ie: directory exists)
    status = ['Ready']*nruns


    # Status tag is number of .cbf files
    for i, dir in enumerate(run_list):
        str = data_dir+'/'+dir+'/**/*.cbf'
        cbf = glob.glob(str, recursive=True)
        status[i] = len(cbf)
        # Debugging
        #print(str)
        #print(len(cbf), cbf)

    # Create the result
    result = {
        'run': run_list,
        'status' : status
    }

    if debug:
        print(result['run'])

    # Write dict to CSV file
    keys_to_save = ['run','status']
    cfel_file.dict_to_csv('data_status.csv', result, keys_to_save)

#end scan_data

