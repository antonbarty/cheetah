#
# pyCXIview
#
# Based on peak_viewer_cxi by Valerio Mariani, CFEL, December 2015
# A replacement for the IDL Cheetah file viewer
#

import sys
import h5py
import numpy
import scipy.constants
import PyQt4.QtCore
import PyQt4.QtGui
import pyqtgraph

from lib.cfel_geometry import *
import UI.CXIview_ui



#
#	CXI viewer code
#
class CXIview(PyQt4.QtGui.QMainWindow):
    
    #
    # display the main image
    #
    def draw_things(self):
        
        # Retrieve data and calibration values
        img = self.hdf5_fh['/entry_1/data_1/data'][self.img_index, :, :]
        #time_string = self.hdf5_fh['/LCLS/eventTimeString'][self.img_index]
        #self.ui.timeStampLabel.setText(time_string)
        title = str(self.img_index)+'/'+ str(self.num_lines) + ' - ' + self.filename
        photon_energy = self.hdf5_fh['/LCLS/photon_energy_eV'][self.img_index]
        self.lambd = scipy.constants.h * scipy.constants.c /(scipy.constants.e * photon_energy)
        self.camera_length = 1e-3*self.hdf5_fh['/LCLS/detector_1/EncoderValue'][self.img_index] 
        
        
        self.img_to_draw[self.pixel_maps[0], self.pixel_maps[1]] = img.ravel()
        self.ui.imageView.setImage(self.img_to_draw, autoLevels=False, autoRange=False)
        #self.ui.imageView.setImage(self.img_to_draw, autoLevels=False, autoRange=False, levels=[0,1000])
        self.ui.imageView.setLevels(0,1000) #, update=True)

		# Draw peaks if needed
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

		# Set title
        self.setWindowTitle(title)
    #end draw_things()
    

	#
	# Go to the previous pattern 
	#
    def previous_pattern(self):

        if self.img_index != 0:
            self.img_index -= 1
        else:
            self.img_index = self.num_lines-1
        self.draw_things()
    #end previous_pattern()


	#
	# Go to the next pattern 
	#
    def next_pattern(self):

        if self.img_index != self.num_lines-1:
            self.img_index += 1
        else:
            self.img_index = 0
        self.draw_things()
    #end next_pattern()
    

    #
	# Go to random pattern
	#
    def random_pattern(self):
        pattern_to_jump = self.num_lines*numpy.random.random(1)[0]
        pattern_to_jump = pattern_to_jump.astype(numpy.int64)
        
        if 0<pattern_to_jump<self.num_lines:
            self.img_index = pattern_to_jump
            self.draw_things()
        else:
            self.ui.jumpToLineEdit.setText(str(self.img_index))
    #end random_pattern()
    

    #
	# Shuffle (play random patterns)
	#
    #def shuffle(self):
        # Fill in later
    #end shuffle()


    #
	# Play (display patterns in order)
	#
    #def play(self):
        # Fill in later
    #end play()


	#
	# Go to particular pattern
	#
    def jump_to_pattern(self):
        pattern_to_jump = int(self.ui.jumpToLineEdit.text())
        
        if 0<pattern_to_jump<self.num_lines:
            self.img_index = pattern_to_jump
            self.draw_things()
        else:
            self.ui.jumpToLineEdit.setText(str(self.img_index))
        #end jump_to_pattern()
        

	#
	# Toggle show or hide peaks
	#
    def showhidepeaks(self, state):
        if state == PyQt4.QtCore.Qt.Checked:
            self.show_peaks = True
        else:
            self.show_peaks = False
        self.draw_things()
	#end showhidepeaks()


	#
	# Mouse clicked somewhere in the window
	#
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
    #end mouse_clicked()
    
    
    #
    #	Initialisation function
    #
    def __init__(self, geom_filename, img_filename, fh):

        super(CXIview, self).__init__()
        pyqtgraph.setConfigOption('background', 0.2)
        self.ui = UI.CXIview_ui.Ui_MainWindow()
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
        self.ui.randomPushButton.clicked.connect(self.random_pattern)
        #self.ui.playPushButton.clicked.connect(self.play)
        #self.ui.shufflePushButton.clicked.connect(self.shuffle)
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
    #end __init()__
#end CXIview

        
#
#	Main function defining this as a program to be called
#
if __name__ == '__main__':
    
    app = PyQt4.QtGui.QApplication(sys.argv)
    
    if len(sys.argv) != 3:
        print('Usage: pyCXIview.py geom_file cxi_file')
        sys.exit()
    #endif 
        
    hdf5_fh = h5py.File(sys.argv[2], 'r')  
    ex = CXIview(sys.argv[1], sys.argv[2], hdf5_fh)
    ex.show()
    
    ret = app.exec_()
    hdf5_fh.close()    
    sys.exit(ret)
#end __main__