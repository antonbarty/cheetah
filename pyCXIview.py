#
# CXIview
# A replacement for the IDL Cheetah file viewer
# Based on peak_viewer_cxi by Valerio Mariani, CFEL, December 2015
#
import sys
import h5py
import numpy
import scipy.constants
import PyQt4.QtCore
import PyQt4.QtGui
import pyqtgraph

import UI.pyCXIview


def pixel_maps_from_geometry_file(fnam):
    """
    Return pixel and radius maps from the geometry file
    
    Input: geometry filename
    
    Output: x: slab-like pixel map with x coordinate of each slab pixel in the reference system of the detector
            y: slab-like pixel map with y coordinate of each slab pixel in the reference system of the detector
            z: slab-like pixel map with distance of each pixel from the center of the reference system.
        
    """
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
            
    r = numpy.sqrt(numpy.square(x) + numpy.square(y))

	# AB
    #numpy.rint(x)
    #numpy.rint(y)

    return x, y, r


def pixel_maps_for_img(geometry_filename):
    x, y, r  = pixel_maps_from_geometry_file(geometry_filename)

    # find the smallest size of cspad_geom that contains all
    # xy values but is symmetric about the origin
    N = 2 * int(max(abs(y.max()), abs(y.min()))) + 2
    M = 2 * int(max(abs(x.max()), abs(x.min()))) + 2

    # convert y x values to i j values
    i = numpy.array(y, dtype=numpy.int) + N/2 - 1
    j = numpy.array(x, dtype=numpy.int) + M/2 - 1

    ij = (i.flatten(), j.flatten())
    img_shape = (N, M)
    return ij, img_shape    


def extract_coffset_and_res(geometry_filename):
    f = open(geometry_filename, 'r')
    f_lines = []
    for line in f:
        f_lines.append(line)

    coffset_lines = [ x for x in f_lines if 'coffset' in x]
    coffset = float(coffset_lines[-1].split('=')[1])
    res_lines = [ x for x in f_lines if 'res' in x]
    res = float(res_lines[-1].split('=')[1])

    return coffset, res


