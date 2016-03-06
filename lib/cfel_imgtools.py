# -*- coding: utf-8 -*-
#
#	CFEL image handling tools
#	Anton Barty
#

import numpy



def histogram_clip_levels(data, value):
    """
    Return the top and bottom <value> percentile of data points
    Useful for scaling images to reject outliers
    <value> is typically 0.001 or 0.01 (equivalent to clipping top and bottom 0.1% and 1% of pixels respectively)
    """
    
    # Cumulative histogram
    h, e = numpy.histogram(data, bins=max(200, numpy.int_(data.max())), range=(0,data.max()))
    c = h.cumsum()/h.sum()

    # Top and bottom <value> percentile
    w_top = numpy.where(c <= (1-value))
    w_bottom = numpy.where(c >= value)
    top = numpy.amax(w_top)
    bottom = numpy.amin(w_bottom)
    
    # Return suggested levels for image scaling
    #print("histogram_clip: ", bottom, top)    
    return bottom, top



    


