//
//  agipd_calibrator.cpp
//  agipd
//
//  Created by Helen Ginn on 17/09/2017.
//

// checkAllocRead the dark setting field for this module into an array....
//	[max-exfl014:barty]user/kuhnm/dark> h5ls -r dark_AGIPD00_agipd_2017-09-16.h5
//	/                        Group
//	/offset                  Dataset {3, 30, 128, 512}
//	/stddev                  Dataset {3, 30, 128, 512}
//	/threshold               Dataset {2, 30, 128, 512}



#include "agipd_calibrator.h"
#include <iostream>

// A few constants...
const int cAgipdCalibrator::nGains = 3;
int cAgipdCalibrator::nCells = 30;


// Constructor
cAgipdCalibrator::cAgipdCalibrator()
{
	_darkOffsetData = NULL;
	_gainLevelData = NULL;
    _badpixelData = NULL;
    _relativeGainData = NULL;
	_myModule = NULL;
	_darkOffsetGainCellPtr = NULL;
	_gainLevelGainCellPtr = NULL;
    _badpixelGainCellPtr = NULL;
    _relativeGainGainCellPtr = NULL;
}

// Constructor with arguments
cAgipdCalibrator::cAgipdCalibrator(std::string filename, cAgipdModuleReader &reader)
{
	_filename = filename;
	_darkOffsetData = NULL;
	_gainLevelData = NULL;
    _badpixelData = NULL;
    _relativeGainData = NULL;
	_myModule = &reader; // pointer should not jump
	_darkOffsetGainCellPtr = NULL;
	_gainLevelGainCellPtr = NULL;
    _badpixelGainCellPtr = NULL;
    _relativeGainGainCellPtr = NULL;
}

// Destructor
cAgipdCalibrator::~cAgipdCalibrator()
{
	free(_darkOffsetData);
	free(_gainLevelData);
    free(_badpixelData);
    free(_relativeGainData);
    _darkOffsetData = NULL;
    _gainLevelData = NULL;
    _badpixelData = NULL;
    _relativeGainData = NULL;
    
    free(_darkOffsetGainCellPtr);
    free(_gainLevelGainCellPtr);
    free(_badpixelGainCellPtr);
    free(_relativeGainGainCellPtr);
	_darkOffsetGainCellPtr = NULL;
	_gainLevelGainCellPtr = NULL;
    _badpixelGainCellPtr = NULL;
    _relativeGainGainCellPtr = NULL;
}


/*
 *	Apply AGIPD calibration
 *	Overwrites contents of aduData with calibrated value
 *	Overwrites contents of gainData with determined gain stage
 *  Sets bad pixel mask to 1 if a bad pixel is encountered
 */
void cAgipdCalibrator::applyCalibration(int cellID, float *aduData, uint16_t *gainData, uint16_t *badpixMask) {

	// Stupidity checks
    if (cellID >= nCells || cellID < 0) {
        std::cout << "WARNING: Out of bounds cellID in cAgipdCalibrator::applyCalibration: " << cellID << std::endl;
		return;
    }

    if (_darkOffsetData == NULL) {
        std::cout << "WARNING: No _darkOffsetData in cAgipdCalibrator::applyCalibration for module " << std::endl;
		return;
    }
    
    if (aduData == NULL || gainData == NULL) {
        std::cout << "WARNING: No aduData or no gainData in cAgipdCalibrator::applyCalibration" << std::endl;
        return;
    }

	// Extract pointers to data for this cell
	int16_t 	*cellDarkOffset[nGains];
    int16_t     *cellGainLevel[nGains];
    float       *cellRelativeGain[nGains];
    uint8_t     *cellBadpix[nGains];
	for (int g = 0; g<nGains; g++)
	{
		cellDarkOffset[g] = darkOffsetForGainAndCell(g, cellID);
        cellGainLevel[g] = gainLevelForGainAndCell(g, cellID);
        cellRelativeGain[g] = relativeGainForGainAndCell(g, cellID);
        cellBadpix[g] = badpixelForGainAndCell(g, cellID);
	}
	
    
    //if(true)
    if(_myModule->_doNotApplyGainSwitch)
    {
        // Option: bypass multi-gain calibration
        // In this case use only the gain0 offset
		for (long p=0; p<_myModule->nn; p++) {
			aduData[p] -= cellDarkOffset[0][p];
            gainData[p] = 0;
            badpixMask[p] = 0;
            if(cellBadpix[0][p] != 0) {
                aduData[p] = 0;
                badpixMask[p] = 1;
            }
		}
		// Bypass the gain calibration stage
		return;
	}
	

	int		pixGain = 0;
	long	pixelsInGainLevel[3] = {0,0,0};
	
    // Loop through pixels
	for (long p=0; p<_myModule->nn; p++) {
        pixGain = 0;

		// Determine which gain stage by thresholding
        if(true) {
            if(gainData[p] > cellGainLevel[1][p])
            {
                pixGain = 1;
                pixelsInGainLevel[1] += 1;
            }
            // Thresholding for gain level 3 is dodgy - thresholds merge
            // Ignore gain level 3 for now and work on levels 0,1
            //if(gainData[p] > cellGainLevel[2][p])
            //{
            //    pixGain = 2;
            //    pixelsInGainLevel[2] += 1;
            //}
            else {
                pixGain = 0;
                pixelsInGainLevel[0] += 1;
            }
        }
        gainData[p] = pixGain;

        // Remember the gain level setting

        
        // Check whether this ia a bad pixel
        if(cellBadpix[pixGain][p] != 0) {
            badpixMask[p]= 1;
            aduData[p] = 0;
            continue;
        }
        

        // Subtract the appropriate offset
		aduData[p] -= cellDarkOffset[pixGain][p];		// Correct way
		//aduData[p] -= cellDarkOffset[0][p];		    // For testing

        
		// Apply gain factor
		aduData[p] *= cellRelativeGain[pixGain][p];
	}

	// 	This is verbose but sometimes useful
	if(false) {
        pixelsInGainLevel[0] = _myModule->nn - pixelsInGainLevel[1] - pixelsInGainLevel[2];
		std::cout << "nPixels in gain mode (0,1,2) = (";
		std::cout << pixelsInGainLevel[0] << ", ";
		std::cout << pixelsInGainLevel[1] << ", ";
		std::cout << pixelsInGainLevel[2] << ")" << std::endl;
	}
}




