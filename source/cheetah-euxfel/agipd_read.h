//
//  Created by Anton Barty on 24/8/17.
//	Distributed under the GPLv3 license
//

#ifndef agipd_h5_hpp
#define agipd_h5_hpp

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




/*
 *	This class reads and assembles all 16 AGIPD modules into one array for one event
 */
class cAgipdReader {

public:
	static const int nAGIPDmodules = 16;


public:
	cAgipdReader();
	~cAgipdReader();
	
	void open(char[]);
	void close(void);
	bool readFrame(long trainID, long pulseID);
	bool nextFrame();
	void resetCurrentFrame();
	bool goodFrame() { return (lastModule >= 0); }

	void maxAllFrames();
	float *getCellAverage(int i);
	void writePNG(float *pngData, std::string filename);

	void setSkip(int skip) { _skip = skip; if (_skip <= 0) _skip = 1; }

public:
	// Data slab dimensions
	long		nframes;

	bool        firstModule;
	long        currentTrain;
	long        currentPulse;
	uint16_t	currentCell;
	
	// Calibration files (set to "No_file_specified" in constructor)
	std::string	gaincalFile;
	std::string	darkcalFile;
	

	// Dimensions and of the composite data slab
	long		dims[2];
	long		n0;
	long		n1;
	long		nn;
	float    	*data;
	uint16_t	*digitalGain;
	uint16_t	*mask;
	
	// Metadata for this event
	uint16_t	cellID[nAGIPDmodules];
	uint16_t	statusID[nAGIPDmodules];
	
	// Module dimensions (how modules are placed in the 2D slab of data)
	long		nmodules[2];
	long		modulen[2];
	long		modulenn;
	
	// Pointer to the data location for each module for easy memcpy()
	// and those who want to look at the data as a stack of panels
	float*  	pdata[nAGIPDmodules];
	uint16_t*	pgain[nAGIPDmodules];
	uint16_t*	pmask[nAGIPDmodules];

	int			verbose;
	bool		rawDetectorData;

	// Individual cells - means for each of the ~30 or whatever cells held here.
	// Dynamically allocated when number of cells is known

	float **    cellAveData;
	int **      cellAveCounts;

private:
	void				generateModuleFilenames(char[]);

	long				currentFrame;
	std::string			moduleFilename[nAGIPDmodules];
	cAgipdModuleReader	module[nAGIPDmodules];
	bool				moduleOK[nAGIPDmodules];

	std::string			darkcalFilename[nAGIPDmodules];
	std::string			gaincalFilename[nAGIPDmodules];
	
	int                 _skip;

	/* Housekeeping for trains and pulses */
	long                minTrain;
	long                maxTrain;
	long                minPulse;
	long                maxPulse;
	long                minCell;
	long                maxCell;

	/* Stores last module added to image, or -1 if no modules available */
	int                 lastModule;
	/* Number of non-empty images in the current train */
	int                 goodImages4ThisTrain;

	TrainPulseMap       trainPulseMap;

	bool nextFramePrivate();
};


#endif /* agipd_h5_hpp */
