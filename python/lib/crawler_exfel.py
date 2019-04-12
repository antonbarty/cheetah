#
#   Crawler routines for EuXFEL
#   Tested using Anaconda / Python 3.4
#

import sys
import os
import glob
import csv
import lib.cfel_filetools as cfel_file

def are_agipd_files_ready (files): 
   highest_sn = 0 
   for pn in range(0,16): 
       files_panel_pn = sorted([ x for x in files if 'AGIPD{:02d}'.format(pn) in x ]) 
       if len(files_panel_pn) != 0:
          highest_sn = int(files_panel_pn[-1].split('.')[0].split('S')[-1]) 
       else:
          return False
   for pn in range(0,16): 
      for sn in range (0, highest_sn): 
          pattern_to_search = 'AGIPD{pn:02d}-S{sn:05d}.h5'.format(pn=pn, sn=sn) 
          if not any(pattern_to_search in x for x in files): 
              return False 
   return True                                                                                                                                                             

def scan_data(data_dir):
    #print("Crawler data: ", data_dir)

    pattern = data_dir + '/r*'
    debug = False

    # Create sorted file list (glob seems to return files in random order)
    rundirs = glob.glob(pattern)
    if debug:
        print(pattern)
        print(rundirs)

    # Strip the preceeding stuff
    for i in range(len(rundirs)):
        rundirs[i] = os.path.basename(rundirs[i])
    #files.sort()

    if debug:
        print(rundirs)

    # Extract the run bit from XTC file name
    # Turn the rXXXX string into an integer (for compatibility with old crawler files)
    run_list = []
    for filename in rundirs:
        #filebase=os.path.basename(filename)
        thisrun = filename[1:]
        run_list.append(thisrun)

    #print('Number of XTC files: ', len(out))


    # Find unique run values (due to multiple XTC files per run)
    #run_list = list(sorted(set(out)))

    nruns = len(run_list)
    print('Number of unique runs: ', nruns)


    # Default status for each is ready
    status = ['Copying']*nruns


    # Check directory contents for AGIPD files
    for dir in rundirs:

        run = dir[1:]

        pattern = data_dir + '/' + dir + '/*AGIPD*'
        files = glob.glob(pattern)
        #files = os.path.basename(files)

        # Case of no AGIPD files (but run directory has been created)
        if len(files) is 0:
            run_indx = run_list.index(run)
            status[run_indx] = 'noAGIPD'
            continue

        # Check whether all AGIPD files are there
        if not are_agipd_files_ready(files):
            run_indx = run_list.index(run)
            status[run_indx] = 'Incomplete'
        else:
            run_indx = run_list.index(run)
            status[run_indx] = 'Ready'


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


    # Some notes pinched from earlier attempts at doing things

    #for filename in glob.iglob(pattern):


    # Split a string
    #>>> '1,2,3'.split(',')
    #['1', '2', '3']

    #
    #>> > ["foo", "bar", "baz"].index("bar")
    #1

    # Find unique values
    # l = ['r0001', 'r0002', 'r0002', 'r0003']
    # s = set(l)
    # s = sorted(s)
    # ll = list(s)