/*
 *    Read in calibration constants needed by Cheetah
 *      h5_putdata, outfile, 'Offset', offset_out
 *      h5_putdata, outfile, 'RelativeGain', gain_out
 *      h5_putdata, outfile, 'DigitalGainLevel', thresh_out
 *      h5_putdata, outfile, 'Badpixel', badpix_out
*/
void cAgipdCalibrator::readCalibrationData()
{
    std::cout << "Opening AGIPD Cheetah calibration file..." << std::endl;
    
    // Anton's AGIPD calibration format
    //> h5ls calib/agipd/Cheetah-AGIPD00-calib.h5
    //AnalogOffset             Dataset {3, 32, 512, 128} H5T_STD_I16LE
    //Badpixel                 Dataset {3, 32, 512, 128} H5T_STD_U8LE
    //DigitalGainLevel         Dataset {3, 32, 512, 128} H5T_STD_U16LE
    //RelativeGain             Dataset {3, 32, 512, 128} H5T_IEEE_F32LE
    std::string badpix_field = "/Badpixel";
    std::string offset_field = "/AnalogOffset";
    std::string gainlevel_field = "/DigitalGainLevel";
    std::string relativegain_field = "/RelativeGain";
    
    
    // Check opening of file
    bool check;
    check = fileCheckAndOpen((char *)_filename.c_str());
    if (check) {
        std::cout << "\tFile check OK\n";
    }
    else {
        std::cout << "\tFile check Bad - keep going and see what happens\n";
    }
    
    int    ndims;
    hsize_t dims[4];

    
    // Read offsets
    // Currently:  3 x 30 x 512 x 128 of type int16_t
    if(_darkOffsetData != NULL)        // Beware of memory leaks
        free(_darkOffsetData);
    _darkOffsetData = (int16_t*) checkAllocReadDataset((char*) offset_field.c_str(), &ndims, dims, H5T_STD_I16LE, sizeof(int16_t));
    
    // Housekeeping
    nCells = (int) dims[1];
    
    // Sanity checks
    // All other fields are likely to be identical so only do this for the offsets
    if(ndims != 4) {
        std::cout << "Error in shape of AnalogOffset: expect ndims=4; but ndims=" << ndims << std::endl;
    }
    if(dims[0] != nGains) {
        std::cout << "Error in AnalogOffset number of gain stages: expect nGains=" << nGains << "; but nGains=" << dims[0] << std::endl;
    }
    if(dims[1] < 0 || dims[1] > 128) {
        std::cout << "Odd: Suspiciouly large number of memory cells: " << dims[1] << std::endl;
    }
    if(dims[2] != _myModule->n1 || dims[3] != _myModule->n0) {
        std::cout << "Error: Offset array does not match module size: " << std::endl;
        std::cout << "dims[2,3] = " << dims[2] << "," << dims[3] << ";  ";
        std::cout << "module = " << _myModule->n1 << "," << _myModule->n0 << std::endl;
    }
    // Skip dimension checks from now on as all other array dimensions are very likely the same

    
    // Read digital channel gain level data
    if(_gainLevelData != NULL)        // Beware of memory leaks
        free(_gainLevelData);
    _gainLevelData = (int16_t*) checkAllocReadDataset((char*) gainlevel_field.c_str(), &ndims, dims, H5T_STD_I16LE, sizeof(int16_t));
	if(ndims != 4) {
		std::cout << "Error in shape of DigitalGainLevel: expect ndims=4; but ndims=" << ndims << std::endl;
		exit(1);
	}
	if(dims[0] != nGains) {
		std::cout << "Error in number of gain stages for DigitalGainLevel: expect nGains=" << nGains << "; but nGains=" << dims[0] << std::endl;
		exit(1);
	}

    
    // Read relative gain data
    if(_relativeGainData != NULL)        // Beware of memory leaks
        free(_relativeGainData);
    _relativeGainData = (float*) checkAllocReadDataset((char*) relativegain_field.c_str(), &ndims, dims, H5T_IEEE_F32LE, sizeof(float));

    
    // Read bad pixel map
    if(_badpixelData != NULL)        // Beware of memory leaks
        free(_badpixelData);
    _badpixelData = (uint8_t*) checkAllocReadDataset((char*) badpix_field.c_str(), &ndims, dims, H5T_STD_U8LE, sizeof(uint8_t));

    
    
    // Create pointers to the gain cells (so that we can access them quickly)
    _darkOffsetGainCellPtr = (int16_t ***)malloc(sizeof(int16_t **) * nGains);
    _gainLevelGainCellPtr = (int16_t ***)malloc(sizeof(int16_t **) * nGains);
    _badpixelGainCellPtr = (uint8_t ***)malloc(sizeof(uint8_t **) * nGains);
    _relativeGainGainCellPtr = (float ***)malloc(sizeof(float **) * nGains);
    
    for (int g = 0; g < nGains; g++) {
        long offset1 = g * nCells * _myModule->nn;
        _darkOffsetGainCellPtr[g] = (int16_t **)malloc(nCells * sizeof(int16_t **));
        _gainLevelGainCellPtr[g] = (int16_t **)malloc(nCells * sizeof(int16_t **));
        _badpixelGainCellPtr[g] = (uint8_t **)malloc(nCells * sizeof(uint8_t **));
        _relativeGainGainCellPtr[g] = (float **)malloc(nCells * sizeof(float **));
        
        for (int c = 0; c < nCells; c++) {
            long offset2 = c * _myModule->nn;
            _darkOffsetGainCellPtr[g][c] = &_darkOffsetData[offset1 + offset2];
            _gainLevelGainCellPtr[g][c] = &_gainLevelData[offset1 + offset2];
            _badpixelGainCellPtr[g][c] = &_badpixelData[offset1 + offset2];
            _relativeGainGainCellPtr[g][c] = &_relativeGainData[offset1 + offset2];
        }
    }
    
    
    // This is to show the data makes sense
    std::cout << "First few dark offsets of gain 0 / cell 0: ";
    int16_t *offset = darkOffsetForGainAndCell(0, 0);
    for (int i = 0; i < 5; i++) {
        std::cout << offset[i] << ", ";
    }
    std::cout << std::endl;
    
    std::cout << "First few digital gain levels for gain 1 / cell 0: ";
    int16_t *gaint = gainLevelForGainAndCell(1, 0);
    for (int i = 0; i < 5; i++) {
        std::cout << gaint[i] << ", ";
    }
    std::cout << std::endl;
}



