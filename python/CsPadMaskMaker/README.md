# CsPadMaskMaker
graphical interface for making a pixel mask of the cspad

```
$ python maskMakerGUI.py -h
usage: maskMakerGUI.py [-h] [-g GEOMETRY] [-m MASK] [-mp MASK_H5PATH]
                       cspad_fnam h5path

CsPadMaskMaker - mask making, but with a mouse!

positional arguments:
  cspad_fnam            filename for the hdf5 cspad image file
  h5path                hdf5 path for the 2D cspad data

optional arguments:
  -h, --help            show this help message and exit
  -g GEOMETRY, --geometry GEOMETRY
                        path to the CrystFEL geometry file for the image
  -m MASK, --mask MASK  path to the h5file of the starting mask
  -mp MASK_H5PATH, --mask_h5path MASK_H5PATH
                        path inside the h5file of the starting mask
```

### Requires
- python (probably >= 2.7)
- h5py 
- pyqtgraph
- scipy
- numpy


### Example
```
$ git clone https://github.com/andyofmelbourne/CsPadMaskMaker.git
$ cd CsPadMaskMaker
```
CsPad:
```
$ ./maskMakerGUI.py example/LA93-r0014-CxiDs1-darkcal.h5 data/data -g example/cspad-cxib2313-v9.geom
```

### Alternative
SLAC has an alternative mask making GUI (which I have not tried yet) but it could be a better option if you have access https://confluence.slac.stanford.edu/display/PSDM/Mask+Editor.
