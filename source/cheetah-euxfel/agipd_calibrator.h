//
//  agipd_calibrator.h
//  agipd
//
//  Created by Helen Ginn on 17/09/2017.
//  Copyright (c) 2017 Anton Barty. All rights reserved.
//

#ifndef __agipd__agipd_calibrator__
#define __agipd__agipd_calibrator__

#include <stdio.h>
#include "hdf5_functions.h"
#include <string>
#include <cstdlib>
#include "agipd_module_reader.h"
#include <hdf5.h>
#include <hdf5_hl.h>

class cAgipdModuleReader; // forward decl.

class cAgipdCalibrator : public cHDF5Functions
{
public:
	cAgipdCalibrator();
	cAgipdCalibrator(std::string filename, cAgipdModuleReader &reader);
	void open();
	int16_t *gainAndCellPtr(int gain, int cell);

private:
	std::string _filename;
	const static int nGains;
	const static int nCells;

	cAgipdModuleReader *_myModule;
	int16_t *_allData;

	int16_t ***_gainCellPtrs;
};

#endif /* defined(__agipd__agipd_calibrator__) */
