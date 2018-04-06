#!/usr/bin/env python
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import argparse
import h5py
try:
    from PyQt5 import QtGui
except :
    from PyQt4 import QtGui
import pyqtgraph as pg
import numpy as np
import scipy
import geometry_funcs as gf
import signal
import os

try :
    import ConfigParser as configparser 
except ImportError :
    import configparser 

cspad_psana_shape = (4, 8, 185, 388)
cspad_geom_shape  = (1480, 1552)
pilatus_geom_shape = (2527, 2463)

def parse_parameters(config):
    """
    Parse values from the configuration file and sets internal parameter accordingly
    The parameter dictionary is made available to both the workers and the master nodes
    The parser tries to interpret an entry in the configuration file as follows:
    - If the entry starts and ends with a single quote, it is interpreted as a string
    - If the entry is the word None, without quotes, then the entry is interpreted as NoneType
    - If the entry is the word False, without quotes, then the entry is interpreted as a boolean False
    - If the entry is the word True, without quotes, then the entry is interpreted as a boolean True
    - If non of the previous options match the content of the entry, the parser tries to interpret the entry in order as:
        - An integer number
        - A float number
        - A string
      The first choice that succeeds determines the entry type
    """

    monitor_params = {}

    for sect in config.sections():
        monitor_params[sect]={}
        for op in config.options(sect):
            monitor_params[sect][op] = config.get(sect, op)
            if monitor_params[sect][op].startswith("'") and monitor_params[sect][op].endswith("'"):
                monitor_params[sect][op] = monitor_params[sect][op][1:-1]
                continue
            if monitor_params[sect][op] == 'None':
                monitor_params[sect][op] = None
                continue
            if monitor_params[sect][op] == 'False':
                monitor_params[sect][op] = False
                continue
            if monitor_params[sect][op] == 'True':
                monitor_params[sect][op] = True
                continue
            try:
                monitor_params[sect][op] = int(monitor_params[sect][op])
                continue
            except :
                try :
                    monitor_params[sect][op] = float(monitor_params[sect][op])
                    continue
                except :
                    # attempt to pass as an array of ints e.g. '1, 2, 3'
                    try :
                        l = monitor_params[sect][op].split(',')
                        monitor_params[sect][op] = np.array(l, dtype=np.int)
                        continue
                    except :
                        pass

    return monitor_params



def radial_symetry(background, rs = None, is_fft_shifted = True):
    if rs is None :
        i = np.fft.fftfreq(background.shape[0]) * background.shape[0]
        j = np.fft.fftfreq(background.shape[1]) * background.shape[1]
        k = np.fft.fftfreq(background.shape[2]) * background.shape[2]
        i, j, k = np.meshgrid(i, j, k, indexing='ij')
        rs      = np.sqrt(i**2 + j**2 + k**2).astype(np.int16)
        
        if is_fft_shifted is False :
            rs = np.fft.fftshift(rs)
        rs = rs.ravel()
    
    ########### Find the radial average
    # get the r histogram
    r_hist = np.bincount(rs)
    # get the radial total 
    r_av = np.bincount(rs, background.ravel())
    # prevent divide by zero
    nonzero = np.where(r_hist != 0)
    zero    = np.where(r_hist == 0)
    # get the average
    r_av[nonzero] = r_av[nonzero] / r_hist[nonzero].astype(r_av.dtype)
    r_av[zero]    = 0

    ########### Make a large background filled with the radial average
    background = r_av[rs].reshape(background.shape)
    return background, rs, r_av

def cheetah_mask(data, mask, x, y, adc_thresh=20, min_snr=6, counter = 5):
    # load options from maskMaker.ini so the user can update parameters on the fly
    ##############################################################################
    # check the ini file for cheetah parameters
    # fall back on defaults if not found
    print('looking for maskMaker.ini')
    ini = os.path.split(os.path.abspath(__file__))[0]
    ini = os.path.join(ini, 'maskMaker.ini')

    if os.path.exists(ini):
        print('found it! loading params...')
        config = configparser.ConfigParser()
        config.read(ini)	
        config = parse_parameters(config)
        adc_thresh = float(config['cheetah']['adc_thresh'])
        min_snr    = float(config['cheetah']['min_snr'])
        counter    = int(config['cheetah']['counter'])
    
    temp, rs, r_av = radial_symetry(data.astype(np.float), rs = np.rint(np.sqrt(x**2+y**2)).astype(np.int).ravel())

    roffset     = np.zeros_like(r_av, dtype=np.float)
    rsigma      = np.zeros_like(r_av, dtype=np.float)
    rcount      = np.zeros_like(r_av, dtype=np.float)
    rthreshold  = np.zeros_like(r_av, dtype=np.float)
    rthreshold[:] = 1e9
    
    mask_temp = mask.copy()
    
    for i in range(counter):
        # calculate radial average
        # calculate radial sigma
        ##########################
        rcount = np.bincount(rs, mask_temp.ravel())
        roffset = np.bincount(rs, (mask_temp*data).ravel())
        rsigma  = np.bincount(rs, (mask_temp*data**2).ravel())
        
        nonzero = rcount>0
        roffset[nonzero]  = roffset[nonzero] / rcount[nonzero].astype(np.float)
        roffset[~nonzero] = 1e9

        rsigma[nonzero]  = np.sqrt(rsigma[nonzero] / rcount[nonzero].astype(np.float) - roffset[nonzero]**2)
        rsigma[~nonzero] = 0.

        # calculate threshold
        #####################
        rthreshold = roffset + min_snr * rsigma
        rthreshold[rthreshold < adc_thresh] = adc_thresh

        mask_temp *= (data < rthreshold[rs].reshape(data.shape))
    return mask_temp


