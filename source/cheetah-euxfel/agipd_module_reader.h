//
//  Created by Anton Barty on 24/8/17.
//	Distributed under the GPLv3 license
//

#ifndef agipd_module_reader_h
#define agipd_module_reader_h

#include <iostream>
#include <string>
#include <vector>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <map>
#include <sstream>
#include "hdf5_functions.h"
#include "agipd_calibrator.h"

typedef std::pair<long, int> TrainPulsePair; // pairs of trains and pulses
typedef std::pair<TrainPulsePair, int> TrainPulseModulePair; // pairs of train-pulses and module numbers
typedef std::map<TrainPulseModulePair, long> TrainPulseMap; // map of train-pulse & module number to frame num.

inline std::string i_to_str(int val)
{
	std::ostringstream ss;
	ss << val;
	std::string temp = ss.str();

	return temp;
}

inline std::string getFilename(std::string filename)
{
	size_t pos = filename.rfind("/");
	if(pos == std::string::npos)  //No path.
		return filename;

	return filename.substr(pos + 1, filename.length());
}

inline std::string getBaseFilename(std::string filename)
{
	std::string fName = getFilename(filename);
	size_t pos = fName.rfind(".");
	if(pos == std::string::npos)  //No extension.
		return fName;

	if(pos == 0)    //. is at the front. Not an extension.
		return fName;

	return fName.substr(0, pos);
}

class cAgipdCalibrator;

/*
 *	This class handles reading and writing for one AGIPD module file
 *	(each module is in a separate file)
 */
class cAgipdModuleReader : public cHDF5Functions
{
	
public:
	cAgipdModuleReader();
	~cAgipdModuleReader();

	void open(char[], int i);
	void close(void);
	void readHeaders(void);
	void readDarkcal(char[]);
	void readGaincal(char[]);
	void readImageStack(void);
	void readFrame(long);

	void setGainDataOffset(int d0, int d1) {gainDataOffset[0] = d0; gainDataOffset[1] = d1; }
	void setDoNotApplyGainSwitch(bool _val) {_doNotApplyGainSwitch = _val; }

	
// Pubic variables
public:
	long		nframes;
	long		nstack;
	long		n0;
	long		n1;
	long		nn;

	// data and ID for the last read event.  Only updated after readFrame is called! 
	float   	*data;
	uint16_t	*digitalGain;
    uint64_t	trainID;
    uint64_t	pulseID;
    uint16_t	cellID;
    uint16_t	statusID;

	// List of all IDs in file (lookup table, useful to have all in memory)
    uint64_t	*trainIDlist;
    uint64_t	*pulseIDlist;
    uint16_t	*cellIDlist;
    uint16_t	*statusIDlist;

	bool		rawDetectorData;
	int			gainDataOffset[2];	// Gain data hyperslab offset relative to image data frame

	bool		_doNotApplyGainSwitch;		// Bypass gain switching

// Private variables
private:
	std::string	filename;
	long		currentFrame;

	std::string h5_trainId_field;
	std::string h5_pulseId_field;
	std::string h5_cellId_field;
	std::string h5_image_data_field;
	std::string h5_image_status_field;
	
	std::string	darkcalFilename;
	std::string	gaincalFilename;

	cAgipdCalibrator *calibrator;
	float		*calibGainFactor;
	


// Private functions
private:
	void		readFrameRawOrCalib(long frameNum, bool isRaw);
	void		applyCalibration(long frameNum);
};




#endif /* agipd_module_reader_h */
