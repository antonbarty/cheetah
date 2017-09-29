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
	_gainThresholdData = NULL;
	_myModule = NULL;
	_darkOffsetGainCellPtr = NULL;
	_gainThresholdGainCellPtr = NULL;
	//_doNotApplyGainSwitch = false;
}

// Constructor with arguments
cAgipdCalibrator::cAgipdCalibrator(std::string filename, cAgipdModuleReader &reader)
{
	_filename = filename;
	_darkOffsetData = NULL;
	_gainThresholdData = NULL;
	_myModule = &reader; // pointer should not jump
	_darkOffsetGainCellPtr = NULL;
	_gainThresholdGainCellPtr = NULL;
	//_doNotApplyGainSwitch = false;
}

// Destructor
cAgipdCalibrator::~cAgipdCalibrator()
{
	free(_darkOffsetData);
	free(_gainThresholdData);
	_darkOffsetGainCellPtr = NULL;
	_gainThresholdGainCellPtr = NULL;
}


/*
 *	Apply AGIPD calibration
 *	Overwrites contents of aduData with calibrated value
 *	Overwrites contents of gainData with determined gain stage
 */
void cAgipdCalibrator::applyCalibration(int cellID, float *aduData, uint16_t *gainData) {

	// Stupidity checks
	if (cellID >= nCells || cellID < 0)
		return;
	if(_darkOffsetData==NULL)
		return;
	if(aduData==NULL || gainData==NULL)
		return;
	
	
	// Extract pointer to dark offsets for this cell
	int16_t 	*cellDarkOffset[nGains];
	for(int i=0; i<nGains; i++) {
		cellDarkOffset[i] = darkOffsetForGainAndCell(i, cellID);
	}
	
	// Extract pointer to gain thresholds for this cell
	int16_t 	*cellGainThreshold[nGains-1];
	for(int i=0; i<nGains-1; i++) {
		cellGainThreshold[i] = gainThresholdForGainAndCell(i, cellID);
	}

	// For now do gain=0 (test if we get the same results as before)
	//	int16_t *offset = darkOffsetForGainAndCell(pixGainLevel, cellID);
	//	if(offset == NULL)
	//		return;
	

	// Simple switch for using only the gain0 offset, bypassing multi-gain calibration
	if(_myModule->_doNotApplyGainSwitch) {
		for (long p=0; p<_myModule->nn; p++) {
			aduData[p] -= cellDarkOffset[0][p];
		}
		// Bypass the gain calibration stage
		return;
	}
	

	// Loop through pixels, determine gain and apply offsets
	// 	calibrated_raw_data[g1] *= 45
	//	calibrated_raw_data[g2] *= 454.2
	int		pixGain = 0;
	long	pixelsInGainLevel[3] = {0,0,0};
	float	gainFactor[] = {1, 45, 454.2};
	
	for (long p=0; p<_myModule->nn; p++) {
		
		// Determine which gain stage by thresholding
		pixGain = 0;
		if(gainData[p] > cellGainThreshold[0][p]) {
			pixGain = 1;
			pixelsInGainLevel[1] += 1;
		}
		if(gainData[p] > cellGainThreshold[1][p]) {
			pixGain = 2;
			pixelsInGainLevel[2] += 1;
		}
		
		// Subtract the appropriate offset
		aduData[p] -= cellDarkOffset[pixGain][p];

		// Multiplication factor
		aduData[p] *= gainFactor[pixGain];
		
		// Remember the gain level setting
		gainData[p] = pixGain;
	}
	pixelsInGainLevel[0] = _myModule->nn - pixelsInGainLevel[1] - pixelsInGainLevel[2];

	// 	This is verbose but useful
	//	Perhaps pass back to agipd_reader and print once per frame rather than once per module
	if(true) {
		std::cout << "nPixels in gain mode (0,1,2) = (";
		std::cout << pixelsInGainLevel[0] << ", ";
		std::cout << pixelsInGainLevel[1] << ", ";
		std::cout << pixelsInGainLevel[2] << ")" << std::endl;
	}
}



/*
 *	New version of the open calibrations function
 *	Changed:
 *		Reads gain threshold and other info from calibration file (in addition to offsets)
 *		Determines nCells from the size of the array (rather than hard coded)
 *		Uses new checkAllocReadDataset(..) function, which is less cumbersome then checkAllocReadHyperslab(..)
 *		void* cHDF5Functions::checkAllocReadDataset(char fieldName[], int *h5_ndims, hsize_t *h5_dims, hid_t h5_type_id, size_t targetsize)
 */
void cAgipdCalibrator::readCalibrationData()
{
	std::cout << "Opening darkcal file..." << std::endl;
	
	// 	Data field for calibration constants
	std::string offset_field = "/offset";
	std::string gainthresh_field = "/threshold";
	

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
	if(dims[1] < 0 || dims[1] > 128) {
		std::cout << "Odd: Suspiciouly large number of memory cells: " << dims[1] << std::endl;
	}
	if(dims[2] != _myModule->n1 || dims[3] != _myModule->n0) {
		std::cout << "Error: Offset array does not match module size: " << std::endl;
		std::cout << "dims[2,3] = " << dims[2] << "," << dims[3] << ";  ";
		std::cout << "module = " << _myModule->n1 << "," << _myModule->n0 << std::endl;
	}

	
	// Do the same for reading gain threshold data
	// Skip checks as it is very likely array dimensions are the same
	if(_gainThresholdData != NULL)		// Beware of memory leaks
		free(_gainThresholdData);
	_gainThresholdData = (int16_t*) checkAllocReadDataset((char*) gainthresh_field.c_str(), &ndims, dims, H5T_STD_I16LE, sizeof(int16_t));

	
	
	// Housekeeping
	nCells = (int) dims[1];

	
	// Pointers to the gain cells (so that we can access them quickly)
	_darkOffsetGainCellPtr = (int16_t ***)malloc(sizeof(int16_t **) * nGains);
	_gainThresholdGainCellPtr = (int16_t ***)malloc(sizeof(int16_t **) * nGains);
	for (int i = 0; i < nGains; i++) {
		long offset1 = i * nCells * _myModule->nn;
		
		_darkOffsetGainCellPtr[i] = (int16_t **)malloc(nCells * sizeof(int16_t **));
		_gainThresholdGainCellPtr[i] = (int16_t **)malloc(nCells * sizeof(int16_t **));

		for (int j = 0; j < nCells; j++) {
			long offset2 = j * _myModule->nn;
			
			_darkOffsetGainCellPtr[i][j] = &_darkOffsetData[offset1 + offset2];
			_gainThresholdGainCellPtr[i][j] = &_gainThresholdData[offset1 + offset2];
		}
	}
	
	// This is to show the data makes sense
	int16_t *offset = darkOffsetForGainAndCell(0, 0);
	int16_t *gaint = gainThresholdForGainAndCell(0, 0);
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





// Return pointer to dark offsets for specified gain and cellID
int16_t *cAgipdCalibrator::darkOffsetForGainAndCell(int gain, int cell)
{
	if (gain >= nGains) return NULL;
	if (cell >= nCells) return NULL;

	return _darkOffsetGainCellPtr[gain][cell];
}

// Return pointer to gain thresholds for specified gain and cellID
// Gain threshold array is nGains-1 in size
int16_t *cAgipdCalibrator::gainThresholdForGainAndCell(int gain, int cell)
{
	if (gain >= nGains-1) return NULL;
	if (cell >= nCells) return NULL;
	
	return _gainThresholdGainCellPtr[gain][cell];
}
