//
//  Created by Anton Barty on 24/8/17.
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

	void setFirstPulse(int pulse) { _firstPulseId = pulse; if (_firstPulseId < 0) _firstPulseId = 0; }
    void setStride(int stride) { _stride = stride; if (_stride <= 0) _stride = 1; }
	void setPulseIDmodulo(int mod) { _pulseIDmodulo = mod; if (_pulseIDmodulo <= 0) _pulseIDmodulo = 1; }
	void setCellIDcorrection(int mod) { _cellIDcorrection = mod; if (_cellIDcorrection <= 0) _cellIDcorrection = 1; }
    void setNewFileSkip(int skip) { _newFileSkip = skip; if (_newFileSkip < 0) _newFileSkip = 0; }
	void setGainDataOffset(int d0, int d1) {_gainDataOffset[0] = d0; _gainDataOffset[1] = d1; }
    void setDetectorString(std::string detName) {_detName = detName;}
	
	
	void setDoNotApplyGainSwitch(bool _val) {_doNotApplyGainSwitch = _val; }

	

    
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
	uint16_t	*badpixMask;
	
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
    std::string         _scheme;            // Scheme to use
    std::string         _detName = "SPB_DET_AGIPD1M-1";           // Detector name string, by default set this to SPB
	int                 _firstPulseId;      // Number of pulses to skip at start of pulse train
	int					_pulseIDmodulo;		// Good frames occur when pulseID % _pulseIDmodulo == 0
	int					_cellIDcorrection; 	// For interleaved data we need to divide cellID by 2, otherwise not
    int                 _stride;            // Step over frames with this spacing
    int                 _newFileSkip;       // Number of useless frames at the start of each file
    int                 _referenceModule;   // The module number passed on the command line (evidently it exists)
	int					_gainDataOffset[2];	// Gain data hyperslab offset relative to image data frame
	bool				_doNotApplyGainSwitch;		// Bypass gain switching


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