def make_pilatus_edges():
    panels      = (12, 5)
    panel_step  = (212, 494)
    panel_shape = (195, 487)

    gaps = np.ones(pilatus_geom_shape, dtype=np.bool)
    for i in range(panels[0]):
        for j in range(panels[1]):
            gaps[i*panel_step[0]-(panel_step[0]-panel_shape[0]):i*panel_step[0], :] = False
            gaps[:, j*panel_step[1]-(panel_step[1]-panel_shape[1]):j*panel_step[1]] = False
    gaps[:, 0] = False
    gaps[:, -1] = False
    gaps[0, :] = False
    gaps[-1, :] = False
    return gaps

def make_pilatus_sub_edges(
    panWid = 487,
    panHei = 195,
    vGap = 17,
    hGap = 7,
    spanWid = 56,
    spanHei = 91,
    svGap = 5,
    shGap = 5,
    svGapEdge = 4,
    shGapEdge = 2):
    # load options from maskMaker.ini so the user can update parameters on the fly
    ##############################################################################
    # check the ini file for cheetah parameters
    # fall back on defaults if not found
    print('looking for maskMaker.ini')
    ini = os.path.split(os.path.abspath(__file__))[0]
    ini = os.path.join(ini, 'maskMaker.ini')

    if os.path.exists(ini):
        print('found it! loading params...')
        config = configparser.ConfigParser()
        config.read(ini)	
        config = parse_parameters(config)
        panWid = int(config['pilatus']['panwid'])
        panHei = int(config['pilatus']['panhei'])
        vGap = int(config['pilatus']['vgap'])
        hGap = int(config['pilatus']['hgap'])
        spanWid = int(config['pilatus']['spanwid'])
        spanHei = int(config['pilatus']['spanhei'])
        svGap = int(config['pilatus']['svgap'])
        shGap = int(config['pilatus']['shgap'])
        svGapEdge = int(config['pilatus']['svgapedge'])
        shGapEdge = int(config['pilatus']['shgapedge'])

    mask = np.ones(pilatus_geom_shape, dtype=np.bool)
    maxSS, maxFS = mask.shape
    # parameters of the Pilatus panels
    nPanX = int(round((maxFS+hGap) / (panWid+hGap)))
    nPanY = int(round((maxSS+vGap) / (panHei+vGap)))
    if (nPanX*(panWid+hGap)-hGap != maxFS): print("something wrong with horizonal number of panels %d\n",nPanX)
    if (nPanY*(panHei+vGap)-vGap != maxSS): print("something wrong with vertical number of panels %d\n",nPanY)
    nsPanX = int(round((panWid-2*shGapEdge+shGap) / (spanWid+shGap)))
    nsPanY = int(round((panHei-2*svGapEdge+svGap) // (spanHei+svGap)))
    
    # mask gaps
    for pxi in range(nPanX-1):
        mask[:, (panWid+hGap)*(pxi+1)-hGap : (panWid+hGap)*(pxi+1)] = False
    
    for pyi in range(nPanY-1):
        mask[(panHei+vGap)*(pyi+1)-vGap : (panHei+vGap)*(pyi+1), :] = False
    
    for pxi in range(nPanX):
        for pyi in range(nPanY):
            conX = pxi*(panWid+hGap)
            conY = pyi*(panHei+vGap)
            
            # between sub-asics vertically
            for spxi in range(nsPanX-1):
                mask[conY:conY+panHei, conX+shGapEdge+(spanWid+shGap)*(spxi+1)-shGap : conX+shGapEdge+(spanWid+shGap)*(spxi+1)] = False

            # vertical edges of panels
            for spxi in range(2):
                mask[conY:conY+panHei, conX+(panWid-shGapEdge-1)*spxi: conX+(panWid-shGapEdge-1)*spxi+shGapEdge+1] = False

            # between sub-asics horizontally
            for spyi in range(nsPanY-1):
                mask[conY+svGapEdge+(spanHei+svGap)*(spyi+1)-svGap : conY+svGapEdge+(spanHei+svGap)*(spyi+1), conX:conX+panWid] = False

            # horizontal edges of panels
            for spyi in range(2):
                mask[conY+(panHei-svGapEdge)*spyi: conY+(panHei-svGapEdge)*spyi+svGapEdge, conX:conX+panWid] = False
    return mask


def unbonded_pixels():
    def ijkl_to_ss_fs(cspad_ijkl):
        """ 
        0: 388        388: 2 * 388  2*388: 3*388  3*388: 4*388
        (0, 0, :, :)  (1, 0, :, :)  (2, 0, :, :)  (3, 0, :, :)
        (0, 1, :, :)  (1, 1, :, :)  (2, 1, :, :)  (3, 1, :, :)
        (0, 2, :, :)  (1, 2, :, :)  (2, 2, :, :)  (3, 2, :, :)
        ...           ...           ...           ...
        (0, 7, :, :)  (1, 7, :, :)  (2, 7, :, :)  (3, 7, :, :)
        """
        if cspad_ijkl.shape != cspad_psana_shape :
            raise ValueError('cspad input is not the required shape:' + str(cspad_psana_shape) )

        cspad_ij = np.zeros(cspad_geom_shape, dtype=cspad_ijkl.dtype)
        for i in range(4):
            cspad_ij[:, i * cspad_psana_shape[3]: (i+1) * cspad_psana_shape[3]] = cspad_ijkl[i].reshape((cspad_psana_shape[1] * cspad_psana_shape[2], cspad_psana_shape[3]))

        return cspad_ij

    mask = np.ones(cspad_psana_shape)

    for q in range(cspad_psana_shape[0]):
        for p in range(cspad_psana_shape[1]):
            for a in range(2):
                for i in range(19):
                    mask[q, p, i * 10, i * 10] = 0
                    mask[q, p, i * 10, i * 10 + cspad_psana_shape[-1]//2] = 0

    mask_slab = ijkl_to_ss_fs(mask)

    import scipy.signal
    kernal = np.array([ [0,1,0], [1,1,1], [0,1,0] ], dtype=np.float)
    mask_pad = scipy.signal.convolve(1 - mask_slab.astype(np.float), kernal, mode = 'same') < 1
    return mask_pad

def edges(shape, pad = 0, det_dict=None):
    mask_edges = np.ones(shape, dtype=np.bool)

    # if det_dict is None then load a default geometry 
    if det_dict is None :
        if shape == cspad_geom_shape :
            print('loading default cspad geometry file for the panel edges')
            geom_fnam = os.path.split(os.path.abspath(__file__))[0]
            geom_fnam = os.path.join(geom_fnam, 'example/cspad-cxib2313-v9.geom')
            x_map, y_map, det_dict = gf.pixel_maps_from_geometry_file(geom_fnam, return_dict = True)
    
    # loop over panels
    if det_dict is not None :
        for p in det_dict.keys():
            i = [det_dict[p]['min_ss'], det_dict[p]['max_ss'] + 1, det_dict[p]['min_fs'], det_dict[p]['max_fs'] + 1]
            mask_edges[i[0]:i[1], i[2]: min(i[3],i[2]+pad+1)] = False
            mask_edges[i[0]:i[1], max(i[2],i[3]-pad-1): i[3]] = False
            mask_edges[i[0]: min(i[1],i[0]+pad+1), i[2]:i[3]] = False
            mask_edges[max(i[0],i[1]-pad-1): i[1], i[2]:i[3]] = False
     
    else :
        mask_edges[:, : pad+1] = False
        mask_edges[:, -pad-1:] = False
        mask_edges[: pad+1, :] = False
        mask_edges[-pad-1:, :] = False

    if shape == pilatus_geom_shape :
        # no padding yet
        print('making pilatus edges')
        mask_edges *= make_pilatus_edges()
    
    return mask_edges

def dilate(mask):
    from scipy.ndimage.morphology import binary_dilation
    mask = ~binary_dilation(~mask)
    return mask

class Application:
    def __init__(self, cspad, geom_fnam = None, mask = None):
        # check if the cspad is psana shaped
        if cspad.shape == (4, 8, 185, 388) :
            self.cspad = gf.ijkl_to_ss_fs(cspad)
            self.cspad_shape_flag = 'psana'
        elif cspad.shape == (4 * 8, 185, 388) :
            self.cspad = gf.ijkl_to_ss_fs(cspad.reshape((4,8,185, 388)))
            self.cspad_shape_flag = 'psana2'
        elif cspad.shape == (1480, 1552):
            self.cspad_shape_flag = 'slab'
            self.cspad = cspad
        elif cspad.shape == pilatus_geom_shape :
            self.cspad_shape_flag = 'pilatus'
            self.cspad = cspad
        else :
            # this is not in fact a cspad image
            self.cspad_shape_flag = 'other'
            self.cspad = cspad

        print(self.cspad_shape_flag)
            
        self.mask  = np.ones_like(self.cspad, dtype=np.bool)
        self.geom_fnam = geom_fnam

        if self.geom_fnam is not None :
            self.pixel_maps, self.cspad_shape = gf.get_ij_slab_shaped(self.geom_fnam)
            i, j = np.meshgrid(range(self.cspad.shape[0]), range(self.cspad.shape[1]), indexing='ij')
            self.ss_geom = gf.apply_geom(self.geom_fnam, i)
            self.fs_geom = gf.apply_geom(self.geom_fnam, j)
            self.cspad_geom = np.zeros(self.cspad_shape, dtype=self.cspad.dtype)
            self.mask_geom  = np.zeros(self.cspad_shape, dtype=np.bool)
            #
            self.background = np.where(np.fliplr(gf.apply_geom(self.geom_fnam, np.ones_like(self.mask)).astype(np.bool).T) == False)
            # 
            # get the xy coords as a slab
            # self.y_map, self.x_map = gf.make_yx_from_1480_1552(geom_fnam)
            self.x_map, self.y_map, self.det_dict = gf.pixel_maps_from_geometry_file(geom_fnam, return_dict = True)
        else :
            i, j = np.meshgrid(range(self.cspad.shape[0]), range(self.cspad.shape[1]), indexing='ij')
            self.y_map, self.x_map = (i-self.cspad.shape[0]//2, j-self.cspad.shape[1]//2)
            self.cspad_shape = self.cspad.shape
            self.det_dict    = None

        self.mask_edges    = False
        self.mask_unbonded = False

        self.unbonded_pixels = unbonded_pixels()
        self.asic_edges      = edges(self.mask.shape, 0, self.det_dict)
        if mask is not None :
            self.mask_clicked  = mask.copy()
        else :
            self.mask_clicked  = np.ones_like(self.mask)

        self.initUI()

    def updateDisplayRGB(self, auto = False):
        """
        Make an RGB image (N, M, 3) (pyqt will interprate this as RGB automatically)
        with masked pixels shown in blue at the maximum value of the cspad. 
        This ensures that the masked pixels are shown at full brightness.
        """
        if self.geom_fnam is not None :
            self.cspad_geom[self.pixel_maps[0], self.pixel_maps[1]] = self.cspad.ravel()
            self.mask_geom[self.pixel_maps[0], self.pixel_maps[1]]  = self.mask.ravel()
            trans      = np.fliplr(self.cspad_geom.T)
            trans_mask = np.fliplr(self.mask_geom.T) 
            #
            # I need to make the mask True between the asics...
            trans_mask[self.background] = True
        else :
            trans      = np.fliplr(self.cspad.T)
            trans_mask = np.fliplr(self.mask.T)
        self.cspad_max  = self.cspad.max()

        # convert to RGB
        # Set masked pixels to B
        display_data = np.zeros((trans.shape[0], trans.shape[1], 3), dtype = self.cspad.dtype)
        display_data[:, :, 0] = trans * trans_mask
        display_data[:, :, 1] = trans * trans_mask
        display_data[:, :, 2] = trans + (self.cspad_max - trans) * ~trans_mask
        
        self.display_RGB = display_data
        if auto :
            self.plot.setImage(self.display_RGB)
        else :
            self.plot.setImage(self.display_RGB, autoRange = False, autoLevels = False, autoHistogramRange = False)

    def generate_mask(self):
        self.mask = self.mask_clicked.copy()

    def mask_unbonded_pixels(self):
        print('masking unbonded pixels')
        if self.toggle_checkbox.isChecked():
            self.mask_clicked[~self.unbonded_pixels] = ~self.mask_clicked[~self.unbonded_pixels]
        elif self.mask_checkbox.isChecked():
            self.mask_clicked[~self.unbonded_pixels] = False
        elif self.unmask_checkbox.isChecked():
            self.mask_clicked[~self.unbonded_pixels] = True
        
        self.generate_mask()
        self.updateDisplayRGB()

    def mask_olek_edge_pixels(self):
        print('masking Oleksandr edges')
        edges = make_pilatus_sub_edges()
        if self.toggle_checkbox.isChecked():
            self.mask_clicked[~edges] = ~self.mask_clicked[~edges]
        elif self.mask_checkbox.isChecked():
            self.mask_clicked[~edges] = False
        elif self.unmask_checkbox.isChecked():
            self.mask_clicked[~edges] = True
        
        self.generate_mask()
        self.updateDisplayRGB()

    def mask_edge_pixels(self):
        print('masking panel edges')
        if self.toggle_checkbox.isChecked():
            self.mask_clicked[~self.asic_edges] = ~self.mask_clicked[~self.asic_edges]
        elif self.mask_checkbox.isChecked():
            self.mask_clicked[~self.asic_edges] = False
        elif self.unmask_checkbox.isChecked():
            self.mask_clicked[~self.asic_edges] = True
        
        self.generate_mask()
        self.updateDisplayRGB()

    def update_mask_edges(self, state):
        if state > 0 :
            print('adding asic edges to the mask')
            self.mask_edges = True
        else :
            print('removing asic edges from the mask')
            self.mask_edges = False
        
        self.generate_mask()
        self.updateDisplayRGB()

    def update_mask_unbonded(self, state):
        if state > 0 :
            print('adding unbonded pixels to the mask')
            self.mask_unbonded = True
        else :
            print('removing unbonded pixels from the mask')
            self.mask_unbonded = False
        
        self.generate_mask()
        self.updateDisplayRGB()

    def update_mask_edges(self, state):
        if state > 0 :
            print('adding asic edges to the mask')
            self.mask_edges = True
        else :
            print('removing asic edges from the mask')
            self.mask_edges = False
        
        self.generate_mask()
        self.updateDisplayRGB()

    def save_mask(self):
        print('updating mask...')
        self.generate_mask()

        if self.cspad_shape_flag == 'psana' :
            print('shifting back to original cspad shape:', self.cspad_shape_flag)
            mask = gf.ss_fs_to_ijkl(self.mask)
        elif self.cspad_shape_flag == 'psana2' : 
            print('shifting back to original cspad shape:', self.cspad_shape_flag)
            mask = gf.ss_fs_to_ijkl(self.mask)
            mask = mask.reshape((32, 185, 388))
        elif self.cspad_shape_flag == 'slab' :
            mask = self.mask
        elif self.cspad_shape_flag == 'other' :
            mask = self.mask
        else :
            mask = self.mask
        
        print('outputing mask as np.int16 (h5py does not support boolean arrays yet)...')
        f = h5py.File('mask.h5', 'w')
        f.create_dataset('/data/data', data = mask.astype(np.int16))
        f.close()
        print('Done!')
        
    def mask_ROI(self, roi):
        sides   = [roi.size()[1], roi.size()[0]]
        courner = [self.cspad_shape[0]/2. - roi.pos()[1], \
                   roi.pos()[0] - self.cspad_shape[1]/2.]

        top_left     = [np.rint(courner[0]) - 1, np.rint(courner[1])]
        bottom_right = [np.rint(courner[0] - sides[0]), np.rint(courner[1] + sides[1]) - 1]

        if self.geom_fnam is not None :
            # why?
            top_left[0]     += 2
            bottom_right[1] += 1
            bottom_right[0] += 1
        
        y_in_rect = (self.y_map <= top_left[0]) & (self.y_map >= bottom_right[0])
        x_in_rect = (self.x_map >= top_left[1]) & (self.x_map <= bottom_right[1])
        i2, j2 = np.where( y_in_rect & x_in_rect )
        self.apply_ROI(i2, j2)

    def mask_ROI_circle(self, roi):
        # get the xy coords of the centre and the radius
        rad    = roi.size()[0]/2. + 0.5
        centre = [self.cspad_shape[0]/2. - roi.pos()[1] - rad, \
                  roi.pos()[0] + rad - self.cspad_shape[1]/2.]
        
        r_map = np.sqrt((self.y_map-centre[0])**2 + (self.x_map-centre[1])**2)
        i2, j2 = np.where( r_map <= rad )
        self.apply_ROI(i2, j2)

    def apply_ROI(self, i2, j2):
        if self.toggle_checkbox.isChecked():
            self.mask_clicked[i2, j2] = ~self.mask_clicked[i2, j2]
        elif self.mask_checkbox.isChecked():
            self.mask_clicked[i2, j2] = False
        elif self.unmask_checkbox.isChecked():
            self.mask_clicked[i2, j2] = True
        
        self.generate_mask()
        self.updateDisplayRGB()
    
    def mask_hist(self):
        min_max = self.plot.getHistogramWidget().item.getLevels()
        
        if self.toggle_checkbox.isChecked():
            self.mask_clicked[np.where(self.cspad < min_max[0])] = ~self.mask_clicked[np.where(self.cspad < min_max[0])]
            self.mask_clicked[np.where(self.cspad > min_max[1])] = ~self.mask_clicked[np.where(self.cspad > min_max[1])]
        elif self.mask_checkbox.isChecked():
            self.mask_clicked[np.where(self.cspad < min_max[0])] = False
            self.mask_clicked[np.where(self.cspad > min_max[1])] = False
        elif self.unmask_checkbox.isChecked():
            self.mask_clicked[np.where(self.cspad < min_max[0])] = True
            self.mask_clicked[np.where(self.cspad > min_max[1])] = True
        
        self.generate_mask()
        self.updateDisplayRGB()

    def dilate_mask(self):
        """
        do this on a per-panel basis
        """
        
        # loop over panels
        if self.geom_fnam is not None :
            for p in self.det_dict.keys():
                i = [self.det_dict[p]['min_ss'], self.det_dict[p]['max_ss'] + 1, self.det_dict[p]['min_fs'], self.det_dict[p]['max_fs'] + 1]
                self.mask_clicked[i[0]:i[1], i[2]:i[3]] = dilate(self.mask_clicked[i[0]:i[1], i[2]:i[3]])
        else :
                self.mask_clicked = dilate(self.mask_clicked)
        
        self.generate_mask()
        self.updateDisplayRGB()

    def errode_mask(self, mask = None):
        # loop over panels
        if self.geom_fnam is not None :
            for p in self.det_dict.keys():
                i = [self.det_dict[p]['min_ss'], self.det_dict[p]['max_ss'] + 1, self.det_dict[p]['min_fs'], self.det_dict[p]['max_fs'] + 1]
                self.mask_clicked[i[0]:i[1], i[2]:i[3]] = ~dilate(~self.mask_clicked[i[0]:i[1], i[2]:i[3]])
        else :
                self.mask_clicked = ~dilate(~self.mask_clicked)
        
        
        self.generate_mask()
        self.updateDisplayRGB()

    def initUI(self):
        signal.signal(signal.SIGINT, signal.SIG_DFL) # allow Control-C
        
        # Always start by initializing Qt (only once per application)
        app = QtGui.QApplication([])

        # Define a top-level widget to hold everything
        w = QtGui.QWidget()

        # 2D plot for the cspad and mask
        self.plot = pg.ImageView()

        # save mask button
        save_button = QtGui.QPushButton('save mask')
        save_button.clicked.connect(self.save_mask)

        # rectangular ROI selection
        self.roi = pg.RectROI([-200,-200], [100, 100])
        self.plot.addItem(self.roi)
        self.roi.setZValue(10)                       # make sure ROI is drawn above image
        ROI_button = QtGui.QPushButton('mask rectangular ROI')
        ROI_button.clicked.connect(lambda : self.mask_ROI(self.roi))

        # circular ROI selection
        self.roi_circle = pg.CircleROI([-200,200], [101, 101])
        self.plot.addItem(self.roi_circle)
        self.roi.setZValue(10)                       # make sure ROI is drawn above image
        ROI_circle_button = QtGui.QPushButton('mask circular ROI')
        ROI_circle_button.clicked.connect(lambda : self.mask_ROI_circle(self.roi_circle))

        # histogram mask button
        hist_button = QtGui.QPushButton('mask outside histogram')
        hist_button.clicked.connect(self.mask_hist)

        # dilate button
        dilate_button = QtGui.QPushButton('dilate mask')
        dilate_button.clicked.connect(self.dilate_mask)

        # errode button
        errode_button = QtGui.QPushButton('errode mask')
        errode_button.clicked.connect(self.errode_mask)
        
        # toggle / mask / unmask checkboxes
        self.toggle_checkbox   = QtGui.QCheckBox('toggle')
        self.mask_checkbox     = QtGui.QCheckBox('mask')
        self.unmask_checkbox   = QtGui.QCheckBox('unmask')
        self.mask_checkbox.setChecked(True)   
        
        toggle_group           = QtGui.QButtonGroup()#"masking behaviour")
        toggle_group.addButton(self.toggle_checkbox)   
        toggle_group.addButton(self.mask_checkbox)   
        toggle_group.addButton(self.unmask_checkbox)   
        toggle_group.setExclusive(True)
        
        # unbonded pixels button
        unbonded_button = QtGui.QPushButton('unbonded pixels')
        unbonded_button.clicked.connect( self.mask_unbonded_pixels )
        if self.cspad_shape_flag in ['psana', 'psana2', 'slab']:
            unbonded_button.setEnabled(False)
        
        # asic edges button
        edges_button = QtGui.QPushButton('panel edges')
        edges_button.clicked.connect( self.mask_edge_pixels )

        # mouse hover ij value label
        ij_label = QtGui.QLabel()
        disp = 'ss fs {0:5} {1:5}   value {2:2}'.format('-', '-', '-')
        ij_label.setText(disp)
        self.plot.scene.sigMouseMoved.connect( lambda pos: self.mouseMoved(ij_label, pos) )
        
        # unbonded pixels checkbox
        unbonded_checkbox = QtGui.QCheckBox('unbonded pixels')
        unbonded_checkbox.stateChanged.connect( self.update_mask_unbonded )
        if self.cspad_shape_flag == 'other' :
            unbonded_checkbox.setEnabled(False)
        
        # asic edges checkbox
        edges_checkbox = QtGui.QCheckBox('asic edges')
        edges_checkbox.stateChanged.connect( self.update_mask_edges )
        if self.cspad_shape_flag == 'other' :
            edges_checkbox.setEnabled(False)
        
        # mouse click mask 
        self.plot.scene.sigMouseClicked.connect( lambda click: self.mouseClicked(self.plot, click) )
        
        ## Add widgets to the layout in their proper positions
        # stack up sidepanel widgets
        vbox = QtGui.QVBoxLayout()
        vbox.addWidget(save_button)
        vbox.addWidget(ROI_button)
        vbox.addWidget(ROI_circle_button)
        vbox.addWidget(hist_button)
        vbox.addWidget(dilate_button)
        vbox.addWidget(errode_button)
        vbox.addWidget(unbonded_button)
        vbox.addWidget(edges_button)
        
        # asic sub-panel edges button
        if self.cspad_shape_flag == 'pilatus':
            olek_edges_button = QtGui.QPushButton('more edges')
            olek_edges_button.clicked.connect( self.mask_olek_edge_pixels )
            vbox.addWidget(olek_edges_button)
        
        # cheetah mask button
        if self.geom_fnam is not None :
            cheetah_mask_button = QtGui.QPushButton('cheetah threshold')
            cheetah_mask_button.clicked.connect(self.make_cheetah_mask)
            vbox.addWidget(cheetah_mask_button)
        
        vbox.addWidget(self.toggle_checkbox)
        vbox.addWidget(self.mask_checkbox)
        vbox.addWidget(self.unmask_checkbox)
        
        vbox.addStretch(1)
        #vbox.addWidget(unbonded_checkbox)
        #vbox.addWidget(edges_checkbox)
        
        # Create a grid layout to manage the widgets size and position
        layout = QtGui.QGridLayout()
        w.setLayout(layout)
        
        layout.addLayout(vbox, 0, 0)
        layout.addWidget(self.plot, 0, 1)
        layout.addWidget(ij_label, 1, 0, 1, 2)
        layout.setColumnStretch(1, 1)
        #layout.setColumnMinimumWidth(0, 250)

        # display the image
        self.generate_mask()
        self.updateDisplayRGB(auto = True)
        
        # centre the circle initially 
        if self.geom_fnam is not None :
            self.roi_circle.setPos([self.cspad_shape[0]//2 - 1 - 50, self.cspad_shape[1]//2 - 1 - 50])
        
        ## Display the widget as a new window
        w.resize(800, 480)
        w.show()
        
        ## Start the Qt event loop
        app.exec_()
    
    def make_cheetah_mask(self):
        mask = cheetah_mask(self.cspad.astype(np.float), self.mask_clicked.copy(), self.x_map, self.y_map, adc_thresh=20, min_snr=6, counter = 5)
        
        print('masking cheetah pixels')
        if self.toggle_checkbox.isChecked():
            print(mask.dtype, np.sum(~mask))
            self.mask_clicked[~mask] = ~self.mask_clicked[~mask]
        elif self.mask_checkbox.isChecked():
            self.mask_clicked[~mask] = False
        elif self.unmask_checkbox.isChecked():
            self.mask_clicked[~mask] = True
        
        self.generate_mask()
        self.updateDisplayRGB()

    def mouseMoved(self, ij_label, pos):
        img = self.plot.getImageItem()
        if self.geom_fnam is not None :
            ij = [self.cspad_shape[0] - 1 - int(img.mapFromScene(pos).y()), int(img.mapFromScene(pos).x())] # ss, fs
            if (0 <= ij[0] < self.cspad_shape[0]) and (0 <= ij[1] < self.cspad_shape[1]):
#                ij_label.setText('ss fs value: ' + str(self.ss_geom[ij[0], ij[1]]).rjust(5) + str(self.fs_geom[ij[0], ij[1]]).rjust(5) + str(self.cspad_geom[ij[0], ij[1]]).rjust(8) )
                ij_label.setText('ss fs value: %d %d %.2e' % (self.ss_geom[ij[0], ij[1]], self.fs_geom[ij[0], ij[1]], self.cspad_geom[ij[0], ij[1]]) )
        else :
            ij = [self.cspad.shape[0] - 1 - int(img.mapFromScene(pos).y()), int(img.mapFromScene(pos).x())] # ss, fs
            if (0 <= ij[0] < self.cspad.shape[0]) and (0 <= ij[1] < self.cspad.shape[1]):
                ij_label.setText('ss fs value: %d %d %.2e' % (ij[0], ij[1], self.cspad[ij[0], ij[1]]) )
#                ij_label.setText('ss fs value: ' + str(ij[0]).rjust(5) + str(ij[1]).rjust(5) + str(self.cspad[ij[0], ij[1]]).rjust(8) )

    def mouseClicked(self, plot, click):
        if click.button() == 1:
            img = plot.getImageItem()
            if self.geom_fnam is not None :
                i0 = int(img.mapFromScene(click.pos()).y())
                j0 = int(img.mapFromScene(click.pos()).x())
                i1 = self.cspad_shape[0] - 1 - i0 # array ss (with the fliplr and .T)
                j1 = j0                           # array fs (with the fliplr and .T)
                if (0 <= i1 < self.cspad_shape[0]) and (0 <= j1 < self.cspad_shape[1]):
                    i = self.ss_geom[i1, j1]  # un-geometry corrected ss
                    j = self.fs_geom[i1, j1]  # un-geometry corrected fs
                    if i == 0 and j == 0 and i1 != 0 and j1 != 0 :
                        return 
                    else :
                        if self.toggle_checkbox.isChecked():
                            self.mask_clicked[i, j] = ~self.mask_clicked[i, j]
                            self.mask[i, j]         = ~self.mask[i, j]
                        elif self.mask_checkbox.isChecked():
                            self.mask_clicked[i, j] = False
                            self.mask[i, j]         = False
                        elif self.unmask_checkbox.isChecked():
                            self.mask_clicked[i, j] = True
                            self.mask[i, j]         = True
                        
                        if self.mask[i, j] :
                            self.display_RGB[j0, i0, :] = np.array([1,1,1]) * self.cspad[i, j]
                        else :
                            self.display_RGB[j0, i0, :] = np.array([0,0,1]) * self.cspad_max
            else :
                i0 = int(img.mapFromScene(click.pos()).y())
                j0 = int(img.mapFromScene(click.pos()).x())
                i1 = self.cspad.shape[0] - 1 - i0 # array ss (with the fliplr and .T)
                j1 = j0                           # array fs (with the fliplr and .T)
                if (0 <= i1 < self.cspad.shape[0]) and (0 <= j1 < self.cspad.shape[1]):
                    if self.toggle_checkbox.isChecked():
                        self.mask_clicked[i1, j1] = ~self.mask_clicked[i1, j1]
                        self.mask[i1, j1]         = ~self.mask[i1, j1]
                    elif self.mask_checkbox.isChecked():
                        self.mask_clicked[i1, j1] = False
                        self.mask[i1, j1]         = False
                    elif self.unmask_checkbox.isChecked():
                        self.mask_clicked[i1, j1] = True
                        self.mask[i1, j1]         = True
                    if self.mask[i1, j1] :
                        self.display_RGB[j0, i0, :] = np.array([1,1,1]) * self.cspad[i1, j1]
                    else :
                        self.display_RGB[j0, i0, :] = np.array([0,0,1]) * self.cspad_max
            
            self.plot.setImage(self.display_RGB, autoRange = False, autoLevels = False, autoHistogramRange = False)

def parse_cmdline_args():
    parser = argparse.ArgumentParser(description='CsPadMaskMaker - mask making, but with a mouse!')
    parser.add_argument('cspad_fnam', type=str, help="filename for the hdf5 cspad image file")
    parser.add_argument('h5path', type=str, help="hdf5 path for the 2D cspad data")
    parser.add_argument('-g', '--geometry', type=str, help="path to the CrystFEL geometry file for the image")
    parser.add_argument('-m', '--mask', type=str, help="path to the h5file of the starting mask")
    parser.add_argument('-mp', '--mask_h5path', type=str, help="path inside the h5file of the starting mask")
    return parser.parse_args()
    
if __name__ == '__main__':
    args = parse_cmdline_args()

    # load the image
    f = h5py.File(args.cspad_fnam, 'r')
    cspad = f[args.h5path].value
    # remove single dimensional entries
    cspad = np.squeeze(cspad)
    f.close()
    
    # load the predefined mask
    if args.mask is not None :
        if args.mask_h5path is not None :
            path = args.mask_h5path
        else :
            path = '/data/data'
        f = h5py.File(args.mask, 'r')
        mask = f[path].value.astype(np.bool)
        f.close()
    else :
        mask = None

    # start the gui
    Application(cspad, geom_fnam = args.geometry, mask = mask)
    """
    ap = Application(cspad, geom_fnam = args.geometry, mask = mask)
    """
    


