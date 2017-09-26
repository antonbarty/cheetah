//
//  agipd_calibrator.cpp
//  agipd
//
//  Created by Helen Ginn on 17/09/2017.
//

#include "agipd_calibrator.h"
#include <iostream>

// A few constants...
const int cAgipdCalibrator::nGains = 3;
int cAgipdCalibrator::nCells = 30;


// Constructor
cAgipdCalibrator::cAgipdCalibrator()
{
	_allData = NULL;
	_myModule = NULL;
	_gainCellPtrs = 0;
}

// Constructor with arguments
cAgipdCalibrator::cAgipdCalibrator(std::string filename, cAgipdModuleReader &reader)
{
	_filename = filename;
	_allData = NULL;
	_myModule = &reader; // pointer should not jump
	_gainCellPtrs = 0;
}

// Destructor
cAgipdCalibrator::~cAgipdCalibrator()
{
	free(_allData);
	_gainCellPtrs = 0;
}


/*
 *	New version of the open calibrations function
 *	Changed:
 *		Reads gain threshold and other info from calibration file (in addition to offsets)
 *		Determines nCells from the size of the array (rather than hard coded)
 *		Uses new checkAllocReadDataset(..) function, which is less cumbersome then checkAllocReadHyperslab(..)
 *		void* cHDF5Functions::checkAllocReadDataset(char fieldName[], int *h5_ndims, hsize_t *h5_dims, hid_t h5_type_id, size_t targetsize)
 */
void cAgipdCalibrator::open()
{
	std::cout << "Opening darkcal file..." << std::endl;
	
	// Check opening of file
	
	bool check;
	check = fileCheckAndOpen((char *)_filename.c_str());
	if (check) {
		std::cout << "\tFile check OK\n";
	}
	else {
		std::cout << "\tFile check Bad - keep going and see what happens\n";
	}
	
	// 	Data field for offsets
	std::string offset_field = "/offset";
	std::string gainthresh_field = "/threshold";
	
	
	// 	New reading function
	//	Offsets are currently:  3 x 30 x 512 x 128 of type int16_t
	if(_allData != NULL)
		free(_allData);		// Beware memory leaks
	
	int	ndims;
	hsize_t		dims[4];
	_allData = (int16_t*) checkAllocReadDataset((char*) offset_field.c_str(), &ndims, dims, H5T_STD_I16LE, sizeof(int16_t));
	
	
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

	
	// Housekeeping
	nCells = (int) dims[1];

	
	// Pointers to the gain cells (so that we can access them quickly)
	_gainCellPtrs = (int16_t ***)malloc(sizeof(int16_t **) * nGains);
	for (int i = 0; i < nGains; i++) {
		long offset1 = i * nCells * _myModule->nn;
		
		_gainCellPtrs[i] = (int16_t **)malloc(sizeof(int16_t **) * nCells);
		
		for (int j = 0; j < nCells; j++) {
			long offset2 = j * _myModule->nn;
			
			_gainCellPtrs[i][j] = &_allData[offset1 + offset2];
		}
	}
	
	// This is to show the data makes sense
	int16_t *offset = gainAndCellPtr(0, 0);
	std::cout << "First few offsets of gain 0 / cell 0: ";
	for (int i = 0; i < 5; i++) {
		std::cout << offset[i] << ", ";
	}
	std::cout << std::endl;
}




/*
 *	Parked version of the original open calibrations function
 *	This version works but only reads the dark offsets.
 *	We want to replace it with something that has gain thresholds and the rest as well
 *	Park this in case we need to rever to it in a hurry
 */
void cAgipdCalibrator::open_version1()
{
	std::cout << "Opening darkcal file..." << std::endl;

	// Check opening of file
	
	bool check;
    check = fileCheckAndOpen((char *)_filename.c_str());
	if (check) {
		std::cout << "\tFile check OK\n";
	}
	else {
		std::cout << "\tFile check Bad - let's see what happens\n";
	}

	// Data field
	std::string field = "/offset";

	hsize_t     slab_start[4];
	hsize_t		slab_size[4];
	slab_start[0] = 0;
	slab_start[1] = 0;
	slab_start[2] = 0;
	slab_start[3] = 0;
	slab_size[0] = nGains;
	slab_size[1] = nCells;
	slab_size[2] = _myModule->n1;
	slab_size[3] = _myModule->n0;

	int ndims = 4;
	hid_t type = H5T_STD_I16LE;
	hid_t size = sizeof(int16_t);

	_allData = (int16_t*) checkAllocReadHyperslab((char *)field.c_str(), ndims,
												  slab_start, slab_size, type, size);

	_gainCellPtrs = (int16_t ***)malloc(sizeof(int16_t **) * nGains);

	for (int i = 0; i < nGains; i++)
	{
		long offset1 = i * nCells * _myModule->nn;

		_gainCellPtrs[i] = (int16_t **)malloc(sizeof(int16_t **) * nCells);

		for (int j = 0; j < nCells; j++)
		{
			long offset2 = j * _myModule->nn;

			_gainCellPtrs[i][j] = &_allData[offset1 + offset2];
		}
	}

	int16_t *offset = gainAndCellPtr(0, 0);

	std::cout << "First few offsets of gain 0 / cell 0: ";
	for (int i = 0; i < 5; i++)
	{
		std::cout << offset[i] << ", ";
	}

	std::cout << std::endl;
}

int16_t *cAgipdCalibrator::gainAndCellPtr(int gain, int cell)
{
	if (gain >= nGains) return NULL;
	if (cell >= nCells) return NULL;

	return _gainCellPtrs[gain][cell];
}
