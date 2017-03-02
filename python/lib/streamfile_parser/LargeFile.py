#
# Author: Dominik Michels
# Date: August 2016
#

class LargeFile:
    """
    This class provides the ability to handle large text files relatively
    quickly. This is achieved by storing a table containing the newline
    positions in the file. Hence it is able to quickly jump to a given line
    number without scanning the whole file again. 
    
    Note: 
        The table storing the newline positions is currently deactivated to save
        memory.  

        Most of the methods reimplement the methods of a python file and will
        therefore not be further described.
    """


    def __init__(self, name, mode="r"):
        """
        Constructor of the class

        Args:
            name: Filepath to the file on the harddrive
            mode: Mode in which the file should be opened, e.g. "r","w","rw"
        """

        self.name = name
        self.file = open(name, mode)
        self.length = 0
        """
        self.length = sum(1 for line in self.file)
        self.file.seek(0)
        self._line_offset = numpy.zeros(self.length)
        self._read_file()
        """


    def __exit__(self):
        """
        This method implements the desctructor of the class. The Desctructer
        closes the file when the object is destroyed.
        """

        self.file.close()


    def __iter__(self):
        return self.file


    def __next__(self):
        return self.file.next()


    def _read_file(self):
        """
        This method reads the whole file once and creates the table _line_offset
        which stores the position of the newlines in the file.  
        """

        offset = 0
        for index, line in enumerate(self.file):
            self._line_offset[index] = offset
            offset += len(line)
        self.file.seek(0)


    def close(self):
        self.file.close()


    def tell(self):
        return self.file.tell()


    def seek(self, pos):
        self.file.seek(pos)


    def readline(self):
        return self.file.readline()


    def fileno(self):
        return self.file.fileno()

    """
    def read_line_by_number(self, line_number):
        self.file.seek(self._line_offset[line_number])
        return self.file.readline()
    """
