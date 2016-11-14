#
# Author: Dominik Michels
# Date: August 2016
#

class StreamfileParserFlags:
    """
    This class provides flags used in the parsing process of the streamfile.

    Note:
        All the class variables are static and therefore the class should not be
        instantiated.  
    """

    none = 0
    geometry = 1
    chunk = 2
    peak = 3
    predicted_peak = 4
    flag_changed = 5
