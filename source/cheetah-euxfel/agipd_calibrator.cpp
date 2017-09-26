//
//  agipd_calibrator.cpp
//  agipd
//
//  Created by Helen Ginn on 17/09/2017.
//

#include "agipd_calibrator.h"
#include <iostream>

const int cAgipdCalibrator::nGains = 3;
const int cAgipdCalibrator::nCells = 30;

cAgipdCalibrator::cAgipdCalibrator()
{
	_allData = NULL;
	_myModule = NULL;
	_gainCellPtrs = 0;
}

cAgipdCalibrator::cAgipdCalibrator(std::string filename, cAgipdModuleReader &reader)
{
	_filename = filename;
	_allData = NULL;
	_myModule = &reader; // pointer should not jump
	_gainCellPtrs = 0;
}

void cAgipdCalibrator::open()
{
	std::cout << "Opening darkcal file..." << std::endl;
    bool check;
    check = fileCheckAndOpen((char *)_filename.c_str());

	if (check) {
		std::cout << "\tFile check OK\n";
	}

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
