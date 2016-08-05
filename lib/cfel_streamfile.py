import numpy
import tempfile
import lib.cfel_geometry as cfel_geom
import re
import sys
import os
import math

class UnitCell:
    """
    This class is a data structure to store the information of a unit cell. The
    unit of the length of the axis is supposed to be in nm and the unit of the
    angles supposed to be degree.  
    """

    def __init__(self, a, b, c, alpha, beta, gamma):
        """
        Note:
            The centering parameter of the unit cell is not required in the
            constructer but can be set later.

        Args:
            a: The length of the a-axis of the unit cell in nm
            b: The length of the b-axis of the unit cell in nm
            c: The length of the c-axis of the unit cell in nm
            alpha: The length of the alpha angle of the unit cell
            beta: The length of the beta angle of the unit cell
            gamma: The length of the gamma angle of the unit cell
        """
        self.a = a
        self.b = b
        self.c = c
        self.alpha = alpha
        self.beta = beta
        self.gamma = gamma
        self.centering = None

    def to_angstroem(self):
        """
        This method converts the length of the unit cell axis from nm to 
        Angstroem.
        """

        self.a /= 10.0
        self.b /= 10.0
        self.c /= 10.0
    
    def dump(self):
        """
        This method prints the unit cell parameters.
        """

        print("Unit Cell:")
        print("a: ", self.a)
        print("b: ", self.b)
        print("c: ", self.c)
        print("alpha: ", self.alpha)
        print("beta: ", self.beta)
        print("gamma: ", self.gamma)
        print("centering: ", self.centering)


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
        
        
class Chunk:
    """
    This class is a datastructure to store the information of a Chunk in the
    streamfile.
    """

    def __init__(self, stream_filename, stream_file):
        """
        Args:
            stream_filename: Filepath to the stream file on the harddrive
            stream_file: File/LargeFile object of the stream file
        """

        # general properties of the chunk
        self.stream_filename = stream_filename
        self.stream_file = stream_file
        self.cxi_filename = None
        self.contained_in_cxi = False
        self.event = None
        self.indexed = False
        self.photon_energy = -1.0
        self.beam_divergence = -1.0
        self.beam_bandwidth = -1.0
        self.average_camera_length = -1.0
        self.num_peaks = None 
        self.num_saturated_peaks = None 
        self.first_line = None
        self.last_line = None 
        self.begin_peaks_pointer = None 
        self.end_peaks_pointer = None 

        # properties if a crystal has been found
        self.crystal = False
        self.begin_predicted_peaks_pointer = None 
        self.end_predicted_peaks_pointer = None
        self.unit_cell = None
        self.resolution_limit = None

        # implementation specific class variables. Regular expressions
        # are needed for parsing the streamfile
        self._float_matching_pattern = r"""
            [-+]? # optional sign
            (?:
            (?: \d* \. \d+ ) # .1 .12 .123 etc 9.1 etc 98.1 etc
            |
            (?: \d+ \.? ) # 1. 12. 123. etc 1 12 123 etc
            )
            # followed by optional exponent part if desired
            (?: [Ee] [+-]? \d+ ) ?
        """
        self._float_matching_regex = re.compile(
            self._float_matching_pattern, re.VERBOSE)

        self._flag = StreamfileParserFlags.none

    def dump(self):
        """
        This method prints all the Chunk information.
        """

        print("""-----------------------------------------------------------""")
        print("Dumping all the chunk information.")
        print("Filename: ", self.cxi_filename)
        print("Event: ", self.event)
        print("Indexed: ", self.indexed)
        print("Photon Energy [eV]: ", self.photon_energy)
        print("Beam divergence [rad]: ", self.beam_divergence)
        print("Average camera length [m]: ", self.average_camera_length)
        print("Num peaks: ", self.num_peaks)
        print("Num saturated peaks: ", self.num_saturated_peaks)
        print("First line: ", self.first_line)
        print("Last line: ", self.last_line)
        print("Crystal found: ", self.crystal)
        print("First peaks line: ", self.begin_peaks_pointer)
        print("Last peaks line: ", self.end_peaks_pointer)

        if(self.crystal == True):
            print("First predicted peaks line: ", 
                self.begin_predicted_peaks_pointer)
            print("Last predicted peaks line: ", 
                self.end_predicted_peaks_pointer)
            self.unit_cell.dump()
            print("Resolution limit: ", self.resolution_limit)

    def parse_line(self, line, previous_line_pointer, current_line_pointer, 
        next_line_pointer):
        """
        This methods parses one line of the stream file. The information in the
        line is stored in the corresponding Chunk variables. 

        Args:
            line: String containing the the line of the streamfile which shall
                be parsed.
            previous_line_pointer: Integer storing the beginning position of
                the previous line in the stream file.
            current_line_pointer: Integer storing the beginning position of
                the current line in the stream file.
            next_line_pointer: Integer storing the beginning position of the 
                next line in the stream file.


        Note:
            The information of the beginning positions of the previous, current
            and next line is needed to read information from the stream file
            after the parsing process. With the method seek of a python file it
            is possible to set the file pointer to the desired position and
            read from there. This is really fast and used to read crystal and
            predicted peak information when needed from the streamfile.
        """

        if self._flag == StreamfileParserFlags.peak:
            if "fs/px" in line:
                self.begin_peaks_pointer = next_line_pointer

        if self._flag == StreamfileParserFlags.predicted_peak:
            if "fs/px" in line:
                self.begin_predicted_peaks_pointer = next_line_pointer

        if "Image filename: " in line:
            self.cxi_filename = line.replace("Image filename: ", "").rstrip()
        elif "photon_energy_eV = " in line:
            # there may also be a different "hdf5/.../photon_energy_eV" tag
            # with the in the streamfile. we use just the plain
            # "photon_energy_ev" entry
            if "hdf5" not in line:
                if self._float_matching_regex.match(line):
                    self.photon_energy = float(re.findall(
                        self._float_matching_regex, line)[0])
                else:
                    self.photon_energy = float('nan')
        elif "beam_divergence = " in line:
            if self._float_matching_regex.match(line):
                self.beam_divergence = float(re.findall(
                    self._float_matching_regex, line)[0])
            else:
                self.beam_divergence = float('nan')
        elif "beam_bandwidth = " in line:
            if self._float_matching_regex.match(line):
                self.beam_bandwidth = float(re.findall(
                    self._float_matching_regex, line)[0])
            else:
                self.beam_bandwidth = float('nan')
        elif "average_camera_length = " in line:
            if self._float_matching_regex.match(line):
                self.average_camera_length = float(re.findall(
                    self._float_matching_regex, line)[0])
            else:
                self.average_camera_length = float('nan')
        elif "num_peaks = " in line:
            self.num_peaks = int(line.replace("num_peaks = ", ""))
        elif "num_saturated_peaks = " in line:
            self.num_saturated_peaks = int(
                line.replace("num_saturated_peaks = ", ""))
        elif "indexed_by" in line:
            if not "indexed_by = none" in line:
                self.indexed = True
        elif "Event: " in line:
            self.event = line.replace("Event: ", "").rstrip()
        # It would maybe be a better style to set the line numbers from the
        # Streamfile class such that the class Chunk has no information about 
        # the current line number. Maybe implement this later.
        elif "Peaks from peak search" in line:
            self._flag = StreamfileParserFlags.peak
            return
        elif "End of peak list" in line:
            self._flag = StreamfileParserFlags.none
            self.end_peaks_pointer = current_line_pointer 
            return
        elif "Begin crystal" in line:
            self.crystal = True
        elif "Reflections measured after indexing" in line:
            self._flag = StreamfileParserFlags.predicted_peak
            return
        elif "End of reflections" in line:
            self._flag = StreamfileParserFlags.none
            self.end_predicted_peaks_pointer = current_line_pointer
            return
        elif "Cell parameters" in line:
            self.unit_cell = UnitCell(
                float(re.findall(self._float_matching_regex, line)[0]),
                float(re.findall(self._float_matching_regex, line)[1]),
                float(re.findall(self._float_matching_regex, line)[2]),
                float(re.findall(self._float_matching_regex, line)[3]),
                float(re.findall(self._float_matching_regex, line)[4]),
                float(re.findall(self._float_matching_regex, line)[5]))
            self.unit_cell.to_angstroem()
        elif "centering = " in line:
            self.unit_cell.centering = line.replace("centering = ", "").strip() 
        elif "diffraction_resolution_limit" in line:
            # we use the second resolution index in angstroem
            if self._float_matching_regex.match(line):
                self.resolution_limit = float(re.findall(
                    self._float_matching_regex, line)[2])
            else:
                self.resolution_limit = float('nan')

    def _get_coordinates_from_streamfile(self, begin_pointer, end_pointer, 
        x_column, y_column):
        try:
            line_number = 1
            peak_x_data = []
            peak_y_data = []

            self.stream_file.seek(begin_pointer)
            while(self.stream_file.tell() != end_pointer):
                line = self.stream_file.readline()
                matches = re.findall(self._float_matching_regex, line)
                # TODO: look up if fs is always x in the cheetah 
                # implementation
                peak_x_data.append(float(matches[x_column]))
                peak_y_data.append(float(matches[y_column]))

            return (peak_x_data, peak_y_data)
        except IOError:
            print("Cannot read the peak information from streamfile: ", 
                self.filename)
            return ([], [])

    def get_hkl_indices_from_streamfile(self, peak_x, peak_y):
        """
        This method returns the hkl indices of the predicted bragg peak at the
        position (peak_x, peak_y)

        Args:
            peak_x (float): The x coordinate of the peak position
            peak_y (float): The y coordinate of the peak position

        Returns:
            list: The hkl indices
        """

        try:
            self.stream_file.seek(self.begin_predicted_peaks_pointer)
            while(self.stream_file.tell() != self.end_predicted_peaks_pointer):
                line = self.stream_file.readline()
                matches = re.findall(self._float_matching_regex, line)
               
                if(math.isclose(peak_x, float(matches[7]))
                    and math.isclose(peak_y, float(matches[8]))):
                    return [int(matches[0]), int(matches[1]), int(matches[2])]
        except IOError:
            print("Cannot read the peak information from streamfile: ", 
                self.filename)
        return []

    def get_peak_data(self):
        """
        This method returns a list of all the peaks in the chunk of the 
        streamfile.

        Returns:
            list: The list with the positions of the peaks. The peaks are
            stored in the format (x_position, y_position).
        """

        return self._get_coordinates_from_streamfile(self.begin_peaks_pointer,
            self.end_peaks_pointer, 0, 1)

    def get_predicted_peak_data(self):
        """
        This method returns a list of all the predicted peaks in the chunk of 
        the streamfile.

        Note:
            If the chunk contains no crystal an empty list is returned.

        Returns:
            list: The list with the positions of the predicted peaks. The peaks 
                are stored in the format (x_position, y_position).
        """

        if self.crystal == True:
            return self._get_coordinates_from_streamfile(
                self.begin_predicted_peaks_pointer, 
                self.end_predicted_peaks_pointer, 7, 8)
        else:
            return []
        

