#
#	CrystFEL Geometry parser
#	Valerio Mariani
#

import sys
import h5py
import numpy
import scipy.constants


"""
Return pixel and radius maps from CrystFEL format geometry file

Input: geometry filename

Output: x: slab-like pixel map with x coordinate of each slab pixel in the reference system of the detector
        y: slab-like pixel map with y coordinate of each slab pixel in the reference system of the detector
        z: slab-like pixel map with distance of each pixel from the center of the reference system.
"""
def pixelmap_from_CrystFEL_geometry_file(fnam):
    f = open(fnam, 'r')
    f_lines = []
    for line in f:
        f_lines.append(line)

    keyword_list = ['min_fs', 'min_ss', 'max_fs', 'max_ss', 'fs', 'ss', 'corner_x', 'corner_y']

    detector_dict = {}

    panel_lines = [ x for x in f_lines if '/' in x and len(x.split('/')) == 2 and x.split('/')[1].split('=')[0].strip() in keyword_list ]

    for pline in panel_lines:
        items = pline.split('=')[0].split('/')
        panel = items[0].strip()
        property = items[1].strip()
        if property in keyword_list:
            if panel not in detector_dict.keys():
                detector_dict[panel] = {}
            detector_dict[panel][property] = pline.split('=')[1].split(';')[0]
        #endif
    #endfor


    parsed_detector_dict = {}

    for p in detector_dict.keys():

        parsed_detector_dict[p] = {}

        parsed_detector_dict[p]['min_fs'] = int( detector_dict[p]['min_fs'] )
        parsed_detector_dict[p]['max_fs'] = int( detector_dict[p]['max_fs'] )
        parsed_detector_dict[p]['min_ss'] = int( detector_dict[p]['min_ss'] )
        parsed_detector_dict[p]['max_ss'] = int( detector_dict[p]['max_ss'] )
        parsed_detector_dict[p]['fs'] = []
        parsed_detector_dict[p]['fs'].append( float( detector_dict[p]['fs'].split('x')[0] ) )
        parsed_detector_dict[p]['fs'].append( float( detector_dict[p]['fs'].split('x')[1].split('y')[0] ) )
        parsed_detector_dict[p]['ss'] = []
        parsed_detector_dict[p]['ss'].append( float( detector_dict[p]['ss'].split('x')[0] ) )
        parsed_detector_dict[p]['ss'].append( float( detector_dict[p]['ss'].split('x')[1].split('y')[0] ) )
        parsed_detector_dict[p]['corner_x'] = float( detector_dict[p]['corner_x'] )
        parsed_detector_dict[p]['corner_y'] = float( detector_dict[p]['corner_y'] )
    #endfor


    max_slab_fs = numpy.array([parsed_detector_dict[k]['max_fs'] for k in parsed_detector_dict.keys()]).max()
    max_slab_ss = numpy.array([parsed_detector_dict[k]['max_ss'] for k in parsed_detector_dict.keys()]).max()


    x = numpy.zeros((max_slab_ss+1, max_slab_fs+1), dtype=numpy.float32)
    y = numpy.zeros((max_slab_ss+1, max_slab_fs+1), dtype=numpy.float32)

    for p in parsed_detector_dict.keys():
        # get the pixel coords for this asic
        i, j = numpy.meshgrid( numpy.arange(parsed_detector_dict[p]['max_ss'] - parsed_detector_dict[p]['min_ss'] + 1),
                               numpy.arange(parsed_detector_dict[p]['max_fs'] - parsed_detector_dict[p]['min_fs'] + 1), indexing='ij')

        #
        # make the y-x ( ss, fs ) vectors, using complex notation
        dx  = parsed_detector_dict[p]['fs'][1] + 1J * parsed_detector_dict[p]['fs'][0]
        dy  = parsed_detector_dict[p]['ss'][1] + 1J * parsed_detector_dict[p]['ss'][0]
        r_0 = parsed_detector_dict[p]['corner_y'] + 1J * parsed_detector_dict[p]['corner_x']
        #
        r   = i * dy + j * dx + r_0
        #
        y[parsed_detector_dict[p]['min_ss']: parsed_detector_dict[p]['max_ss'] + 1, parsed_detector_dict[p]['min_fs']: parsed_detector_dict[p]['max_fs'] + 1] = r.real
        x[parsed_detector_dict[p]['min_ss']: parsed_detector_dict[p]['max_ss'] + 1, parsed_detector_dict[p]['min_fs']: parsed_detector_dict[p]['max_fs'] + 1] = r.imag
    #endfor
            
    # Calculate radius
    r = numpy.sqrt(numpy.square(x) + numpy.square(y))

    return x, y, r

"""
Read Cheetah style pixelmap
(HDF5 file with fields "x", "y" and "z" containing pixel coordinates in meters)
"""
def read_pixelmap(filename):

    # Open HDF5 pixelmap file
    fp = h5py.File(filename, 'r')     
    x = fp['x'][:]
    y = fp['y'][:]
    fp.close()    


    # Correct for pixel size (meters --> pixels)
    # Currently hard coded for CSPAD
    x /= 110e-6
    y /= 110e-6
    

    # Calculate radius
    r = numpy.sqrt(numpy.square(x) + numpy.square(y))

    return x, y, r
    

"""
Read geometry files and return pixel map
A bit of a wrapper - determines file type and calls the appropriate routines
"""
def read_geometry(geometry_filename, format=format):
    
    # Later on we should determine the format automatically (eg: from filename extension)
    if format == 'pixelmap': 
        x, y, r  = read_pixelmap(geometry_filename)
    elif format == 'CrystFEL':
        x, y, r  = pixelmap_from_CrystFEL_geometry_file(geometry_filename)
    else:
        print('Unsupported geometry type')
    #endif        

    # find the smallest size of cspad_geom that contains all
    # xy values but is symmetric about the origin
    M = 2 * int(max(abs(x.max()), abs(x.min()))) + 2
    N = 2 * int(max(abs(y.max()), abs(y.min()))) + 2

    print(x.max(), x.min())
    print(y.max(), y.min())

    # convert x y values to i j values
    # Minus sign for y-axis because Python takes (0,0) in top left corner instead of bottom left corner
    i = numpy.array(x, dtype=numpy.int) + M/2 - 1
    j = numpy.array(-y, dtype=numpy.int) + N/2 - 1

    ij = (i.flatten(), j.flatten())
    img_shape = (M, N)
    return ij, img_shape    


"""
    Determine camera offset from geometry file
    Pixelmaps do not have these values so we have a temporary hack in place
"""
def read_geometry_coffset_and_res(geometry_filename, format=format):
    
    # Later on we should determine the format automatically (eg: from filename extension)
    if format == 'pixelmap': 
        coffset = 0.591754
        res = 9090.91
    elif format == 'CrystFEL':
        f = open(geometry_filename, 'r')
        f_lines = []
        for line in f:
            f_lines.append(line)
        #endfor
        coffset_lines = [ x for x in f_lines if 'coffset' in x]
        coffset = float(coffset_lines[-1].split('=')[1])
        res_lines = [ x for x in f_lines if 'res' in x]
        res = float(res_lines[-1].split('=')[1])
    else:
        print('Unsupported geometry type')
        coffset = numpy.inf
        res = numpy.inf
    #endif        


    return coffset, res
    #end extract_coffset_and_res()