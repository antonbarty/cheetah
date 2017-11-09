# -*- coding: utf-8 -*-
#
#	CFEL image handling tools
#	Anton Barty
#

import socket

#
#   Determine where we are running the Cheetah GUI
#   Useful for setting default directory paths, batch queue commands, etc
#
def determine_location():

    print("Determining location:")
    hostname = socket.getfqdn()
    print("Hostname: ", hostname)




    #
    #   Determine where we are
    #   Enables location configuration to be overridden separately from what is determined here, eg: from command line
    #
    if hostname.endswith("slac.stanford.edu"):
        print("Looks like we are at SLAC.")
        location = 'LCLS'

    elif hostname.endswith("pcdsn"):
        print("Looks like we are on psana at LCLS.")
        location = 'LCLS'

    elif hostname.startswith('max-exfl') and hostname.endswith("desy.de"):
        print("Looks like we are on the EuXFEL Maxwell nodes.")
        location = 'max-exfl'

    elif hostname.startswith('max-cfel') and hostname.endswith("desy.de"):
        print("Looks like we are on a CFEL maxwell node.")
        location = 'max-cfel'

    elif hostname.startswith('max') and hostname.endswith("desy.de"):
        print("Looks like we are on a maxwell node.")
        location = 'max-cfel'

    elif hostname.endswith("desy.de"):
        print("Looks like we are somewhere at DESY.")
        location = 'DESY'

    elif hostname.endswith("xfel.eu"):
        print("Looks like we are on the euXFEL network.")
        location = 'euXFEL'

    else:
        print("Unable to determine location from hostname")
        location = 'None'

    return location




#
#   Set location specific default directory paths, batch queue commands, etc
#
def set_location_configuration(location="Default"):

    print("Setting location as ", location)
    #
    #   Location specific configurations, as needed
    #
    result = {}

    if  location=='LCLS':

        config = {
            'qcommand' : 'bsub -q psanaq'
        }
        result.update(config)

    elif location == 'max-exfl':
        config = {
            'qcommand': 'slurm'
        }
        result.update(config)

    elif location == 'max-cfel':
        config = {
            'qcommand': 'slurm'
        }
        result.update(config)

    elif  location=='CFEL':
        config = {
            'qcommand' : 'slurm'
        }
        result.update(config)


    elif  location=='euXFEL':
        config = {
            'qcommand' : 'slurm'
        }
        result.update(config)

    else:
        default = {
            'qcommand' : ' '
        }
        result.update(default)


    # Add location to the dictionary for completeness
    result.update({'location' : location})

    # Return
    return result


