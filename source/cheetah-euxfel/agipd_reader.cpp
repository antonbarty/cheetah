//
//  Created by Anton Barty on 24/8/17.
//	Distributed under the GPLv3 license
//


//#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <math.h>
#include "agipd_reader.h"
#include <algorithm>

cAgipdReader::cAgipdReader(void){
	data = NULL;
	badpixMask = NULL;
	digitalGain = NULL;
	rawDetectorData = true;
	currentTrain = 0;
	currentPulse = 0;
	minTrain = 0;
	maxTrain = 0;
	minPulse = 0;
	maxPulse = 0;
	minCell = 0;
	maxCell = 0;
	verbose = 0;
	goodImages4ThisTrain = -1;
	_firstPulseId = 0;
    _stride = 1;
    _newFileSkip = 0;
	_doNotApplyGainSwitch = false;
	
	_gainDataOffset[0] = 0;
	_gainDataOffset[1] = 1;


	gaincalFile = "No_file_specified";
	darkcalFile = "No_file_specified";
	for(long i=0; i<nAGIPDmodules; i++) {
		darkcalFilename[i] = "No_file_specified";
		gaincalFilename[i] = "No_file_specified";
	}
};

cAgipdReader::~cAgipdReader()
{
	std::cout << "\tAGIPD destructor called" << std::endl;

	if(data == NULL)
		return;
	cAgipdReader::close();
};


/*
 *  Data is laid out differently for different experiments
 *  Here we set a few defaults, which can later be overridden if desired
 *
 * 	CellID reflects which AGIPD memory cell is used --> used for calibration
 *	PulseID is taken from the AGIPD firmware --> used for determining which are good data frames
 *	AGIPD running at 4MHz, XFEL at 1MHz means 4 AGIPD frames per XFEL pulse
 *	Analogue and digital channels saved sequentiually [ADADAD] means 2x count array elements between images
 *	Thus one image per 8 AGIPD PulseID counters; works for XFEL2012, XFEL2042 and XFEL2017
 */
void cAgipdReader::setScheme(char *scheme) {
    _scheme = scheme;
	
    std::cout << "****** Begin AGIPD configuration ******\n";
	std::cout << "Configuring XFEL data layout\n";

	// Barty, September 2017
	if(_scheme == "XFEL2012") {
		std::cout << "\tSetting AGIPD data scheme to XFEL2012\n";
        setFirstPulse(0);
		setPulseIDmodulo(8);		// Good frames occur when pulseID % _pulseIDmodulo == 0
		setCellIDcorrection(2);		// For interleaved data we need to correct the cellID by 2, else not
		setGainDataOffset(1,0);		// Gain data hyperslab offset relative to image data = frameNum+1
        //setNewFileSkip(60);
    }

    // Orville, September 2017
    if(_scheme == "XFEL2017") {
        std::cout << "\tSetting AGIPD data scheme to XFEL2012\n";
        setFirstPulse(0);
        setPulseIDmodulo(4);        // Good frames occur when pulseID % _pulseIDmodulo == 0
        setCellIDcorrection(2);        // For interleaved data we need to correct the cellID by 2, else not
        setGainDataOffset(1,0);        // Gain data hyperslab offset relative to image data = frameNum+1
        //setNewFileSkip(60);
    }

    
    // Fromme, November 2017
    else if(_scheme == "XFEL2066") {
		std::cout << "\tSetting AGIPD data scheme to XFEL2066\n";
        setFirstPulse(0);
		setPulseIDmodulo(4);		// Good frames occur when pulseID % _pulseIDmodulo == 0
		setCellIDcorrection(1);		// Data is not interleaved
		setGainDataOffset(0,1);		// Gain data hyperslab offset relative to image data
        //setNewFileSkip(60);
    }

    // March 2018
    else if(_scheme == "XFEL2018a") {
        std::cout << "\tSetting AGIPD data scheme to XFEL2018a\n";
        setFirstPulse(4);
        setPulseIDmodulo(4);        // Good frames occur when pulseID % _pulseIDmodulo == 0
        setCellIDcorrection(1);        // Data is not interleaved
        setGainDataOffset(0,1);        // Gain data hyperslab offset relative to image data
        setNewFileSkip(0);
    }

    
	// Default (current) scheme
	else {
		std::cout << "\tSetting AGIPD data scheme to Default\n";
		setFirstPulse(0);
		setPulseIDmodulo(8);		// Good frames occur when pulseID % _pulseIDmodulo == 0
		setCellIDcorrection(2);		// For interleaved data we need to correct the cellID by 2, else not
		setGainDataOffset(1,0);		// Gain data hyperslab offset relative to image data = frameNum+1
		//setNewFileSkip(60);
	}

	std::cout << "\tData frames on PulseId modulo: " << _pulseIDmodulo << std::endl;
    std::cout << "\tSkip pulseIDs in train less than: " << _firstPulseId << std::endl;
	std::cout << "\tCellId correction: " << _cellIDcorrection << std::endl;
	std::cout << "\tGain data offset: (" << _gainDataOffset[0] << ", " << _gainDataOffset[1] << ")" << std::endl;
    std::cout << "****** End AGIPD configuration ******\n";

	
    // Defaults will be used if none of the above are found
    return;
}



