#
# Author: Dominik Michels 
# Date: August 2016
#

import sys
import re
import pprint

from BeamCharacteristicFlags import *
from PanelFlags import *

class GeometryFileParser:
    """
    This class provides the functionality to parse the CrystFEL geometry file.
    """

    def __init__(self, filename):
        """
        Args:
            filename (string): Filename of the geometry file.
        """

        self.lines = []
        self._read_geometry_file(filename)
        self.dictionary = {}
        self.dictionary['panels'] = {}
        self.dictionary['beam_characteristics'] = {}
        self.dictionary['bad_regions'] = {}
        self.dictionary['rigid_groups'] = {}
        self.dictionary['rigid_group_collections'] = {}
        self._parse()
        pprint.pprint(self.dictionary['panels'])
        pprint.pprint(self.dictionary['beam_characteristics'])
        pprint.pprint(self.dictionary['bad_regions'])


    def _read_geometry_file(self, filename):
        """
        This methods reads the geometry file line by line into the self.lines
        list. Comments and empty lines are removed.

        Args:
            filename (string): Filename of the geometry file.
        """

        try:
            with open(filename, "r") as f:
                for line in f:
                    line_parsed_comments = line.split(";", 1)[0].strip()
                    if line_parsed_comments:
                        self.lines.append(line_parsed_comments)
        except IOError:
            print("Error reading the geometry file: ", self.filename)
            exit()


    def _has_check(self, line, pattern):
        regex = re.compile(pattern, re.VERBOSE)

        if regex.match(line):
            return True
        else:
            return False


    def _get_information(self, line, pattern):
        regex = re.compile(pattern, re.VERBOSE)

        information, number_of_subs = re.subn(regex, "", line)
        if number_of_subs == 0:
            return ""
        else:
            return information


    def has_rigid_group_information(self, line):
        """
        This methods checks whether the current line has information about rigid
        groups in the detector

        Args:
            line (string): Line containing the rigid group information

        Returns:
            bool: True if it has information about rigid groups, False 
                otherwise
        """

        rigid_group_information_pattern = """
            (^[\s]*)          
            (rigid_group)
            (?!_collection)
            ([A-Za-z0-9_]+)     
            ([\s]*)          
            =
            ([\s]*)             
            ([A-Za-z0-9_]+)[\s]*  (,[\s]*[A-Za-z0-9_]+[\s]*)*$
        """

        return self._has_check(line, rigid_group_information_pattern)


    def has_rigid_group_collection_information(self, line):
        """
        This methods checks whether the current line has information about rigid
        groups in the detector

        Args:
            line (string): Line containing the rigid group information

        Returns:
            bool: True if it has information about rigid groups, False 
                otherwise
        """

        rigid_group_information_pattern = """
            (^[\s]*)           
            (rigid_group_collection)
            ([A-Za-z0-9_]+)    
            ([\s]*)             
            =
            ([\s]*)             
            ([A-Za-z0-9_]+)[\s]*  (,[\s]*[A-Za-z0-9_]+[\s]*)*$
        """

        return self._has_check(line, rigid_group_information_pattern)


    def get_rigid_group_information(self, line):
        """
        This method returns the rigid group information from the given line.

        Args:
            line (string): Line containing the rigid group information

        Returns:
            list: list of all the rigid groups

        """

        rigid_group_information_pattern = """
            (^[\s]*)          
            ([A-Za-z0-9_]+)     
            ([\s]*)          
            =
            ([\s]*)             
        """

        information = self._get_information(line, 
            rigid_group_information_pattern)
        stripped_information = "".join(information.split())
        return stripped_information.split(",")

    def get_rigid_group_collection_information(self, line):
        """
        This method returns the rigid group collection information from the 
        given line.

        Args:
            line (string): Line containing the rigid group collection 
            information

        Returns:
            list: list of all the rigid groups collections

        """

        return self.get_rigid_group_information(line)


    def has_bad_region_information(self, line):
        """
        This methods checks whether the current line has information about bad
        regions in the detector.

        Args:
            line (string): Line containing the local panel information

        Returns:
            bool: True if it has information about bad sections, False 
                otherwise
        """

        bad_region_pattern = """
            (^[\s]*)            # whitespaces at the beginning
            (?=bad)             
            ([A-Za-z0-9_]+)     # panel name
            \/
            ([A-Za-z0-9_]+)     # property name 
            ([\s]*)          
            =
            (.*$)               # property value
        """

        return self._has_check(line, bad_region_pattern)
    

    def get_bad_region_information(self, line):
        """
        This method returns the bad region information from the given line.

        Args:
            line (string): Line containing the bad region information

        Returns:
            string: Bad region information

        Note:
            If the line does not match a CrystFEL geometry bad region
            information line an empty string is returned.
        """

        local_panel_information_pattern = """
            (^[\s]*)            # whitespaces at the beginning
            (?=bad)             
            ([A-Za-z0-9_]+)     # panel name
            \/
            ([A-Za-z0-9_]+)     # property name 
            ([\s]*)          
            =
            ([\s]*)             # whitespaces after equality sign
        """

        return self._get_information(line, local_panel_information_pattern)


    def has_local_panel_information(self, line):
        """
        This methods checks if the current line is matching a specific panel
        flag contains information for a specific panel or for all following
        panels.

        Args:
            line (string): Line containing the local panel information

        Returns:
            bool: True is it only has information for one panel, False 
                otherwise
        """

        local_panel_information_pattern = """
            (^[\s]*)            # whitespaces at the beginning
            (?!bad)             # the panel information must not begin with bad
            ([A-Za-z0-9_]+)     # panel name
            \/
            ([A-Za-z0-9_]+)     # property name 
            ([\s]*)          
            =
            (.*$)               # property value
        """

        return self._has_check(line, local_panel_information_pattern) 


    def get_panel_name(self, line):
        """
        This method returns the panel name from a given line.

        Args:
            line (string): Line containing the panel name

        Returns:
            string: Panel name
        """

        panel_name_pattern = """
            \/
            ([A-Za-z0-9_]+)     # property name 
            ([\s]*)          
            =
            (.*$)               # property value
        """

        return self._get_information(line, panel_name_pattern)


    def get_bad_region_name(self, line):
        """
        This method returns the name of the bad region from a given line.

        Args:
            line (string): Line containing the bad region name

        Returns:
            string: Bad region name

        Note:
            This function is exactly the same as get_panel_name
        """

        return self.get_panel_name(line)


    def get_local_panel_information(self, line):
        """
        This method returns the local panel information from the given line.

        Args:
            line (string): Line containing the local panel information

        Returns:
            string: Local panel information

        Note:
            If the line does not match a CrystFEL geometry local panel
            information line an empty string is returned.
        """

        local_panel_information_pattern = """
            (^[\s]*)            # whitespaces at the beginning
            ([A-Za-z0-9_]+)     # panel name
            \/
            ([A-Za-z0-9_]+)     # property name 
            ([\s]*)          
            =
            ([\s]*)             # whitespaces after equality sign
        """

        return self._get_information(line, local_panel_information_pattern)


    def has_global_panel_information(self, line):
        """
        This method checks whether the current line has global panel
        information.

        Returns:
            bool: True if the information is global, False otherwise
        """

        global_panel_information_pattern = """
            (^[\s]*)            # whitespaces at the beginning
            (?!rigid_group)             # may not be a rigid group
            (?!rigid_group_collection)             # may not be a rigid group
            ([A-Za-z0-9_]+)     # property name 
            ([\s]*)          
            =
            (.*$)               # property value
        """
        
        return self._has_check(line, global_panel_information_pattern)


    def get_global_panel_information(self, line):
        """
        This method returns the global panel information from the given line.

        Args:
            line (string): Line containing the global panel information

        Returns:
            string: Global panel information

        Note:
            If the line does not match a CrystFEL geometry global panel
            information line an empty string is returned.
        """

        global_panel_information_pattern = """
            (^[\s]*)                    # whitespaces at the beginning
            (?!rigid_group)             # may not be a rigid group
            (?!rigid_group_collection)  # may not be a rigid group
            ([A-Za-z0-9_]+)             # property name 
            ([\s]*)                     # whitespaces after property name
            =
            ([\s]*)                     # whitespaces after equality sign
        """

        return self._get_information(line, global_panel_information_pattern)

        
    def has_beam_characteristics_information(self, line):
        """
        This method checks whether the current line has beam characteristics
        information.

        Returns:
            Bool: True if the information is global, False otherwise

        Note:
            The layout of the beam characteristic information is exactly the
            same as the layout of the global panel information. Therefore this
            method just wraps the has_global_panel_information method.
        """

        return self.has_global_panel_information(line)


    def get_beam_characteristics_information(self, line):
        """
        This method returns the beam characteristics information from the given 
        line.

        Args:
            line (string): Line containing the beam characteristics information

        Returns:
            string: Global panel information

        Note:
            If the line does not match a CrystFEL geometry beam characteristics
            information line an empty string is returned.

            The layout of the beam characteristic information is exactly the
            same as the layout of the global panel information. Therefore this
            method just wraps the has_global_panel_information method.

        """
        
        return self.get_global_panel_information(line)


    def get_flag(self, line):
        """
        This method returns the CrystFEL geometry flag (eg. ss, fs,
        photon_energy_eV, ...) from a given line. If no flag is found an empty
        string is returned.

        Args:
            line (string): Line containing the beam characteristics information

        Returns:
            string: Flag

        """

        flag = ""

        is_characteristic = self.has_beam_characteristics_information(line)
        is_panel = self.has_local_panel_information(line)
        is_global = self.has_global_panel_information(line)
        is_rigid_group = self.has_rigid_group_information(line)
        is_rigid_group_collection = self.has_rigid_group_collection_information(
            line)
        is_bad_region = self.has_bad_region_information(line)
        if (is_characteristic
            or is_global
            or is_rigid_group
            or is_rigid_group_collection):

            flag_pattern = """
                ([\s]*)
                =
                (.*)
            """

            regex = re.compile(flag_pattern, re.VERBOSE)
            flag = re.sub(regex, "", line)
        elif (is_panel or is_bad_region):
            flag_pattern1 = """
                (^[\s]*)
                [A-Za-z0-9_]+
                \/
            """

            flag_pattern2 = """
                ([\s]*)
                =
                (.*)
            """

            regex1 = re.compile(flag_pattern1, re.VERBOSE)
            regex2 = re.compile(flag_pattern2, re.VERBOSE)
            sub_line = re.sub(regex1, "", line)
            flag = re.sub(regex2, "", sub_line)
        if (is_panel or is_characteristic):
            # check if found flags are valid
            for key in BeamCharacteristicFlags.list + PanelFlags.list:
                if key == flag:
                    return flag
            return ""
        else:
            return flag


    def convert_type(self, key, value):
        if (key == "ss" or key == "fs"):
            stripped = "".join(value.split())
            vector_pattern = """
            ^\s*
            ([-+]?  \s* (?: (?: \d* \. \d+) | (?: \d+ \.? )))?
            \s*
            x
            \s*
            [-+]?  (\s* (?: (?: \d* \. \d+) | (?: \d+ \.? )))?
            y
            \s*$
            """

            regex = re.compile(vector_pattern, re.VERBOSE)
            if regex.match(stripped):
                float_pattern = """
                    ([-+]?  \s* (?: (?: \d* \. \d+) | (?: \d+ \.? )))
                """
                regex = re.compile(float_pattern, re.VERBOSE)
                matches = re.findall(regex, stripped)
                if len(matches) == 2:
                    return {'x' : float(matches[0]), 'y' : float(matches[1])}
                elif len(matches) == 0:
                    return {'x' : 1.0, 'y' : 1.0}
                else:
                    x_pattern = "^x.*$"
                    regex = re.compile(x_pattern, re.VERBOSE)
                    if regex.match(stripped):
                        return {'x' : 1.0, 'y' : float(matches[0])}
                    else:
                        return {'x' : float(matches[0]), 'y' : 1.0}
            else:
                #TODO: improve error handling here
                print("Geometry file corrupted.")
                print("Unable to parse the", key, "property :", value)
        else:
            try:
                return int(value)
            except (ValueError, TypeError):
                try:
                    return  float(value)
                except (ValueError, TypeError):
                    return value


    def _parse(self):
        """
        This methods parses the geometry file.
        """

        beam_flags = BeamCharacteristicFlags.list
        panel_flags = PanelFlags.list

        # we setup a dictionary storing current global panel information
        global_panel_properties = {}
        panels = set()
        bad_regions = set()

        for line in self.lines:
            # every line has to contain a flag, get the flag first
            flag = self.get_flag(line)
            if flag == "":
                print("Geometry file is corrupted in line: ", line)
                print("The flag \"", flag, "\" is not a valid CrystFEL" \
                    "geometry flag.")
                exit()

            # rigid groups
            if(self.has_rigid_group_information(line)):
                information = self.get_rigid_group_information(line)
                self.dictionary['rigid_groups'][flag] = \
                    self.convert_type(flag, information)
            # rigid group collections
            elif(self.has_rigid_group_collection_information(line)):
                information = self.get_rigid_group_collection_information(line)
                self.dictionary['rigid_group_collections'][flag] = \
                    self.convert_type(flag, information)
            # search for panel characteristics
            # check first if the flag is for one panel only or global
            # information valid for many panels
            elif self.has_local_panel_information(line):
                panel_name = self.get_panel_name(line)
                # check whether the panel has already appeared in the geometry
                if panel_name not in panels:
                    panels.add(panel_name)
                    self.dictionary['panels'][panel_name] = {}
                    # set the current global information about panels  in the 
                    # dictionary
                    for key in global_panel_properties:
                       self.dictionary['panels'][panel_name][key] = \
                        global_panel_properties[key] 
                # set the local line information in the dictionary
                information = self.get_local_panel_information(line)
                self.dictionary['panels'][panel_name][flag] = \
                    self.convert_type(flag, information)
            # global panel information has the same shape as beam characteristic
            # information, we can just separate them by the flag
            elif self.has_global_panel_information(line):
                if flag in BeamCharacteristicFlags.list:
                    information = self.get_beam_characteristics_information(
                        line)
                    self.dictionary['beam_characteristics'][flag] = \
                        self.convert_type(flag, information)
                else: 
                    information = self.get_global_panel_information(line)
                    global_panel_properties[flag] = \
                        self.convert_type(flag, information)
            # bad regions
            elif self.has_bad_region_information(line):
                bad_region_name = self.get_bad_region_name(line)
                if bad_region_name not in bad_regions:
                    bad_regions.add(bad_region_name)    
                    self.dictionary['bad_regions'][bad_region_name] = {}
                information = self.get_bad_region_information(line)
                self.dictionary['bad_regions'][bad_region_name][flag] = \
                    self.convert_type(flag, information)
            else:
                print("Geometry file is corrupted in line: ", line)
                exit()
