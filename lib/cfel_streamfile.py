import numpy
import tempfile
import lib.cfel_geometry as cfel_geom

"""
This class is providing flags used in the parsing process of the streamfile.
"""
class StreamfileParserFlags:
    none = 0
    geometry = 1

"""
This class implements the parser of the crystfel streamfile with the help of a
state machine.
"""
class StreamfileParser:
    def __init__(self, filename):
        self.filename = filename
        self.__geom_dict = None
        self.__gen_temporary_geometry_file()
        self.read_streamfile()

    def __exit__(self, exc_type, exc_value, traceback):
        self.__temporary_geometry_file.close()

    def __gen_temporary_geometry_file(self):
        self.__temporary_geometry_file = tempfile.NamedTemporaryFile(mode="w",suffix=".geom",
            delete = True)

    def __write_temporary_geometry_file(self, geometry_lines):
        try:
            for line in geometry_lines: 
                self.__temporary_geometry_file.write("%s\n" % line)
        except IOError:
            print("""Writing the temporary geometry file failed.
                Maybe the program does not have the right to write on
                disk.""")

    def get_geometry(self):
        return self.__geom_dict

    def read_streamfile(self):
        """
        Read the streamfile and process its content.
        """
        f = open(self.filename, 'r')
        f_lines = []
        for line in f:
            f_lines.append(line)

        # start to parse the streamfile
        # we write the geometry file into a temporary directory to process it
        # with the cheetah function read_geometry in lib/cfel_geometry
        geometry_lines = []

        options = {
            StreamfileParserFlags.none: lambda x: None,
            StreamfileParserFlags.geometry: lambda x: geometry_lines.append(x)
        }

        flag = StreamfileParserFlags.none
        for line in f_lines:
            # scan the line for keywords and perform end or begin flag 
            # operations
            # TODO: maybe also use a dictionary here
            if "Begin geometry file" in line:
                flag = StreamfileParserFlags.geometry
            elif "End geometry file" in line:
                flag = StreamfileParserFlags.none
                self.__write_temporary_geometry_file(geometry_lines)
                self.__geom_dict = cfel_geom.read_geometry(self.__temporary_geometry_file.name)
                               
            # process active flags
            options[flag](line)
