#!//reg/g/cfel/anaconda/bin/python3
#
# CXIview
#
# Python/Qt viewer for .cxi files (and other files output by Cheetah)
# Based on peak_viewer_cxi by Valerio Mariani, CFEL, December 2015
# A replacement for the IDL Cheetah file viewer
#
#!/Applications/anaconda/bin/python3


import sys
import os
import h5py
import numpy
import scipy.constants
import PyQt4.QtCore
import PyQt4.QtGui
import pyqtgraph
import argparse


import UI.cxiview_ui
import lib.cfel_colours
from lib.cfel_geometry import *



#
#	CXI viewer code
#
class cxiview(PyQt4.QtGui.QMainWindow):
    
    #
    # display the main image
    #
    def draw_things(self):
        
        # Retrieve data and calibration values
        img = self.hdf5_fh['/entry_1/data_1/data'][self.img_index, :, :]
        photon_energy = self.hdf5_fh['/LCLS/photon_energy_eV'][self.img_index]
        self.lambd = scipy.constants.h * scipy.constants.c /(scipy.constants.e * photon_energy)
        self.camera_length = 1e-3*self.hdf5_fh['/LCLS/detector_1/EncoderValue'][self.img_index] 
        #time_string = self.hdf5_fh['/LCLS/eventTimeString'][self.img_index]
        
        title = str(self.img_index)+'/'+ str(self.num_lines) + ' - ' + self.filename
        #self.ui.timeStampLabel.setText(time_string)

        
        # Set the image
        # http://www.pyqtgraph.org/documentation/graphicsItems/imageitem.html
        self.img_to_draw[self.pixel_maps[0], self.pixel_maps[1]] = img.ravel()
        self.ui.imageView.setImage(self.img_to_draw, autoLevels=False, autoRange=False)
        #self.ui.imageView.setImage(self.img_to_draw, autoLevels=False, autoRange=False, levels=[0,1000])

        # Histogram equalisation        
        h, e = numpy.histogram(img.ravel(), bins=max(200, numpy.int_(img.max())), range=(0,img.max()))
        c = h.cumsum()/h.sum()
        w = numpy.where(c <= 0.999)
        top = numpy.amax(w)
        #print('Colour scale upper bound = ', top)
        self.ui.imageView.setLevels(0,top) #, update=True)
        
        # Static level
        #self.ui.imageView.setLevels(0,1000) #, update=True)

        # Set the histogram widget to not auto-scale so badly
        # http://www.pyqtgraph.org/documentation/graphicsItems/histogramlutitem.html#pyqtgraph.HistogramLUTItem
        hist = self.ui.imageView.getHistogramWidget()
        hist.setHistogramRange(-100, 10000, padding=0.1)
        #self.ui.imageView.ui.histogram.setHistogramRange(-100, 10000, padding=0.1)

        # Edit the colour table (unduly complicated - edit the editor rather than load a table of values)
        # http://www.pyqtgraph.org/documentation/graphicsItems/gradienteditoritem.html#pyqtgraph.GradientEditorItem.__init__
        #self.ui.imageView.ui.GradientEditorItem.addTick(0.5,'#207020')
        #grad = self.ui.imageView.getGradientWidget()

        # Find gradient editor item reference
        #grad =  self.ui.imageView.ui.histogram.gradient
        
         #self.ui.imageview.ui.histogram.gradient.setColorMap(colormap)
        #col = grad.colorMap()
        #grad.addTick(0.5,color=(1.0,0.0,0.0,1.0))
        #grad.addTick(0.5,color='r')
        #grad.item.addTick(1,color='r')

        
        #map = histogram
        #map = self.ui.imageView.ui.gradient
        #map = self.ui.imageView.ui.TickSliderItem().listTicks();

        # Suggestion from https://groups.google.com/forum/#!topic/pyqtgraph/gEjC08Vb8NQ
        #pos = numpy.array([0.0, 0.5, 1.0])
        #color = numpy.array([[0,0,0,255], [255,128,0,255], [255,255,0,255]], dtype=numpy.ubyte)
        #map = pyqtgraph.ColorMap(pos, color)
        #lut = map.getLookupTable(0.0, 1.0, 256)
        #self.ui.imageView.ui.setLookupTable(lut)


        # Modifying the colour table - from Valerio - and it works
        pos = numpy.array([0.0,0.5,1.0])
        color = numpy.array([[255,255,255,255], [128,128,128,255], [0,0,0,255]], dtype=numpy.ubyte)
        self.new_color_map = pyqtgraph.ColorMap(pos,color)
        self.ui.imageView.ui.histogram.gradient.setColorMap(self.new_color_map)
       

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

            self.peak_canvas.setData(peak_x, peak_y, symbol = 'o', size = 10, pen = self.ring_pen, brush = (0,0,0,0), pxMode = False)

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
    def shuffle(self):
        print("Not yet implemented")
    #end shuffle()


    #
	# Play (display patterns in order)
	#
    def play(self):
        print("Not yet implemented")
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
            
            self.ui.statusBar.setText('Last clicked pixel:     x: %4i     y: %4i     value: %4i     resolution: %4.2f' % (x_mouse_centered, y_mouse_centered, self.img_to_draw[x_mouse,y_mouse], resolution))
    #end mouse_clicked()
    
    
    #
    #	Initialisation function
    #
    def __init__(self, geom_filename, img_filename, fh):

        super(cxiview, self).__init__()
        pyqtgraph.setConfigOption('background', 0.2)
        self.ui = UI.cxiview_ui.Ui_MainWindow()
        self.ui. setupUi(self)
        self.ui.imageView.ui.menuBtn.hide()
        self.ui.imageView.ui.roiBtn.hide()

        self.peak_canvas = pyqtgraph.ScatterPlotItem()
        self.ui.imageView.getView().addItem(self.peak_canvas)
        #self.ui.imageView.getView().setRange(xRange=(0,1500))
        #self.ui.imageView.getView().scaleBy(0.1)

        self.intregex = PyQt4.QtCore.QRegExp('[0-9]+')
        self.qtintvalidator = PyQt4.QtGui.QRegExpValidator()
        self.qtintvalidator.setRegExp(self.intregex)        

        self.ui.previousPushButton.clicked.connect(self.previous_pattern)
        self.ui.nextPushButton.clicked.connect(self.next_pattern)
        self.ui.randomPushButton.clicked.connect(self.random_pattern)
        self.ui.playPushButton.clicked.connect(self.play)
        self.ui.shufflePushButton.clicked.connect(self.shuffle)
        self.ui.jumpToLineEdit.editingFinished.connect(self.jump_to_pattern)
        self.ui.jumpToLineEdit.setValidator(self.qtintvalidator)

        self.show_peaks = False
        self.ui.peaksCheckBox.setChecked(False)
        self.ui.peaksCheckBox.stateChanged.connect(self.showhidepeaks)

        
        self.proxy = pyqtgraph.SignalProxy(self.ui.imageView.getView().scene().sigMouseClicked, rateLimit=60, slot=self.mouse_clicked)

        self.hdf5_fh = fh
        self.filename = img_filename
        self.num_lines = self.hdf5_fh['/entry_1/data_1/data'].shape[0]
        self.slab_shape = (self.hdf5_fh['/entry_1/data_1/data'].shape[1],self.hdf5_fh['/entry_1/data_1/data'].shape[2])

        self.pixel_maps, self.img_shape = read_geometry(geom_filename, format='pixelmap')
        self.pixel_maps = numpy.int_(self.pixel_maps)
        self.coffset, self.res = read_geometry_coffset_and_res(geom_filename, format='pixelmap')
        
        self.img_to_draw = numpy.zeros(self.img_shape, dtype=numpy.float32)

        self.ring_pen = pyqtgraph.mkPen('r', width=2)
        self.circle_pen = pyqtgraph.mkPen('b', width=2)
        
        self.img_index = 0
        self.ui.jumpToLineEdit.setText(str(self.img_index))

        
        
        # Try to set the colour table to inverse-BW

        # Scale QtGraph histogram to not auto-range
        #qt_Histogram = self.ui.imageView.ui.HistogramLUTItem()
        #qt_Histogram.autoHistogramRange()
        #qt_Histogram.setHistogramRange(-100, 10000, padding=0.1)
        #self.ui.imageView.ui.HistogramLUTItem().setHistogramRange(-100, 10000, padding=0.1)
        self.ui.imageView.ui.histogram.setHistogramRange(-100, 10000, padding=0.1)
                
        self.draw_things()
        self.ui.statusBar.setText('Ready') 
        
        
        
    #end __init()__