class Streamfile:
    """
    This class is a datastructure to store the information in the stream file.
    """

    def __init__(self, filename):
        """
        Args:
            filename: Filepath to the stream file on the harddrive
        """

        self.filename = filename

        try: 
            self.file = LargeFile(filename) 
        except IOError:
            print("Cannot read from streamfile: ", self.filename)
            exit()

        self.chunks = []
        # we need a variable to check if the geometry has already been
        # processed because the geometry flag migth appear multiple times
        # in a stream file due to stream concatenation
        self._geometry_processed = False
        self._geom_dict = None
        self._temporary_geometry_file = None
        self._gen_temporary_geometry_file()
        self.parse_streamfile()

    def __exit__(self, exc_type, exc_value, traceback):
        if not self._temporary_geometry_file.closed:
            self._temporary_geometry_file.close()

    def get_geometry(self):
        """
        This method returns the geometry information in the format requiered
        for the cxiviewer

        Return:
            dict: The geometry information.
        """

        return self._geom_dict

    def get_peak_data(self, index):
        """
        This methods return a list of all the peaks in the streamfile
        corresponding to a specific chunk.

        Args:
            index (int): The number of the chunk from which the peak information
                is returned.

        Returns:
            list: The list with the positions of the peaks. The peaks are
                stored in the format (x_position, y_position).
        """
        return self.chunks[index].get_peak_data()

    def has_crystal(self, index):
        """
        This method returns True or False depending on whether a crystal was
        found in a specific chunk.

        Args:
            index (int): The number of the chunk from which the information
                is returned.

        Returns:
            bool: True if a crystal was found. False if no crystal was found.
        """

        return self.chunks[index].crystal
        
    def get_unit_cell(self, index):
        """
        This method returns the UnitCell object of a specific chunk if a
        crystal was found. If no crystal was found None is returned.

        Args:
            index (int): The number of the chunk from which the unit cell
                is returned.

        Returns:
            unit cell (UnitCell): The unit cell of the chunk
        """

        return self.chunks[index].unit_cell

    def get_predicted_peak_data(self, chunk_index):
        """
        This methods return a list of all the predicted peaks in the streamfile
        corresponding to a specific chunk.

        Args:
            index (int): The number of the chunk from which the peak information
                should be returned.

        Returns:
            list: The list with the positions of the predicted peaks. The peaks 
                are stored in the format (x_position, y_position).
        """

        if(self.chunks[chunk_index].crystal == True):
            return self.chunks[chunk_index].get_predicted_peak_data()
        else:
            return []

    def get_hkl_indices(self, chunk_index, peak_x, peak_y):
        return self.chunks[chunk_index].get_hkl_indices_from_streamfile(
            peak_x, peak_y)

    def get_cxi_filenames(self):
        list_of_filenames = [] 
        for chunk in self.chunks:
            # TODO: better use a ordered set class here
            if chunk.cxi_filename not in list_of_filenames:
                list_of_filenames.append(chunk.cxi_filename)
        return list_of_filenames

    def _gen_temporary_geometry_file(self):
        try:
            self._temporary_geometry_file = tempfile.NamedTemporaryFile(
                mode="w", suffix=".geom", delete = False)
        except IOError:
            print("""Generating the temporary geometry file failed.
                Probably the program does not have the right to write on
                disk.""")
            exit()

    def _write_temporary_geometry_file(self, geometry_lines):
        try:
            for line in geometry_lines: 
                self._temporary_geometry_file.write("%s\n" % line)
            self._temporary_geometry_file.flush()
            self._geometry_processed = True
        except IOError:
            print("""Writing the temporary geometry file failed.
                Probably the program does not have the right to write on
                disk.""")
            exit()

    def parse_streamfile(self):
        """
        This methods parses the stream file. The information in the stream file
        is stored corresponding Streamfile variables.
        """

        # start to parse the streamfile
        # we write the geometry file into a temporary directory to process it
        # with the cheetah function read_geometry in lib/cfel_geometry
        geometry_lines = []
        filenames = []

        flag = StreamfileParserFlags.none
        flag_changed = False
        # line numbering starts at 1
        line_number = 1
        new_chunk = None


        filesize = os.fstat(self.file.fileno()).st_size
        current_line_pointer = 0
        previous_line_pointer = 0
        next_line_pointer = 0
        try:
            for line in self.file:
                next_line_pointer = current_line_pointer + len(line)

                # scan the line for keywords and perform end or begin flag 
                # operations
                if "Begin geometry file" in line:
                    if not self._geometry_processed:
                        flag = StreamfileParserFlags.geometry
                        flag_changed = True
                elif "End geometry file" in line:
                    if not self._geometry_processed:
                        flag = StreamfileParserFlags.none
                        self._write_temporary_geometry_file(geometry_lines)
                        self._geom_dict = cfel_geom.read_geometry(
                            self._temporary_geometry_file.name)
                        flag_changed = True
                elif "Begin chunk" in line:
                    flag = StreamfileParserFlags.chunk
                    new_chunk = Chunk(self.filename, self.file)
                    new_chunk.begin_pointer = next_line_pointer
                    flag_changed = True
                elif "End chunk" in line:
                    flag = StreamfileParserFlags.none
                    new_chunk.end_pointer = current_line_pointer
                    self.chunks.append(new_chunk)
                    flag_changed = True


                # Process active flags, python dictionary switch-case
                # alternative doesn't work here because we may need to
                # execute complex tasks depending on the flag which
                # may require many different arguments. Python sucks from
                # time to time.

                # if the flag has changed in the current line there is no
                # more to do
                if not flag_changed:
                    if flag == StreamfileParserFlags.geometry:
                        geometry_lines.append(line.strip())
                    elif flag == StreamfileParserFlags.chunk:
                        new_chunk.parse_line(line, previous_line_pointer, 
                            current_line_pointer, next_line_pointer)
                else:
                    flag_changed = False
                    
                line_number += 1
                previous_line_pointer = current_line_pointer
                current_line_pointer = next_line_pointer
        except IOError:
            print("Cannot read from streamfile: ", self.filename)
            exit()