class CXIview(PyQt4.QtGui.QMainWindow):
    
    def draw_things(self):
        
        img = self.hdf5_fh['/entry_1/data_1/data'][self.img_index, :, :]
        #time_string = self.hdf5_fh['/LCLS/eventTimeString'][self.img_index]
        #self.ui.timeStampLabel.setText(time_string)
        title = str(self.img_index)+'/'+ str(self.num_lines) + ' - ' + self.filename
        photon_energy = self.hdf5_fh['/LCLS/photon_energy_eV'][self.img_index]
        self.lambd = scipy.constants.h * scipy.constants.c /(scipy.constants.e * photon_energy)
        self.camera_length = 1e-3*self.hdf5_fh['/LCLS/detector_1/EncoderValue'][self.img_index] 
        
        self.img_to_draw[self.pixel_maps[0], self.pixel_maps[1]] = img.ravel()
        self.ui.imageView.setImage(self.img_to_draw, autoLevels=False, autoRange=False)

        if self.show_peaks == True:

            peak_x = []
            peak_y = []

            n_peaks = self.hdf5_fh['/entry_1/result_1/nPeaks'][self.img_index]            
            peak_x_data = self.hdf5_fh['/entry_1/result_1/peakXPosRaw'][self.img_index]
            peak_y_data = self.hdf5_fh['/entry_1/result_1/peakYPosRaw'][self.img_index]
            
            for ind in range(0,n_peaks):
                
                peak_fs = peak_x_data[ind]                
                peak_ss = peak_y_data[ind]         
               
                peak_in_slab = int(round(peak_ss))*self.slab_shape[1]+int(round(peak_fs))
                peak_x.append(self.pixel_maps[0][peak_in_slab])
                peak_y.append(self.pixel_maps[1][peak_in_slab])

            self.peak_canvas.setData(peak_x, peak_y, symbol = 'o', size = 15, pen = self.ring_pen, brush = (0,0,0,0), pxMode = False)

        else:

            self.peak_canvas.setData([])

        self.setWindowTitle(title)


    def previous_pattern(self):

        if self.img_index == 0:
            self.img_index = self.num_lines-1
        else:
            self.img_index -= 1
        self.draw_things()


    def next_pattern(self):

        if self.img_index == self.num_lines-1:
            self.img_index = 0
        else:
            self.img_index += 1
        self.draw_things()


    def jump_to_pattern(self):
        pattern_to_jump = int(self.ui.jumpToLineEdit.text())
        
        if 0<pattern_to_jump<self.num_lines:
            self.img_index = pattern_to_jump
            self.draw_things()
        else:
            self.ui.jumpToLineEdit.setText(str(self.img_index))


    def showhidepeaks(self, state):

        if state == PyQt4.QtCore.Qt.Checked:
            self.show_peaks = True
        else:
            self.show_peaks = False
        self.draw_things()


    def mouse_clicked(self, event):
        pos = event[0].pos()
        if self.ui.imageView.getView().sceneBoundingRect().contains(pos):
            mouse_point = self.ui.imageView.getView().mapSceneToView(pos)
            x_mouse = int(mouse_point.x())
            y_mouse = int(mouse_point.y()) 
            x_mouse_centered = x_mouse - self.img_shape[0]/2 + 1
            y_mouse_centered = y_mouse - self.img_shape[0]/2 + 1
            radius = numpy.sqrt(x_mouse_centered**2 + y_mouse_centered**2) / self.res
                
            resolution = 10e9*self.lambd/(2.0*numpy.sin(0.5*numpy.arctan(radius/(self.camera_length+self.coffset))))            
            
            self.ui.pixelLabel.setText('Last clicked pixel:     x: %4i     y: %4i     value: %4i     resolution: %4.2f' % (x_mouse_centered, y_mouse_centered, self.img_to_draw[x_mouse,y_mouse], resolution))
    
    
    def __init__(self, geom_filename, img_filename, fh):

        super(CXIview, self).__init__()
        pyqtgraph.setConfigOption('background', 0.2)
        self.ui = UI.pyCXIview.Ui_MainWindow()
        self.ui. setupUi(self)
        self.ui.imageView.ui.menuBtn.hide()
        self.ui.imageView.ui.roiBtn.hide()

        self.peak_canvas = pyqtgraph.ScatterPlotItem()
        self.ui.imageView.getView().addItem(self.peak_canvas)

        self.intregex = PyQt4.QtCore.QRegExp('[0-9]+')
        self.qtintvalidator = PyQt4.QtGui.QRegExpValidator()
        self.qtintvalidator.setRegExp(self.intregex)        

        self.ui.previousPushButton.clicked.connect(self.previous_pattern)
        self.ui.nextPushButton.clicked.connect(self.next_pattern)
        self.ui.peaksCheckBox.setChecked(True)
        self.ui.peaksCheckBox.stateChanged.connect(self.showhidepeaks)
        self.ui.jumpToLineEdit.editingFinished.connect(self.jump_to_pattern)
        self.ui.jumpToLineEdit.setValidator(self.qtintvalidator)
        
        self.proxy = pyqtgraph.SignalProxy(self.ui.imageView.getView().scene().sigMouseClicked, rateLimit=60, slot=self.mouse_clicked)

        self.hdf5_fh = fh
        self.filename = img_filename
        self.num_lines = self.hdf5_fh['/entry_1/data_1/data'].shape[0]
        self.slab_shape = (self.hdf5_fh['/entry_1/data_1/data'].shape[1],self.hdf5_fh['/entry_1/data_1/data'].shape[2])

        self.pixel_maps, self.img_shape = pixel_maps_for_img(geom_filename)

        #AB 
        self.pixel_maps = numpy.int_(self.pixel_maps)
        self.coffset, self.res = extract_coffset_and_res(geom_filename)
        self.img_to_draw = numpy.zeros(self.img_shape, dtype=numpy.float32)

        self.ring_pen = pyqtgraph.mkPen('r', width=2)
        self.circle_pen = pyqtgraph.mkPen('b', width=2)
        
        self.img_index = 0
        self.ui.jumpToLineEdit.setText(str(self.img_index))
        self.show_peaks = True

        self.draw_things()
        

if __name__ == '__main__':
    
    app = PyQt4.QtGui.QApplication(sys.argv)
    if len(sys.argv) != 3:
        print('Usage: pyCXIview.py geom_file cxi_file')
        sys.exit()
    hdf5_fh = h5py.File(sys.argv[2], 'r')  
    ex = CXIview(sys.argv[1], sys.argv[2], hdf5_fh)
    ex.show()
    ret = app.exec_()
    hdf5_fh.close()    
    sys.exit(ret)