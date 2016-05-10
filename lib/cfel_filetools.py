# -*- coding: utf-8 -*-
#
#	CFEL file handling tools
#	Anton Barty
#

import h5py
import glob
import numpy as np


# Needed for dialog_pickfile()
import PyQt4
import PyQt4.QtGui
import sys
qtApp = PyQt4.QtGui.QApplication(sys.argv)


def dialog_pickfile(write=False, directory=False, multiple=False, filter='*.*'):
    """
    In [1]: import PyQt4
    In [2]: from PyQt4 import QtGui
    In [8]: import sys
    The following line is already present in cxiview - may not always need it
    In [9]: app = QtGui.QApplication(sys.argv)

    In [14]: a = QtGui.QMainWindow()
    In [15]: fname = QtGui.QFileDialog.getOpenFileName(a, 'Open file','/data')
    In [16]: print(fname)
    /Data/work/2016/lcls-scheduling-run_13_0-2.pdf
    In [17]: fname = QtGui.QFileDialog.getSaveFileName(a, 'Open file','/data')
    :return:
    gui.QFileDialog.getExistingDirectory(a)

    """


def file_search(pattern, recursive=True, iterator=False):
    """
    :param pattern:
    :param recursive: True/False (default=True, '**' matches directories)
    :param iterator:
    :return:
    """
    if iterator:
        files = glob.iglob(pattern, recursive=recursive)
    else:
        files = glob.glob(pattern, recursive=recursive)
    return files



def read_h5(filename, field="/data/data"):
    """
    Read a simple HDF5 file

    if n_elements(filename) eq 0 then $
        filename = dialog_pickfile()
    """

    # Open HDF5 file
    fp = h5py.File(filename, 'r')

    # Read the specified field
    data = fp[field][:]

    # Close and clean up
    fp.close()

    # return
    return data
# end read_h5




def write_h5(filename, field="data/data", compress=3):
    """ 
    Write a simple HDF5 file

    IDL code
    	if n_elements(filename) eq 0 then begin
		print,'write_simple_hdf5: No filename specified...'
		return
	endif
	
	if n_elements(data) eq 0 then begin
		print,'write_simple_hdf5: No data specified...'
		return
	endif
	dim = size(data,/dimensions)
	
	
	if not keyword_set(compress) then begin
		compress=3
	endif
	

	;; HDF5 compression chunks
	chunksize = dim
	if n_elements(dim) ge 3 then $
	;;	chunksize[0] = dim[0]/8;
	;;	chunksize[0:n_elements(chunksize)-3] = 1

	
	fid = H5F_CREATE(filename) 
	group_id = H5G_CREATE(fid, 'data')
	datatype_id = H5T_IDL_CREATE(data) 
	dataspace_id = H5S_CREATE_SIMPLE(dim) 

	if (compress eq 0) then begin
		dataset_id = H5D_CREATE(fid,'data/data',datatype_id,dataspace_id) 
	endif else begin
		;; GZIP keyword is ignored if CHUNK_DIMENSIONS is not specified.
		dataset_id = H5D_CREATE(fid,'data/data',datatype_id,dataspace_id, gzip=compression,  chunk_dimensions=chunksize) 
		;dataset_id = H5D_CREATE(fid,'data/data',datatype_id,dataspace_id, chunk_dimensions=chunksize, gzip=compression, /shuffle) 
	endelse

	H5D_WRITE,dataset_id,data 
	H5D_CLOSE,dataset_id   
	H5S_CLOSE,dataspace_id 
	H5T_CLOSE,datatype_id 
	H5G_CLOSE,group_id
	H5F_CLOSE,fid 
     """


# end write_h5


