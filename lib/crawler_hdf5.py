#
#   Crawler routines for Cheetah HDF5 directory
#   Tested using Anaconda / Python 3.4
#

import sys
import os
import glob
import csv
import lib.cfel_filetools as cfel_file


def scan_hdf5(hdf5_dir):
    print("Crawler HDF5: ", hdf5_dir)

    pattern = hdf5_dir + '/r*/status.txt'

    #printf, fout, '# Run, status, directory, processed, hits, hitrate%, mtime'

    run_out = []
    status_out = []
    directory_out = []
    processed_out = []
    hits_out = []
    hitrate_out = []
    mtime_out = []

    # Create sorted file list or files come in seemingly random order
    files = glob.glob(pattern)
    files.sort()

    #for filename in glob.iglob(pattern):        #, recursive=True):
    for filename in files:        #, recursive=True):

        # Default values are blanks
        run = ''
        status = ''
        directory = ''
        processed = ''
        hits = ''
        hitrate = ''
        mtime = ''

        # Extract the Cheetah HDF5 directory name
        basename = os.path.basename(filename)
        dirname = os.path.dirname(filename)
        dirname2 = os.path.basename(dirname)
        directory = dirname2

        # Extract the run number (Warning: LCLS-specific)
        run = directory[1:5]



        #print(filename)
        f = open(filename, 'r')
        for line in f:
            #print(line, end='')
            part = line.partition(':')

            if part[0] == 'Status':
                status = part[2].strip()

            if part[0] == 'Frames processed':
                processed = part[2].strip()

            if part[0] == 'Number of hits':
                hits= part[2].strip()
        #endfor
        f.close()


        # Calculate hit rate (with some error checking)
        if hits != '' and processed != '' and processed != '0':
            hitrate = 100 * ( float(hits) / float(processed))
        else:
            hitrate='---'

        # Diagnostic
        debug = False
        if debug:
            print("---------------")
            print("Run: ", run)
            print(directory)
            print(status)
            print(processed)
            print(hits)
            print(hitrate)

        # Append to main list
        run_out.append(run)
        directory_out.append(directory)
        status_out.append(status)
        processed_out.append(processed)
        hits_out.append(hits)
        hitrate_out.append(str(hitrate))
        mtime_out.append(mtime)
    #endfor


    # Create the result
    result = {
        'run' : run_out,
        'status' : status_out,
        'directory' : directory_out,
        'processed' : processed_out,
        'hits' : hits_out,
        'hitrate%': hitrate_out
    }


    # Write dict to CSV file
    keys_to_save = ['run','status','directory','processed','hits','hitrate%']
    cfel_file.dict_to_csv('test.csv', result, keys_to_save )




