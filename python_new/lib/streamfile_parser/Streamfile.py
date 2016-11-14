#
# Author: Dominik Michels
# Date: August 2016
#

import tempfile
import os
import textwrap

from  ..geometry_parser import GeometryFileParser as geom
from .Chunk import *
from .StreamfileParserFlags import *
from .LargeFile import *
from .ParserError import *


class Streamfile:
    """
    This class is a datastructure to store the information in the stream file.
    """

    def __init__(self, filename):
        """
        Constructor of the class

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
        self.geometry = None
        self._temporary_geometry_file = None
        self._gen_temporary_geometry_file()
        self.parse_streamfile()
        
        self.clen = None
        self.clen_codeword = None


    def __exit__(self, exc_type, exc_value, traceback):
        if not self._temporary_geometry_file.closed:
            self._temporary_geometry_file.close()


    def get_geometry(self):
        """
        This method returns the geometry information in the format requiered
        for the cxiview.py program.

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
        
        return self.chunks[index].has_crystal()

        
    def get_unit_cell(self, index, crystal_index = 0):
        """
        This method returns the UnitCell object of a specific chunk if a
        crystal was found. If no crystal was found None is returned.

        Args:
            index (int): The number of the chunk from which the unit cell
                is returned.

            crystal_index (int): Index of the crystal from which the unit cell
                is returned.

        Returns:
            unit cell (UnitCell): The unit cell of the chunk
        """

        return self.chunks[index].get_unit_cell(crystal_index)


    def get_predicted_peak_data(self, chunk_index, crystal_index = 0):
        """
        This methods return a list of all the predicted peaks in the streamfile
        corresponding to a specific chunk.

        Args:
            index (int): The number of the chunk from which the peak information
                should be returned.

            crystal_index (int): Index of the crystal from which the unit cell
                is returned.

        Returns:
            list: The list with the positions of the predicted peaks. The peaks 
                are stored in the format (x_position, y_position).
        """

        return self.chunks[chunk_index].get_predicted_peak_data(crystal_index)


    def get_number_of_crystals(self, chunk_index):
        """
        This method returns the number of crystals in a specific chunk.
        
        Args:
            index (int): The number of the chunk from which the number of 
                crystals should be returned.

        Returns:
            int: The number of crystals in the chunk
        """
        
        return self.chunks[chunk_index].get_number_of_crystals()


    def get_number_of_chunks(self):
        """
        This methods returns the number of chunks in the streamfile.

        Returns:
            int: The number of chunks
        """

        return len(self.chunks)


    def get_event_id(self, chunk_index):
        """
        This method returns the event id of the chunk.

        Args:
            chunk_index (int): The number of the chunk from which the number of 
                crystals should be returned.

        Returns:
            int: The event id of the chunk
        """

        return self.chunks[chunk_index].event


    def get_hkl_indices(self, peak_x, peak_y, chunk_index, crystal_index = 0):
        """
        This methods returns the hkl index of peak corresponding to the
        given coordinates, the given crystal in the given chunk.

        Args:
            peak_x (float): x coordinate of the peak
            peak_y (float): y coordinate of the peak
            chunk_index (int): The number of the chunk in which the crystal is
                located
            crystal_index (int): The number of the crystal which the peak 
                belongs to

        Returns:
            list: hkl indices
        """

        return self.chunks[chunk_index].get_hkl_indices_from_streamfile(
            crystal_index, peak_x, peak_y)


    def get_cxi_filenames(self):
        """
        This method returns the cxi/h5 filenames in the streamfile.

        Returns:
            list: List of filenames

        Note: 
            This method is deprecated and has been replaced by the method
            get_cxiview_event_list which gives more detailed information
            about the events including filenames and event number.
        """

        list_of_filenames = [] 
        for chunk in self.chunks:
            # TODO: better use a ordered set class here
            if chunk.cxi_filename not in list_of_filenames:
                list_of_filenames.append(chunk.cxi_filename)
        return list_of_filenames


    def get_cxiview_event_list(self):
        """
        This method returns a dictionary with the event information the
        cxiviewer needs to display the events correctly.

        Returns:
            dict: Dictionary containing the event information
        """

        nevents = len(self.chunks)
        filenames = []
        eventids = []
        h5fields = []
        formats = []

        try:
            # TODO: We assume here that the data is stored at the same position
            # for every panel. That may not be true. But currently the cxiviewer
            # does not support different storage positions for different panels
            field = next(iter(self.geometry['panels'].values()))['data']
        except KeyError:
            field = "data/data"

        for chunk in self.chunks:
            filenames.append(chunk.cxi_filename) 
            eventids.append(chunk.event)
            h5fields.append(field)

            basename = os.path.basename(chunk.cxi_filename)
            if basename.endswith(".h5") and basename.startswith("LCLS"):
                formats.append("cheetah_h5")
            elif basename.endswith(".h5"):
                formats.append("generic_h5")
            else:
                formats.append("cxi")

        result = {
            'nevents' : nevents,
            'filename': filenames,
            'event': eventids,
            'h5field': h5fields,
            'format': formats
        }
        return result


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
        is stored corresponding Streamfile class variables.
        """

        # start to parse the streamfile
        # we write the geometry file into a temporary directory to process it
        # with the cheetah function read_geometry in lib/cfel_geometry
        geometry_lines = []
        filenames = []

        flag = StreamfileParserFlags.none
        flag_changed = False
        skip_chunk = False
        # line numbering starts at 1
        line_number = 1
        new_chunk = None
        clen = None
        clen_codeword = None

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
                        #self._geom_dict = cfel_geom.read_geometry(
                            #self._temporary_geometry_file.name)
                        geom_parser = geom.GeometryFileParser(
                            self._temporary_geometry_file.name)
                        geom_parser.parse()
                        self.geometry = geom_parser.dictionary
                        self._geom_dict = geom_parser.pixel_map_for_cxiview()

                        # handle the clen property which can be given either as
                        # a float directly or as a codeword in the chunk
                        clen = self._geom_dict['clen']
                        if isinstance(clen, str):
                            clen_codeword = clen
                            clen = None
                           
                        flag_changed = True
                elif "Begin chunk" in line:
                    flag = StreamfileParserFlags.chunk
                    # the clen and clen_codeword parameters has to be passed
                    # from the geometry file to the chunk. the chunk cannot
                    # determine that information by itself
                    new_chunk = Chunk(self.filename, self.file, clen, 
                        clen_codeword)
                    
                    new_chunk.begin_pointer = next_line_pointer
                    flag_changed = True
                elif "End chunk" in line:
                    flag = StreamfileParserFlags.none
                    new_chunk.end_pointer = current_line_pointer
                    if not skip_chunk:
                        # check if begin chunk marker was present
                        if new_chunk is not None:
                            self.chunks.append(new_chunk)
                    new_chunk = None
                    skip_chunk = False
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
                        try:
                            if not skip_chunk:
                                new_chunk.parse_line(line, 
                                    previous_line_pointer, current_line_pointer,
                                    next_line_pointer)
                        except ParserError as e:
                            print(textwrap.fill("Error: " + e.args[0], 80))
                            print(textwrap.fill("Line: " + line, 80))
                            print("Action: Skipping current chunk")
                            print("")
                            skip_chunk = True
                else:
                    flag_changed = False
                    
                line_number += 1
                previous_line_pointer = current_line_pointer
                current_line_pointer = next_line_pointer
        except IOError:
            print("Cannot read from streamfile: ", self.filename)
            exit()
