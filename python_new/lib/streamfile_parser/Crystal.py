#
# Author: Dominik Michels
# Date: August 2016
#

from .UnitCell import *

class Crystal:
    """
    This class is a data structure to store the information about a crystal in
    the crystfel stream file.
    """
    
    def __init__(self):
        self.begin_predicted_peaks_pointer = None 
        self.end_predicted_peaks_pointer = None
        self.unit_cell = None
        self.resolution_limit = None


    def dump(self):
        """
        This method prints the parameters of the crystal
        """

        print("Crystal:")
        print("Begin predicted peaks pointer: ", 
            self.begin_predicted_peaks_pointer)
        print("End predicted peaks pointer: ", 
            self.end_predicted_peaks_pointer)
        self.unit_cell.dump()
        print("Resolution limit: ", self.resolution_limit)