/*
 *	Create a list of filenames for all 16 AGIPD modules based on the filename for one module
 *	Small function but isolated here so it's easy to swap conventions if needed
 */
void cAgipdReader::generateModuleFilenames(char *module0filename){
	long	pos;
	char	tempstr[10];

    //  What is the number of the module passed on the command line (evidently it exists)
    //  Will compare timestamps of all other modules to this one
    std::string     referenceModuleName;
    referenceModuleName = module0filename;
    pos = referenceModuleName.find("AGIPD");
    _referenceModule = atoi(referenceModuleName.substr(pos+5,2).c_str());
    std::cout << "Using module number " << _referenceModule << " as a reference" << std::endl;
    
	// Filenames for all the other modules
	for(long i=0; i<nAGIPDmodules; i++) {
		moduleFilename[i] = module0filename;
		

		// Replace the number in AGIPD00 with 00-15
		pos = moduleFilename[i].find("AGIPD");

		if (pos == std::string::npos) {
			std::cout << "Cannot generate AGIPD module filename as I can't find the string 'AGIPD'!" << std::endl;
			continue;
		}
        
		sprintf(tempstr, "%0.2li", i);
		moduleFilename[i].replace(pos+5,2,tempstr);
		
		// Which chunk/sequence are we looking at?
		pos = moduleFilename[i].find(".h5");
		if (pos == std::string::npos || (pos + 5 < 0)) {
			std::cout << "Cannot decide if first stack - never mind" << std::endl;
			continue;
		}

		long stackPos = pos - 5; /* Position of the stack thing */


		std::string stackNum = moduleFilename[i].substr(stackPos, 5);
		//int stackInt = atoi(stackNum.c_str());

		//firstModule = (stackInt == 0);
		
		if(verbose) {
			printf("\tModule %0.2li = %s\n",i, moduleFilename[i].c_str());
		}
	}

	// Filenames for all the darkcal files
    if(darkcalFile != "No_file_specified") {
        for(long i=0; i<nAGIPDmodules; i++) {
            // Replace the AGIPD00 number with 00-15
            darkcalFilename[i] = darkcalFile;
            pos = darkcalFilename[i].find("AGIPD");
            if (pos != std::string::npos) {
                sprintf(tempstr, "%0.2li", i);
                darkcalFilename[i].replace(pos+5,2,tempstr);
            }
            std::cout << "\t\t" << darkcalFilename[i] << std::endl;
        }
    }

	// Filenames for all the gaincal files
	if(gaincalFile != "No_file_specified") {
		std::cout << "\tGain calibration files " << std::endl;
		for(long i=0; i<nAGIPDmodules; i++) {
			// Replace the AGIPD00 number with 00-15
			gaincalFilename[i] = gaincalFile;
			pos = gaincalFilename[i].find("AGIPD");
            if (pos != std::string::npos) {
                sprintf(tempstr, "%0.2li", i);
                gaincalFilename[i].replace(pos+5,2,tempstr);
            }
			std::cout << "\t\t" << gaincalFilename[i] << std::endl;
		}
	}
}



