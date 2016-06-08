#
#   Crawler routines for SLAC/XTC
#   Tested using Anaconda / Python 3.4
#

import sys
import os
import glob
import csv
import lib.cfel_filetools as cfel_file



def crawler_merge():

    data_status = cfel_file.csv_to_dict('data_status.csv')
    cheetah_status = cfel_file.csv_to_dict('cheetah_status.csv')


    # Diagnostics
    print('Keys: ', data_status.keys())
    print('Length: ', len( data_status[list(data_status.keys())[0]] ))
    #print('Runs: ', data_status['run'])
    print('Keys: ', cheetah_status.keys())
    print('Length: ', len(cheetah_status[list(cheetah_status.keys())[0]]))





