import numpy
import tempfile
import lib.cfel_geometry as cfel_geom
import re
import time
import sys

"""
This class is a data structure to store the information of a unit cell. The unit
of the length of the axis is supposed to be in nm and the unit of the angles
supposed to be degree.
"""
class UnitCell:
    def __init__(self, a, b, c, alpha, beta, gamma):
        self.a = a
        self.b = b
        self.c = c
        self.alpha = alpha
        self.beta = beta
        self.gamma = gamma

    def to_angstroem(self):
        self.a /= 10.0
        self.b /= 10.0
        self.c /= 10.0
    
    def dump(self):
        print("Unit Cell:")
        print("a: ", self.a)
        print("b: ", self.b)
        print("c: ", self.c)
        print("alpha: ", self.alpha)
        print("beta: ", self.beta)
        print("gamma: ", self.gamma)


"""
This class is providing flags used in the parsing process of the streamfile.
"""
class StreamfileParserFlags:
    none = 0
    geometry = 1
    chunk = 2
    peak = 3
    predicted_peak = 4
    flag_changed = 5


"""
This class provides the ability to handle large text files relatively quickly.
The funcionality to jump between different lines is implemented.
"""
class LargeFile:
    def __init__(self, name, mode="r"):
        self.name = name
        self.file = open(name, mode)
        print("Start counting line numbers.")
        self.length = sum(1 for line in self.file)
        print("End counting line numbers.")
        self.file.seek(0)
        self._line_offset = numpy.zeros(self.length)
        self._read_file()

    def __exit__(self):
        self.file.close()

    def __iter__(self):
        return self.file

    def __next__(self):
        return self.file.next()

    def _read_file(self):
        offset = 0
        print("Start searching newlines.")
        for index, line in enumerate(self.file):
            #self._line_offset.append(offset)
            #numpy.append(self._line_offset, offset)
            self._line_offset[index] = offset
            offset += len(line)
        self.file.seek(0)
        print("End searching newlines.")

    def close(self):
        self.file.close()

    def seek_line(self, line_number):
        self.file.seek(self._line_offset[line_number])

    def read_line(self, line_number):
        self.file.seek(self._line_offset[line_number])
        return self.file.readline()
        
        
"""
This class is providing the information of a specific chunk in the streamfile
"""
class Chunk:
    def __init__(self, stream_filename, stream_file):
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
        self.first_peaks_line = None 
        self.last_peaks_line = None 

        # properties if a crystal has been found
        self.crystal = None
        self.first_predicted_peaks_line = None 
        self.last_predicted_peaks_line = None
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

    # TODO: maybe put this function into the Streamfile class. Then all the
    # parsing activity is going on there. This may again result into a
    # cleaner style
    def dump(self):
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
        print("First peaks line: ", self.first_peaks_line)
        print("Last peaks line: ", self.last_peaks_line)

        if(self.crystal is not None):
            print("First predicted peaks line: ", 
                self.first_predicted_peaks_line)
            print("Last predicted peaks line: ", self.last_predicted_peaks_line)
            self.unit_cell.dump()
            print("Resolution limit: ", self.resolution_limit)

    def parse_line(self, line, line_number):
        flag = StreamfileParserFlags.none

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
            flag = StreamfileParserFlags.peak
            self.first_peaks_line = line_number + 2
            return
        elif "End of peak list" in line:
            flag = StreamfileParserFlags.none
            self.last_peaks_line = line_number - 1
            return
        elif "Begin crystal" in line:
            self.crystal = True
        elif "Reflections measured after indexing" in line:
            flag = StreamfileParserFlags.predicted_peak
            # +2 here because there is a table header before the predicted peaks
            # we have to account for
            self.first_predicted_peaks_line = line_number + 2
            return
        elif "End of reflections" in line:
            flag = StreamfileParserFlags.none
            self.last_predicted_peaks_line = line_number - 1
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
        elif "diffraction_resolution_limit" in line:
            # we use the second resolution index in angstroem
            if self._float_matching_regex.match(line):
                self.resolution_limit = float(re.findall(
                    self._float_matching_regex, line)[2])
            else:
                self.resolution_limit = float('nan')

    def _get_coordinates_from_streamfile(self, first_line, last_line, 
        x_column, y_column):
        try:
            #print("first line: ", first_line)
            #print("last line: ", last_line)
            #f = open(self.stream_filename, "r")
            line_number = 1
            peak_x_data = []
            peak_y_data = []

            for line_number in xrange(first_line, last_line):
                line = self.stream_file.read_line(line_number)
                matches = re.findall(self._float_matching_regex, line)
                # TODO: look up if fs is always x in the cheetah 
                # implementation
                peak_x_data.append(float(matches[x_column]))
                peak_y_data.append(float(matches[y_column]))

            return (peak_x_data, peak_y_data)
        except IOError:
            print("Cannot read from streamfile: ", self.filename, ". Quitting")
            exit()

    def get_peak_data(self):
        return self._get_coordinates_from_streamfile(self.first_peaks_line,
            self.last_peaks_line, 0, 1)

    def get_predicted_peak_data(self):
        return self._get_coordinates_from_streamfile(
            self.first_predicted_peaks_line, self.last_predicted_peaks_line, 
            7, 8)
        