/*
 *	Read in calibration data from files provided by Manuela
 *  This function is currently not called but parked here in case needed again in the future.
 */
void cAgipdCalibrator::readDESYCalibrationData()
{
	std::cout << "Opening DESY-style calibration file..." << std::endl;
	
	// 	Data field for calibration constants
	std::string offset_field = "/offset";
	std::string gainthresh_field = "/threshold";
	
    
    // Anton's format
    //> h5ls calib/agipd/Cheetah-AGIPD00-calib.h5
    //Badpixel                 Dataset {3, 32, 512, 128}
    //DigitalGainThreshold     Dataset {2, 32, 512, 128}
    //Offset                   Dataset {3, 32, 512, 128}
    //RelativeGain             Dataset {3, 32, 512, 128}
    

	// Check opening of file
	bool check;
	check = fileCheckAndOpen((char *)_filename.c_str());
	if (check) {
		std::cout << "\tFile check OK\n";
	}
	else {
		std::cout << "\tFile check Bad - keep going and see what happens\n";
	}
	
	
	// 	New reading function
	//	Offsets are currently:  3 x 30 x 512 x 128 of type int16_t
	int	ndims;
	hsize_t		dims[4];

	if(_darkOffsetData != NULL)		// Beware of memory leaks
		free(_darkOffsetData);
	_darkOffsetData = (int16_t*) checkAllocReadDataset((char*) offset_field.c_str(), &ndims, dims, H5T_STD_I16LE, sizeof(int16_t));
	
	
	// Sanity checks (only do this for the offsets, as all other fields are likely to be identical)
	if(ndims != 4) {
		std::cout << "Error in shape: expect ndims=4; but ndims=" << ndims << std::endl;
	}
	if(dims[0] != nGains) {
		std::cout << "Error in number of gain stages: expect nGains=" << nGains << "; but nGains=" << dims[0] << std::endl;
	}
	if(dims[1] > 128) {
		std::cout << "Odd: Suspiciouly large number of memory cells: " << dims[1] << std::endl;
	}
	if(dims[2] != _myModule->n1 || dims[3] != _myModule->n0) {
		std::cout << "Error: Offset array does not match module size: " << std::endl;
		std::cout << "dims[2,3] = " << dims[2] << "," << dims[3] << ";  ";
		std::cout << "module = " << _myModule->n1 << "," << _myModule->n0 << std::endl;
	}

	
	// Do the same for reading gain threshold data
	// Skip checks as it is very likely array dimensions are the same
	if(_gainLevelData != NULL)		// Beware of memory leaks
		free(_gainLevelData);
	_gainLevelData = (int16_t*) checkAllocReadDataset((char*) gainthresh_field.c_str(), &ndims, dims, H5T_STD_I16LE, sizeof(int16_t));

	
	
	// Housekeeping
	nCells = (int) dims[1];

	
	// Pointers to the gain cells (so that we can access them quickly)
	_darkOffsetGainCellPtr = (int16_t ***)malloc(sizeof(int16_t **) * nGains);
	_gainLevelGainCellPtr = (int16_t ***)malloc(sizeof(int16_t **) * nGains);
	for (int i = 0; i < nGains; i++) {
		long offset1 = i * nCells * _myModule->nn;
		
		_darkOffsetGainCellPtr[i] = (int16_t **)malloc(nCells * sizeof(int16_t **));
		_gainLevelGainCellPtr[i] = (int16_t **)malloc(nCells * sizeof(int16_t **));

		for (int j = 0; j < nCells; j++) {
			long offset2 = j * _myModule->nn;
			
			_darkOffsetGainCellPtr[i][j] = &_darkOffsetData[offset1 + offset2];
			_gainLevelGainCellPtr[i][j] = &_gainLevelData[offset1 + offset2];
		}
	}
	
	// This is to show the data makes sense
	int16_t *offset = darkOffsetForGainAndCell(0, 0);
	int16_t *gaint = gainLevelForGainAndCell(0, 0);
	std::cout << "First few dark offsets of gain 0 / cell 0: ";
	for (int i = 0; i < 5; i++) {
		std::cout << offset[i] << ", ";
	}
	std::cout << std::endl;

	std::cout << "First few gain thresholds of gain 0 / cell 0: ";
	for (int i = 0; i < 5; i++) {
		std::cout << gaint[i] << ", ";
	}
	std::cout << std::endl;
}



