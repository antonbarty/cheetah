//
//  agipd_calibrator.h
//  agipd
//
//  Created by Helen Ginn on 17/09/2017.
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
	void applyCalibration(int, float*, uint16_t*);


	int16_t *darkOffsetForGainAndCell(int gain, int cell);
	int16_t *gainLevelForGainAndCell(int gain, int cell);
    uint8_t *badpixelForGainAndCell(int gain, int cell);
    float   *relativeGainForGainAndCell(int gain, int cell);
	
	//void setDoNotApplyGainSwitch(bool _val) {_doNotApplyGainSwitch = _val; }


private:
	std::string _filename;
	const static int nGains;
	static int nCells;

	//bool		_doNotApplyGainSwitch;		// Bypass gain switching
	
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