/*
 *	Open all AGIPD module files relating to the specified module
 *  and perform a bunch of sanity checks.
 *  XFEL files are not versioned so we have to make some gueses
 */
void cAgipdReader::open(char *baseFilename){
	
	// Create a list of filenames for all 16 modules
	printf("Opening all modules for %s\n", baseFilename);
	generateModuleFilenames(baseFilename);
	
	
	
	// Open all module files and read the header
	for(long i=0; i<nAGIPDmodules; i++) {
		if(verbose) {
			printf("Module %0.2li:\n", i);
		}
		module[i].verbose = 0;
		module[i].open((char *) moduleFilename[i].data(), i);
		module[i].readHeaders();
		module[i].readDarkcal((char *)darkcalFilename[i].c_str());
		module[i].setGainDataOffset(_gainDataOffset[0],_gainDataOffset[1]);
		module[i].setCellIDcorrection(_cellIDcorrection);
		//module[i].setDoNotApplyGainSwitch(_doNotApplyGainSwitch);
	}

	// Det up size and layout of the assembled data stack
	// Use module[0] as the reference and stack the modules one on top of another
	nframes = module[_referenceModule].nframes;
	modulen[0] = module[_referenceModule].n0;
	modulen[1] = module[_referenceModule].n1;
	modulenn = modulen[0]*modulen[1];
	nmodules[0] = 1;
	nmodules[1] = nAGIPDmodules;
	dims[0] = modulen[0]*nmodules[0];
	dims[1] = modulen[1]*nmodules[1];
	n0 = dims[0];
	n1 = dims[1];
	nn = n0*n1;

	
	// Check for inconsistencies
	std::cout << "\tChecking number of events in each file ";
	for(long i=0; i<nAGIPDmodules; i++) {
		if (module[i].noData)
			continue;

		if(module[i].nframes != module[_referenceModule].nframes) {
			std::cout << "\tInconsistent number of frames between modules " << _referenceModule << " and " << i << std::endl;
			std::cout << "\t" << module[i].nframes << " != " << module[_referenceModule].nframes << std::endl;
            std::cout << "\tsetting moduleOK[i] = false\n";
			moduleOK[i] = false;
		}
		else {
			moduleOK[i] = true;
		}
	}
	std::cout << " [OK]" <<  std::endl;
	
	std::cout << "\tChecking all data is of the same type";
	rawDetectorData = module[_referenceModule].rawDetectorData;
	for(long i=0; i<nAGIPDmodules; i++) {
		if (module[i].noData)
			continue;

		if(module[i].rawDetectorData != rawDetectorData) {
			std::cout << std::endl;
			std::cout << "\tInconsistent data, some is RAW and some is not: " << i << std::endl;
			exit(1);
		}
	}
	std::cout << " [OK]" <<  std::endl;

	
	std::cout << "\tChecking image size in each file";
	for(long i=0; i<nAGIPDmodules; i++)
	{
		if (module[i].noData)
			continue;

		if(module[i].n0 != module[_referenceModule].n0 || module[i].n1 != module[_referenceModule].n1) {
			std::cout << std::endl;
            std::cout << "\tInconsistent image sizes between modules " << _referenceModule << " and " << i << std::endl;
			std::cout << "\t ( " << module[i].n0 << " != " << module[_referenceModule].n0 << " )" << std::endl;
			std::cout << "\t ( " << module[i].n1 << " != " << module[_referenceModule].n1 << " )" << std::endl;
			moduleOK[i] = false;
		}
		else {
			moduleOK[i] = true;
		}
	}
	std::cout << " [OK]" <<  std::endl;

	
	std::cout << "\tChecking for mismatched timestamps" << std::endl;
	//std::vector<long> allTrainIDs;

    
    // Check for start and end trainID and pulseID
    minTrain = INT_MAX;
    maxTrain = INT_MIN;
    minPulse = INT_MAX;
    maxPulse = INT_MIN;
    minCell = INT_MAX;
    maxCell = INT_MIN;

    for(long i=0; i<nAGIPDmodules; i++)
	{
		if (module[i].noData)
			continue;

		if (moduleOK[i] == false)
			continue;


		//long cellID = module[i].cellID;

		//int start = firstModule ? 200 : 0;
        int start = 0;

        std::cout << "\tModule " << i << " train/pulse scan from frame " << start;
        std::cout << " to " << module[i].nframes << std::endl;

		for(long j=start; j<module[i].nframes; j++)
		{
			long trainID = module[i].trainIDlist[j];
			long pulseID = module[i].pulseIDlist[j];
            long cellID = module[i].cellIDlist[j];
            
            // Silly values to be ignored
            if(trainID <= 0)
                continue;

			if (trainID < minTrain) minTrain = trainID;
			if (trainID > maxTrain) maxTrain = trainID;

			if (pulseID < minPulse) minPulse = pulseID;
			if (pulseID > maxPulse) maxPulse = pulseID;

			if (cellID < minCell) minCell = cellID;
			if (cellID > maxCell) maxCell = cellID;
		}
	}

	currentTrain = minTrain;
	currentPulse = minPulse;

    std::cout << "****** Begin AGIPD configuration ******\n";

	std::cout << "Trains extend from IDs " << minTrain << " to " << maxTrain << std::endl;
	std::cout << "Pulses extend from IDs " << minPulse << " to " << maxPulse << std::endl;
    std::cout << "Current train set to minimum, " << minTrain << std::endl;
	std::cout << "Current pulse set to minimum, " << minPulse << std::endl;
	std::cout << "Cells of module readout extend from IDs " << minCell << " to " << maxCell << std::endl;

    
    // This is what happens with a blank set of files
    if(minTrain == INT_MAX && maxTrain==INT_MIN && minPulse==INT_MAX && maxPulse==INT_MIN) {
        std::cout << "WARNING: all files appear to be empty of any data" << std::endl;
        std::cout << "minTrain == INT_MAX && maxTrain==INT_MIN && minPulse==INT_MAX && maxPulse==INT_MIN" << std::endl;
        std::cout << "Setting minTrain = 1; maxTrain = 1; minPulse = 0; maxPulse = 30;" << std::endl;
        minTrain = 1;
        maxTrain = 10;
        minPulse = 0;
        maxPulse = 30;
    }

    std::cout << "****** End AGIPD configuration ******\n";

    
    // Create trainID/pulseID pairs
	for(long module_num=0; module_num<nAGIPDmodules; module_num++) {
		for (long train = minTrain; train <= maxTrain; train++) {
			for (long pulse = minPulse; pulse <= maxPulse; pulse++) {
				TrainPulsePair train2Pulse = std::make_pair(train, pulse);
				TrainPulseModulePair tp2Module = std::make_pair(train2Pulse, module_num);
				trainPulseMap[tp2Module] = -1; // start with unassigned
			}
		}
	}

	for(long module_num=0; module_num<nAGIPDmodules; module_num++) {
		if (module[module_num].noData)
			continue;

		for(long frame=0; frame < module[module_num].nframes; frame++) {
			long trainID = module[module_num].trainIDlist[frame];
			long pulseID = module[module_num].pulseIDlist[frame];

			TrainPulsePair train2Pulse = std::make_pair(trainID, pulseID);
			TrainPulseModulePair tp2Module = std::make_pair(train2Pulse, module_num);
			trainPulseMap[tp2Module] = frame;
		}
	}

	
	// Allocate memory for data and masks
	data = (float*) malloc(nn*sizeof(float));
	badpixMask = (uint16_t*) malloc(nn*sizeof(uint16_t));
	digitalGain = (uint16_t*) malloc(nn*sizeof(uint16_t));
	
	
	// Pointer to the data location for each module
	// (for copying module data and for those who want to look at the data as a stack of panels)
	long offset;
	for (long i=0; i < nAGIPDmodules; i++) {
		offset = modulenn;
		pdata[i] = data + i * offset;
		pgain[i] = digitalGain + i*offset;
		pmask[i] = badpixMask + i*offset;
	}

	// Bye bye
	std::cout << "All AGIPD files successfully opened\n";
}
// cAgipdReader::open()


