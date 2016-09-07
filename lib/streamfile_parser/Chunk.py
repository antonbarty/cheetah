#
# Author: Dominik Michels
# Date: August 2016
#

import re
import textwrap
import math

from .LargeFile import *
from .Crystal import *
from .StreamfileParserFlags import *
from .ParserError import *

class Chunk:
    """
    This class is a datastructure to store the information of a Chunk in the
    streamfile.
    """

    def __init__(self, stream_filename, stream_file, clen, clen_codeword):
        """
        Constructor of the class

        Args:
            stream_filename: Filepath to the stream file on the harddrive
            stream_file: File/LargeFile object of the stream file
            clen: value of the clen in the geometry file
            clen_codeword: string under which the clen is stored in the chunk
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
        self.clen_codeword = clen_codeword
        self.clen = clen
        self.first_line = None
        self.last_line = None 
        self.begin_peaks_pointer = None 
        self.end_peaks_pointer = None 
        self.crystals = []
        self.num_crystals = 0

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

        self._integer_matching_pattern = r"""
            (?<![-.])\b[0-9]{1,}\b(?!\.[0-9])
        """

        self._float_matching_regex = re.compile(
            self._float_matching_pattern, re.VERBOSE)

        self._integer_matching_regex = re.compile(
            self._integer_matching_pattern, re.VERBOSE)

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
        print("First line: ", self.first_line)
        print("Last line: ", self.last_line)
        print("Crystal found: ", self.has_crystal())
        print("Number of crystals found: ", len(self.crystals))
        print("First peaks line: ", self.begin_peaks_pointer)
        print("Last peaks line: ", self.end_peaks_pointer)


    def _print_non_crit_error_message(self, property, line):
        print(textwrap.fill("Warning: The property \"" + property + "\" could" +
            " not be parsed", 80))
        print(textwrap.fill("Line: " + line, 80))
        print(textwrap.fill("Action: Set " + property + " to nan by default", 
            80))
        print("")


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
                self.crystals[self.num_crystals - 1]. \
                    begin_predicted_peaks_pointer = next_line_pointer
        if self.clen_codeword is not None:
            if self.clen_codeword in line:
                # we split the line first because the clen_codeword may 
                # contain some float number
                search_string = line.split("=")[1]
                matches = re.findall(
                    self._float_matching_regex, search_string)
                if len(matches) == 1:
                    self.clen = float(matches[0])
                else:
                    self.clen = float('nan')
                    self._print_non_crit_error_message("clen", line)
                    
        if "Image filename: " in line:
            self.cxi_filename = line.replace("Image filename: ", "").rstrip()
        elif "photon_energy_eV = " in line:
            # there may also be a different "hdf5/.../photon_energy_eV" tag
            # in the streamfile. We use just the plain "photon_energy_ev" entry
            if "hdf5" not in line:
                matches = re.findall(
                        self._float_matching_regex, line)
                if len(matches) == 1:
                    self.photon_energy = float(matches[0])
                else:
                    self.photon_energy = float('nan')
                    self._print_non_crit_error_message("photon energy", line)
        elif "beam_divergence = " in line:
            matches = re.findall(
                    self._float_matching_regex, line)
            if len(matches) == 1:
                self.beam_divergence = float(matches[0])
            else:
                self.beam_divergence = float('nan')
                self._print_non_crit_error_message("beam divergence", line)
        elif "beam_bandwidth = " in line:
            matches = re.findall(
                    self._float_matching_regex, line)
            if len(matches) == 1:
                self.beam_bandwidth = float(matches[0])
            else:
                self.beam_bandwidth = float('nan')
                self._print_non_crit_error_message("beam bandwidth", line)
        elif "indexed_by" in line:
            if not "indexed_by = none" in line:
                self.indexed = True
        elif "Event: " in line:
            matches = re.findall(self._integer_matching_regex, line)
            if len(matches) == 1:
                self.event = int(matches[0])
            else:
                raise ParserError("The event number could not be read.")
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
            self.num_crystals += 1
            self.crystals.append(Crystal())
            #self.crystal = True
        elif "Reflections measured after indexing" in line:
            self._flag = StreamfileParserFlags.predicted_peak
            return
        elif "End of reflections" in line:
            self._flag = StreamfileParserFlags.none
            self.crystals[self.num_crystals - 1].end_predicted_peaks_pointer = \
                current_line_pointer
            return
        elif "Cell parameters" in line:
            matches = re.findall(self._float_matching_regex, line)
            if len(matches) == 6:
                self.crystals[self.num_crystals-1].unit_cell = UnitCell(
                    float(matches[0]), float(matches[1]), float(matches[2]),
                    float(matches[3]), float(matches[4]), float(matches[5]))
                self.crystals[self.num_crystals - 1].unit_cell.to_angstroem()
            else:
                raise ParserError("The cell parameters could not be read.")
        elif "centering = " in line:
            self.crystals[self.num_crystals - 1].unit_cell.centering = \
                line.replace("centering = ", "").strip() 
        elif "diffraction_resolution_limit" in line:
            # we use the second resolution index in angstroem
            matches = re.findall(
               self._float_matching_regex, line)
            if len(matches) == 3:
                self.crystals[self.num_crystals - 1].resolution_limit = \
                    float(matches[2])
            else:
                self.crystals[self.num_crystals - 1].resolution_limit = \
                    float('nan')
                self._print_non_crit_error_message("diffraction resolution" + 
                    "limit", line)


    def _get_coordinates_from_streamfile(self, begin_pointer, end_pointer, 
        x_column, y_column):
        try:
            line_number = 1
            peak_x_data = []
            peak_y_data = []
            # check pointer validity
            if ((begin_pointer is None)
                or (end_pointer is None)):
                print("Error: Cannot find the peak information in the chunk")
                print("")
                return ([], [])

            self.stream_file.seek(begin_pointer)
            while(self.stream_file.tell() != end_pointer):
                line = self.stream_file.readline()
                matches = re.findall(self._float_matching_regex, line)
                try:
                    peak_x_data.append(float(matches[x_column]))
                    peak_y_data.append(float(matches[y_column]))
                except (IndexError, ValueError):
                    print("Error: Cannot parse the peak information")
                    print(textwrap.fill("Line: " + line, 80))
                    print("")
                    return ([],[])
            return (peak_x_data, peak_y_data)
        except IOError:
            print(textwrap.fill("Error: Cannot read the peak information from "+
                "streamfile: " + self.filename, 80))
            return ([], [])


    def get_hkl_indices_from_streamfile(self, crystal_index, peak_x, peak_y):
        """
        This method returns the hkl indices of the predicted bragg peaks of
        the crystal with the given index at the position (peak_x, peak_y)

        Args:
            crystal_index (int): Index of the crystal from which the unit cell
                is returned.
            peak_x (float): The x coordinate of the peak position
            peak_y (float): The y coordinate of the peak position

        Returns:
            list: The hkl indices
        """

        try:
            crystal = self.crystals[crystal_index]
            self.stream_file.seek(crystal.begin_predicted_peaks_pointer)
            # check pointer validity
            if ((crystal.begin_predicted_peaks_pointer is None)
                or (crystal.end_predicted_peaks_pointer is None)):
                print("Error: Cannot find the hkl information in the chunk")
                print("")
                return []

            while(self.stream_file.tell() != 
                crystal.end_predicted_peaks_pointer):
                line = self.stream_file.readline()
                matches = re.findall(self._float_matching_regex, line)
               
                try:
                    if(math.isclose(peak_x, float(matches[7]))
                        and math.isclose(peak_y, float(matches[8]))):
                        return [int(matches[0]), int(matches[1]), 
                            int(matches[2])]
                except (IndexError, ValueError):
                    print("Error: Cannot parse the hkl information")
                    print(textwrap.fill("Line: " + line, 80))
                    print("")
                    return []
        except IOError:
            print(textwrap.fill("Error: Cannot read the hkl information from " +
                "streamfile: " + self.filename, 80))
            print("")
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


    def has_crystal(self):
        """
        This method returns True if the Chunk has at least one crystal and
        False if no crystal is present.

        Returns:
            Bool
        """

        if (len(self.crystals) != 0):
            return True
        else:
            return False


    def get_number_of_crystals(self):
        """
        This method returns the number of crystals in the chunk.

        Returns:
            number_of_crystals (int): The number of crystals in the chunk
        """
        
        return (len(self.crystals))


    def get_unit_cell(self, crystal_index):
        """
        This method returns the unit cell of the crystal with the given index.

        Args:
            crystal_index (int): Index of the crystal from which the unit cell
                is returned.

        Returns:
            UnitCell: Unit cell of the crystal
        """

        return self.crystals[crystal_index].unit_cell


    def get_predicted_peak_data(self, crystal_index):
        """
        This method returns a list of all the predicted peaks from the crystal
        with the given index.

        Note:
            If the chunk contains no crystal an empty list is returned.

        Args:
            crystal_index (int): Index of the crystal from which the unit cell
                is returned.
                
        Returns:
            list: The list with the positions of the predicted peaks. The peaks 
                are stored in the format (x_position, y_position).
        """

        crystal = self.crystals[crystal_index] 
        return self._get_coordinates_from_streamfile(
                crystal.begin_predicted_peaks_pointer, 
                crystal.end_predicted_peaks_pointer, 7, 8)
