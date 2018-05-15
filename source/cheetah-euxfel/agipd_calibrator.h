//
//  agipd_calibrator.h
//  agipd
//
//  Created by Helen Ginn on 17/09/2017.
//  Coded by Helen Ginn and Anton Barty
//  Distributed under the GPLv3 license
//
//  Created and funded as a part of academic research; proper academic attribution expected.
//  Feel free to reuse or modify this code under the GPLv3 license, but please ensure that users cite the following paper reference:
//  Barty, A. et al. "Cheetah: software for high-throughput reduction and analysis of serial femtosecond X-ray diffraction data."
//  J Appl Crystallogr 47, 1118–1131 (2014)
//
//  The above statement may not be modified except by permission of the original authors listed above
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
	~cAgipdCalibrator();
	
	void readCalibrationData();
    void readDESYCalibrationData();
	void applyCalibration(int, float*, uint16_t*, uint16_t*);


	int16_t *darkOffsetForGainAndCell(int gain, int cell);
	int16_t *gainLevelForGainAndCell(int gain, int cell);
    uint8_t *badpixelForGainAndCell(int gain, int cell);
    float   *relativeGainForGainAndCell(int gain, int cell);
	


private:
	std::string _filename;
    bool _calibrationLoaded;
	const static int nGains;
	static int nCells;

	
	cAgipdModuleReader *_myModule;
	int16_t *_darkOffsetData;
	int16_t *_gainLevelData;
    uint8_t *_badpixelData;
    float   *_relativeGainData;
    

	int16_t ***_darkOffsetGainCellPtr;
	int16_t ***_gainLevelGainCellPtr;
    uint8_t ***_badpixelGainCellPtr;
    float ***_relativeGainGainCellPtr;
};

#endif /* defined(__agipd__agipd_calibrator__) */