void cAgipdReader::close(void){

	std::cout << "Closing AGIPD files " << std::endl;
	
	// Close files for each module
	for(long i=0; i<nAGIPDmodules; i++) {
		std::cout << "\tClosing " << moduleFilename[i] << std::endl;
		module[i].close();
	}
	std::cout << "\t" << nAGIPDmodules << " module elements closed " << std::endl;
	
	// Clean up memory
	if(data != NULL) {
		free(data);
		free(badpixMask);
		free(digitalGain);
		data = NULL;
		badpixMask = NULL;
		digitalGain = NULL;
	}
	std::cout << "\tAGIPD reader closed " << std::endl;
}
// cAgipdReader::close()


// Why is there a separate private and public function for this?
bool cAgipdReader::nextFrame() {
	return nextFramePrivate();
}


bool cAgipdReader::nextFramePrivate() {
	lastModule = -1;
	bool success = true;

	while (lastModule < 0 && success) {
        // Go to next pulse; if next pulse is off end of train then go to next train
        currentPulse++;
		if (currentPulse >= maxPulse) {
			currentPulse = minPulse;
			currentTrain++;
			goodImages4ThisTrain = -1;
		}
		
		// Skip to first useful pulse in a train (corrupted)
        if(true) {
            if(currentPulse < _firstPulseId) {
                //std::cout << "Skipping pulse currentPulse in train " << currentTrain << std::endl;
                continue;
            }
        }

		//	PulseID is taken from the AGIPD firmware --> used for determining which are good data frames
		//  Good frames occur when pulseID % _pulseIDmodulo == 0
		if(currentPulse % _pulseIDmodulo > 0)
			continue;
		
		
        // Skip first _newFileSkip trains in a file (corrupted)
        //if(currentTrain < _newFileSkip) {
        //    continue;
        //}
		

		success = readFrame(currentTrain, currentPulse);

		if (lastModule >= 0) {
			goodImages4ThisTrain++;
		}
	}
    
	// Statistics on bad pixels, etc...
    if(true) {
        long    pixelsInGainLevel[3] = {0,0,0};
        long    nbad=0;

        for(long p=0; p<nn; p++) {
            if(true) {
                if(digitalGain[p]<0 || digitalGain[p] > 2 ){
                    std::cout << "Bad digital gain value: " << digitalGain[p] << std::endl;
                }
            }
            pixelsInGainLevel[digitalGain[p]] += 1;
            if(badpixMask[p] != 0)
                nbad++;
        }
        std::cout << "nPixels in gain mode (0,1,2) = (";
        std::cout << pixelsInGainLevel[0] << ", ";
        std::cout << pixelsInGainLevel[1] << ", ";
        std::cout << pixelsInGainLevel[2] << "),   ";
        printf("%li bad pixels (%f%%)\n", nbad, (100.*nbad)/nn);
    }

    std::cout << "Returning image number " << goodImages4ThisTrain << " in train "  << currentTrain << std::endl;
    return success;
}