"""
This class implements the parser of the crystfel streamfile with the help of a
state machine.
"""
class Streamfile:
    def __init__(self, filename):
        self.filename = filename

        try: 
            self.file = LargeFile(filename) 
        except IOError:
            print("Cannot read from streamfile: ", self.filename, ". Quitting")
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
        return self._geom_dict

    def get_peak_data(self, index):
        return self.chunks[index].get_peak_data()

    def has_crystal(self, index):
        return self.chunks[index].crystal

    def get_predicted_peak_data(self, index):
        if(self.chunks[index].crystal == True):
            return self.chunks[index].get_predicted_peak_data()
        else:
            return []

    def get_cxi_filenames(self):
        list_of_filenames = set([]) 
        for chunk in self.chunks:
            list_of_filenames.add(chunk.cxi_filename)
        return list(list_of_filenames)

    def _gen_temporary_geometry_file(self):
        self._temporary_geometry_file = tempfile.NamedTemporaryFile(mode="w",
            suffix=".geom", delete = False)

    def _write_temporary_geometry_file(self, geometry_lines):
        try:
            for line in geometry_lines: 
                self._temporary_geometry_file.write("%s\n" % line)
            self._temporary_geometry_file.flush()
            self._geometry_processed = True
        except IOError:
            print("""Writing the temporary geometry file failed.
                Maybe the program does not have the right to write on
                disk.""")

    def parse_streamfile(self):
        """
        Read the streamfile and process its content.
        """
        # start to parse the streamfile
        # we write the geometry file into a temporary directory to process it
        # with the cheetah function read_geometry in lib/cfel_geometry
        geometry_lines = []
        filenames = []

        #options = {
        #    StreamfileParserFlags.none: lambda x: None,
        #    StreamfileParserFlags.geometry: lambda x: geometry_lines.append(x),
        #    StreamfileParserFlags.chunk: _process_chunk
        #}

        flag = StreamfileParserFlags.none
        flag_changed = False
        # line numbering starts at 1
        line_number = 1
        new_chunk = None


        try:
            for line in self.file:
                if (line_number % (self.file.length//25) == 0):
                    print("{0:0.2f}".format(line_number/self.file.length*100),"% of the streamfile parsed.")
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
                    new_chunk.first_line = line_number + 1
                    flag_changed = True
                elif "End chunk" in line:
                    flag = StreamfileParserFlags.none
                    new_chunk.last_line = line_number - 1
                    self.chunks.append(new_chunk)
                    flag_changed = True


                # process active flags, python dictionary switch-case
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
                        new_chunk.parse_line(line, line_number)
                else:
                    flag_changed = False
                    
                line_number += 1
            print("Number of chunks found: ", len(self.chunks))
        except IOError:
            print("Cannot read from streamfile: ", self.filename, ". Quitting.")
            exit()
