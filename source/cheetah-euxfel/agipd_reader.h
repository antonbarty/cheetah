//
//  Created by Anton Barty on 24/8/17.
//	Distributed under the GPLv3 license
//

#ifndef agipd_reader_h
#define agipd_reader_h

#include <iostream>
#include <string>
#include <vector>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <map>
#include <sstream>
#include "agipd_module_reader.h"
#include "hdf5_functions.h"
#include "agipd_calibrator.h"




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
    void setScheme(char[]);
	void close(void);
	bool readFrame(long trainID, long pulseID);
	bool nextFrame();
	void resetCurrentFrame();
	bool goodFrame() { return (lastModule >= 0); }

	void maxAllFrames();
	float *getCellAverage(int i);
	void writePNG(float *pngData, std::string filename);

	void setSkip(int skip) { _skip = skip; if (_skip < 0) _skip = 0; }
    void setStride(int stride) { _stride = stride; if (_stride <= 0) _stride = 1; }
    void setNewFileSkip(int skip) { _newFileSkip = skip; if (_newFileSkip < 0) _newFileSkip = 0; }

    
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
    void                setModuleToBlank(int);


	std::string			darkcalFilename[nAGIPDmodules];
	std::string			gaincalFilename[nAGIPDmodules];
	
    /* Things that change between experiments */
    std::string         _scheme;
	int                 _skip;              // Number of pulses to skip at start of pulse train
    int                 _stride;            // Step over frames with this spacing
    int                 _newFileSkip;       // Number of useless frames at the start of each file
    int                 _referenceModule;   // The module number passed on the command line (evidently it exists)

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


#endif /* agipd_reader_h */