// Go back to the start of the queue
void cAgipdReader::resetCurrentFrame() {
	currentPulse = minPulse;
	currentTrain = minTrain;
}


// Set the frame to blank if there is an error
void cAgipdReader::setModuleToBlank(int moduleID) {
    statusID[moduleID] = 1;
	for(long p=0; p<modulenn; p++) {
        pdata[moduleID][p] = 0;
        pgain[moduleID][p] = 0;
		pmask[moduleID][p] = 1;
	}
}


bool cAgipdReader::readFrame(long trainID, long pulseID)
{
	if (trainID < minTrain || trainID >= maxTrain) {
		std::cout << "\treadFrame::trainID out of bounds " << trainID << std::endl;
		return false;
	}

	if (pulseID < minPulse || pulseID >= maxPulse) {
		std::cout << "\treadFrame::pulseID out of bounds " << pulseID << std::endl;
		return false;
	}

	int moduleCount = 0;
	lastModule = -1;

    
	// Loop through modules
	for(int moduleID=0; moduleID<nAGIPDmodules; moduleID++)
	{
		//std::cout << "Reading module " << i << " of " << nAGIPDmodules << std::endl;
		TrainPulsePair tp = std::make_pair(trainID, pulseID);
		TrainPulseModulePair tp2mod = std::make_pair(tp, moduleID);
		long frameNum = trainPulseMap[tp2mod];

		if (module[moduleID].noData==true || frameNum < 0)
		{
            setModuleToBlank(moduleID);
			cellID[moduleID] = module[moduleID].cellID;
			continue;
		}

		// Read the requested frame number (and update metadata in structure)
        //std::cout << "Reading train " << trainID << " pulse " << pulseID << " module " << i << std::endl;
        module[moduleID].readFrame(frameNum);
        // std::cout << "Reading frame done" << std::endl;

		if (module[moduleID].noData) {
            setModuleToBlank(moduleID);
			continue;
		}
		moduleCount++;

		// Copy across
		cellID[moduleID] = module[moduleID].cellID;
		statusID[moduleID] = module[moduleID].statusID;
        //std::cout << "memcpy data" << std::endl;
		memcpy(pdata[moduleID], module[moduleID].data, modulenn*sizeof(float));
		
        //std::cout << "memcpy gain" << std::endl;
		memcpy(pgain[moduleID], module[moduleID].digitalGain, modulenn*sizeof(uint16_t));

		lastModule = (int)moduleID;
		
		// Set entire panel mask to whatever the status is.
        //std::cout << "Memset mask" << std::endl;
		if(module[moduleID].statusID != 0) {
			for(long p=0; p<modulenn; p++) {
				pmask[moduleID][p] = module[moduleID].statusID;
			}
		}
		else {
			memcpy(pmask[moduleID], module[moduleID].badpixMask, modulenn*sizeof(uint16_t));
		}
	}

	if (lastModule >= 0)
	{
		std::cout << "Read train " << trainID << ", pulseID " << pulseID << " with "
        << moduleCount << " modules";
	}

    
    // Print out the module IDs
    if(true) {
        std::cout << ", cellIDs=[";
        for(int moduleID=0; moduleID<nAGIPDmodules; moduleID++) {
            std::cout << cellID[moduleID] << ",";
        }
        std::cout << "]"; // << std::endl;

    }
    
    std::cout << std::endl;
    
	currentTrain = trainID;
	currentPulse = pulseID;
	currentCell = module[0].cellID;

	return true;
}