/*
 *  Set of funcitons for returning shortcut pointer to location of calibration data
 *  Does some simple error checking
 */
// Return pointer to dark offsets for specified gain and cellID
int16_t *cAgipdCalibrator::darkOffsetForGainAndCell(int gain, int cell)
{
	if (gain >= nGains) return NULL;
	if (cell >= nCells) return NULL;

	return _darkOffsetGainCellPtr[gain][cell];
}

// Return pointer to gain thresholds for specified gain and cellID
int16_t *cAgipdCalibrator::gainLevelForGainAndCell(int gain, int cell)
{
	if (gain >= nGains) return NULL;
	if (cell >= nCells) return NULL;
	
	return _gainLevelGainCellPtr[gain][cell];
}


// Return pointer to relative gain for specified gain and cellID
float *cAgipdCalibrator::relativeGainForGainAndCell(int gain, int cell)
{
    if (gain >= nGains) return NULL;
    if (cell >= nCells) return NULL;
    
    return _relativeGainGainCellPtr[gain][cell];
}

// Return pointer to relative gain for specified gain and cellID
uint8_t *cAgipdCalibrator::badpixelForGainAndCell(int gain, int cell)
{
    if (gain >= nGains) return NULL;
    if (cell >= nCells) return NULL;
    
    return _badpixelGainCellPtr[gain][cell];
}


