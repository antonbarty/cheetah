#
# Author: Dominik Michels
# Date: August 2016
#

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

        self.a *= 10.0
        self.b *= 10.0
        self.c *= 10.0
    

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