void cAgipdReader::maxAllFrames(void)
{
	// Array for storying sum of all frames
	float *sumdata;

	std::cout << "Writing out max of all frames..." << std::endl;

	sumdata = (float*) malloc(nn*sizeof(float));
	memset(sumdata, 0, sizeof(float)*nn);

	nextFrame();
	std::cout << "First pixel is: " << data[0] << std::endl;
	resetCurrentFrame();

	cellAveData = (float **)malloc(sizeof(float *) * maxCell);
	cellAveCounts = (int **)malloc(sizeof(int *) * maxCell);

	for (int i = 0; i < maxCell; i++)
	{
		cellAveData[i] = (float *)malloc(sizeof(float) * nn);
		cellAveCounts[i] = (int *)malloc(sizeof(int) * nn);

		memset(cellAveData[i], 0, sizeof(float) * nn);
		memset(cellAveCounts[i], 0, sizeof(int) * nn);
	}

	while (nextFrame())
	{
		for (int j = 0; j < nn; j++)
		{
			// which module?

			int currentModule = (int)nn / modulenn;
			int currentCell = cellID[currentModule];

			if (data[j] > sumdata[j])
			{
				sumdata[j] = data[j];
			}

			cellAveData[currentCell][j] += data[j];
			cellAveCounts[currentCell][j]++;
		}
	}

	for (int i = 0; i < maxCell; i++)
	{
		for (int j = 0; j < nn; j++)
		{
			cellAveData[i][j] /= cellAveCounts[i][j];
		}
	}
}

float *cAgipdReader::getCellAverage(int i)
{
	return cellAveData[i];
}