def read_cxi(filename, frameID=0, data=False, mask=False, peaks=False, photon_energy=False, camera_length=False, num_frames=False, slab_size=False):
    """ 
    Read a frame from multi-event CXI file
    Also read mask and peak lists if requested
    Would be smarter to read the requested stuff at once and return it all at the same time from the same file handle
    :param filename:
    :param frameID:
    :param mask:
    :param peaks:
    :param photon_energy:
    :param camera_length:
    :param slab_size:
    :return:
    """

    # Open CXI file
    hdf5_fh = h5py.File(filename, 'r')


    # Peak list
    if peaks == True:
        n_peaks = hdf5_fh['/entry_1/result_1/nPeaks'][frameID]
        peakXPosRaw = hdf5_fh['/entry_1/result_1/peakXPosRaw'][frameID]
        peakYPosRaw = hdf5_fh['/entry_1/result_1/peakYPosRaw'][frameID]
        peak_xy = (peakXPosRaw.flatten(), peakYPosRaw.flatten())
    else:
        n_peaks = 0
        peakXPosRaw = np.nan
        peakYPosRaw = np.nan


    # Masks
    if mask == True:
        mask_array = hdf5_fh['/entry_1/data_1/mask'][frameID, :, :]
    else:
        mask_array = np.nan


    # Photon energy
    if photon_energy == True:
        photon_energy_eV = hdf5_fh['/LCLS/photon_energy_eV'][frameID]
        #return photon_energy_eV
    else:
        photon_energy_eV = np.nan


    # Camera length
    if camera_length == True:
        EncoderValue = hdf5_fh['/LCLS/detector_1/EncoderValue'][frameID]
    else:
        EncoderValue = np.nan

    # Array dimensions
    if slab_size == True:
        size = hdf5_fh['/entry_1/data_1/data'].shape
    else:
        size = [0,0,0]

    # Number of frames
    if num_frames == True:
        # For files which have finished being written this can be inferred from the data array shape
        # For files still being written there are blank frames at the end, so look for non-zero entries in x_pixel_size
        # The minimum of these two values is the number of events actually written so far
        size = hdf5_fh['/entry_1/data_1/data'].shape
        nframes_1 = size[0]

        pix_size = hdf5_fh['entry_1/instrument_1/detector_1/x_pixel_size'][:]
        nframes_2 = len(np.where(pix_size != 0 )[0])

        nframes = np.min([nframes_1, nframes_2])
    else:
        nframes = -1

    # Image data
    if data == True:
        data_array = hdf5_fh['/entry_1/data_1/data'][frameID, :, :]
    else:
        data_array = np.nan


    # Close file
    hdf5_fh.close()


    # Build return structure
    result = {
        'data' : data_array,
        'mask' : mask_array,
        'size' : size,
        'nframes' : nframes,
        'EncoderValue' : EncoderValue,
        'photon_energy_eV' : photon_energy_eV,
        'n_peaks' : n_peaks,
        'peakXPosRaw' : peakXPosRaw,
        'peakYPosRaw' : peakYPosRaw
    }
    return result
# end read_cxi




def list_events_from_file(pattern='./*.cxi', field='data/data'):
    """
    :param file_pattern: Single filename, or search string
    :param field: HDF5 field from which to draw data, can be different for each file, default='data/data'
    :return: List of filenames, eventID and HDF5 field

    reload:
    import importlib
    importlib.reload(lib.cfel_filetools)
    from lib.cfel_filetools import *
    """

    # "field==none" means "use default value"
    if field=='none':
        field = 'data/data'

    # Find all files matching pattern
    files = glob.glob(pattern, recursive=True)
    if len(files) == 0:
        print('No files found matching pattern: ', pattern)


    # List the found files (sanity check)
    #print('Found files:')
    #for filename in glob.iglob(pattern, recursive=True):
    #    print(filename)

    # Create empty event list
    filename_out = []
    eventid_out = []
    fieldname_out = []
    format_out = []


    print('Found files:')
    for filename in glob.iglob(pattern, recursive=True):

        # CXI file
        if filename.endswith(".cxi"):
            # Number of events in file
            nframes = read_cxi(filename, num_frames=True)['nframes']
            if nframes == 0:
                break

            # Default location for data in .cxi files is not data/data
            # But leave option for passing a different hdf5 data path on the command line
            cxi_field = field
            if cxi_field == 'data/data':
                cxi_field = '/entry_1/data_1/data'

            # Generate lists for this file
            cxi_filename = [filename] * nframes
            cxi_eventid = list(range(nframes))
            cxi_fieldname = [cxi_field] * nframes
            cxi_format = ['cxi'] * nframes

            # Append to main list
            filename_out.extend(cxi_filename)
            eventid_out.extend(cxi_eventid)
            fieldname_out.extend(cxi_fieldname)
            format_out.extend(cxi_format)
            #endif

        # Assume .h5 file is a single frame data file (for now)
        # Be more clever about generalising this later on
        # (eg: if number of dimensions of field = 2, it's an image; if number of dimensions = 3 it's a slab)
        if filename.endswith(".h5"):
            nframes = 1
            if nframes == 0:
                break

            filename_out.extend([filename])
            eventid_out.extend([0])
            fieldname_out.extend([field])
            format_out.extend(['h5_single'])
            #endif

        filename_short = filename.split('/')[-1]
        print(filename_short, '    ', nframes)
    #endfor

    nevents = len(filename_out)
    #print('Events found: ', nevents)


    # Build return structure
    result = {
        'nevents' : nevents,
        'filename': filename_out,
        'event': eventid_out,
        'field': fieldname_out,
        'format': format_out
    }
    return result

# end find_cheetah_images