#end cxiview

        
#
#	Main function defining this as a program to be called
#
if __name__ == '__main__':
    
    #
    #   Parse command line arguments
    #    
    parser = argparse.ArgumentParser(description='CFEL CXI file viewer')
    parser.add_argument("-g", help="Geometry file (.geom/.h5)")
    parser.add_argument("-i", help="Input file or directory (.cxi/.h5)")
    parser.add_argument("-p", help="Circle peaks by default")
    #parser.add_argument("-d", default="none", help="Directory to scan")
    #parser.add_argument("--rmin", type=float, help="minimum pixel resolution cutoff")
    #parser.add_argument("--nmax", default=np.inf, type=int, help="maximum number of peaks to read")
    args = parser.parse_args()
    
    
    # This bit may be irrelevent if we can make parser.parse_args() require this field    
    if args.i is None:
        print('Usage: CXIview.py -i cxi_file -g geom_file ')
        sys.exit()
    #endif        
    #if len(sys.argv) != 3:
    #    print('Usage: pyCXIview.py geom_file cxi_file')
    #    sys.exit()
    #endif 


    #
    #   Spawn the viewer
    #        
    app = PyQt4.QtGui.QApplication(sys.argv)    
        
    hdf5_fh = h5py.File(args.i, 'r')  
    ex = cxiview(args.g, args.i, hdf5_fh)
    ex.show()    
    ret = app.exec_()

    
    #
    # Cleanup on exit    
    #
    hdf5_fh.close()    
    app.exit()
    
    # This function does the following in an attempt to ‘safely’ terminate the process:
    #   Invoke atexit callbacks
    #   Close all open file handles
    os._exit(ret)
    sys.exit(ret)
#end __main__