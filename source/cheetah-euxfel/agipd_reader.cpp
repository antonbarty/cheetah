//
//  Created by Anton Barty on 24/8/17.
//	Distributed under the GPLv3 license
//


//#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <math.h>
#include "PNGFile.h"
#include "agipd_read.h"
#include <algorithm>

cAgipdReader::cAgipdReader(void){
	data = NULL;
	mask = NULL;
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
 *	Create a list of filenames for all 16 AGIPD modules based on the filename for module 00
 *	Small function but isolated here so it's easy to swap conventions if needed
 */
void cAgipdReader::generateModuleFilenames(char *module0filename){
	long	pos;
	char	tempstr[10];

	// Filenames for all the other modules
	for(long i=0; i<nAGIPDmodules; i++) {
		moduleFilename[i] = module0filename;
		

		// Replace the AGIPD00 number with 00-15
		pos = moduleFilename[i].find("AGIPD");
		sprintf(tempstr, "%0.2li", i);
		moduleFilename[i].replace(pos+5,2,tempstr);
		
		// Which chunk/sequence are we looking at?
		pos = moduleFilename[i].find(".h5");
		long stackPos = pos - 5; /* Position of the stack thing */
		std::string stackNum = moduleFilename[i].substr(stackPos, 5);
		int stackInt = atoi(stackNum.c_str());

		firstModule = (stackInt == 0);
		std::cout << "Stack number is first? - " << firstModule << std::endl;

		if(verbose) {
			printf("\tModule %0.2li = %s\n",i, moduleFilename[i].c_str());
		}
	}
	
	// Filenames for all the other darkcal files
	if(darkcalFile != "No_file_specified") {
		for(long i=0; i<nAGIPDmodules; i++) {
			// Replace the AGIPD00 number with 00-15
			darkcalFilename[i] = darkcalFile;
			pos = darkcalFilename[i].find("AGIPD");
			sprintf(tempstr, "%0.2li", i);
			darkcalFilename[i].replace(pos+5,2,tempstr);
		}
	}

	
	// Filenames for all the other gaincal files
	if(gaincalFile != "No_file_specified") {
		for(long i=0; i<nAGIPDmodules; i++) {
			// Replace the AGIPD00 number with 00-15
			gaincalFilename[i] = gaincalFile;
			pos = gaincalFilename[i].find("AGIPD");
			sprintf(tempstr, "%0.2li", i);
			gaincalFilename[i].replace(pos+5,2,tempstr);
		}
	}
}



/*
 *	Open all AGIPD module files relating to the 00 module
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
		//module[i].readDarkcal(darkcalFilename[i].c_str());
		//module[i].readGaincal(gaincalFilename[i].c_str());
	}

	// Det up size and layout of the assembled data stack
	// Use module[0] as the reference and stack the modules one on top of another
	nframes = module[0].nframes;
	modulen[0] = module[0].n0;
	modulen[1] = module[0].n1;
	modulenn = modulen[0]*modulen[1];
	nmodules[0] = 1;
	nmodules[1] = nAGIPDmodules;
	dims[0] = modulen[0]*nmodules[0];
	dims[1] = modulen[1]*nmodules[1];
	n0 = dims[0];
	n1 = dims[1];
	nn = n0*n1;

	
	// Check for inconsistencies
	std::cout << "\tChecking number of events in each file" << std::endl;
	moduleOK[0] = true;
	for(long i=1; i<nAGIPDmodules; i++) {
		if (module[i].noData)
			continue;

		if(module[i].nframes != module[0].nframes) {
			std::cout << "\tInconsistent number of frames between modules 0 and " << i << std::endl;
			std::cout << "\t" << module[i].nframes << " != " << module[0].nframes << std::endl;
			moduleOK[i] = false;
		}
		else {
			moduleOK[i] = true;
		}
	}
	
	std::cout << "\tChecking all data is of the same type" << std::endl;
	rawDetectorData = module[0].rawDetectorData;
	for(long i=1; i<nAGIPDmodules; i++) {
		if (module[i].noData)
			continue;

		if(module[i].rawDetectorData != rawDetectorData) {
			std::cout << "\tInconsistent data, some is RAW and some is not: " << i << std::endl;
			exit(1);
		}
	}
	
	
	std::cout << "\tChecking image size in each file" << std::endl;
	moduleOK[0] = true;
	for(long i=1; i<nAGIPDmodules; i++)
	{
		if (module[i].noData)
			continue;

		if(module[i].n0 != module[0].n0 || module[i].n1 != module[0].n1) {
			std::cout << "\tInconsistent image sizes between modules 0 and " << i << std::endl;
			std::cout << "\t ( " << module[i].n0 << " != " << module[0].n0 << " )" << std::endl;
			std::cout << "\t ( " << module[i].n1 << " != " << module[0].n1 << " )" << std::endl;
			moduleOK[i] = false;
		}
		else {
			moduleOK[i] = true;
		}
	}

	std::cout << "\tChecking for mismatched timestamps" << std::endl;
	std::vector<long> allTrainIDs;

	for(long i=0; i<nAGIPDmodules; i++)
	{
		if (module[i].noData)
			continue;

		if (moduleOK[i] == false)
			continue;

		minTrain = INT_MAX;
		maxTrain = INT_MIN;
		minPulse = INT_MAX;
		maxPulse = INT_MIN;
		minCell = INT_MAX;
		maxCell = INT_MIN;

		long cellID = module[i].cellID;

		int start = firstModule ? 200 : 0;

		std::cout << "Starting from " << start << std::endl;

		for(long j=start; j<module[0].nframes; j++)
		{
			long trainID = module[i].trainIDlist[j];
			long pulseID = module[i].pulseIDlist[j];

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

	std::cout << "Trains extend from IDs " << minTrain << " to " << maxTrain << std::endl;
	std::cout << "Current train set to minimum, " << minTrain << std::endl;
	std::cout << "Pulses extend from IDs " << minPulse << " to " << maxPulse << std::endl;
	std::cout << "Current pulse set to minimum, " << minPulse << std::endl;
	std::cout << "Cells of module readout extend from IDs " << minCell << " to " << maxCell << std::endl;

	for(long i=0; i<nAGIPDmodules; i++)
	{
		for (long k = minTrain; k < maxTrain; k++)
		{
			for (long j = minPulse; j < maxPulse; j++)
			{
				TrainPulsePair train2Pulse = std::make_pair(k, j);

				TrainPulseModulePair tp2Module;
				tp2Module = std::make_pair(train2Pulse, i);

				trainPulseMap[tp2Module] = -1; // start with unassigned
			}
		}
	}

	for(long i=0; i<nAGIPDmodules; i++)
	{
		if (module[i].noData) continue;

		for(long j=0; j < module[i].nframes; j++)
		{
			long trainID = module[i].trainIDlist[j];
			long pulseID = module[i].pulseIDlist[j];

			TrainPulsePair train2Pulse = std::make_pair(trainID, pulseID);
			TrainPulseModulePair tp2Module = std::make_pair(train2Pulse, i);
			trainPulseMap[tp2Module] = j;
		}

	}

	// Allocate memory for data and masks
	data = (float*) malloc(nn*sizeof(float));
	mask = (uint16_t*) malloc(nn*sizeof(uint16_t));
	digitalGain = (uint16_t*) malloc(nn*sizeof(uint16_t));
	
	
	// Pointer to the data location for each module
	// (for copying module data and for those who want to look at the data as a stack of panels)
	long offset;
	for (long i=0; i < nAGIPDmodules; i++) {
		offset = modulenn;
		pdata[i] = data + i * offset;
		pgain[i] = digitalGain + i*offset;
		pmask[i] = mask + i*offset;
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
		free(mask);
		free(digitalGain);
		data = NULL;
		mask = NULL;
		digitalGain = NULL;
	}
	std::cout << "\tAGIPD reader closed " << std::endl;
}
// cAgipdReader::close()

bool cAgipdReader::nextFrame()
{
	currentPulse++;
	if (currentPulse >= maxPulse)
	{
		currentPulse = minPulse;
		currentTrain++;
	}

	return readFrame(currentTrain, currentPulse);
}

void cAgipdReader::resetCurrentFrame()
{
	currentPulse = minPulse;
	currentTrain = minTrain;
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

	// Loop through modules
	for(long i=0; i<nAGIPDmodules; i++)
	{
		TrainPulsePair tp = std::make_pair(trainID, pulseID);
		TrainPulseModulePair tp2mod = std::make_pair(tp, i);
		long frameNum = trainPulseMap[tp2mod];

		if (frameNum < 0)
		{
			cellID[i] = module[i].cellID;
			statusID[i] = 1;
			memset(pdata[i], 0, modulenn*sizeof(float));
			memset(pgain[i], 0, modulenn*sizeof(uint16_t));
			continue;
		}

		if (module[i].noData)
		{
			continue;
		}

		moduleCount++;

		// Read the requested frame number (and update metadata in structure)
		module[i].readFrame(frameNum);

		if (module[i].noData)
		{
			continue;
		}

		// Copy across
		cellID[i] = module[i].cellID;
		statusID[i] = module[i].statusID;
		memcpy(pdata[i], module[i].data, modulenn*sizeof(float));
		memcpy(pgain[i], module[i].digitalGain, modulenn*sizeof(uint16_t));
		
		
		// Set entire panel mask to whatever the status is.
		memset(pmask[i], module[i].statusID, modulenn*sizeof(uint16_t));
	}

	std::cout << "Read train " << trainID << ", pulse " << pulseID << " with "
	<< moduleCount << " modules." << std::endl;

	
	currentTrain = trainID;
	currentPulse = pulseID;
	currentCell = module[0].cellID;

	return true;
}

void cAgipdReader::writePNG(float *pngData, std::string filename)
{

	double sum = 0;
	double sumSq = 0;

	for (int j = 0; j < nn; j++)
	{
		sum += pngData[j];
		sumSq += pngData[j] * pngData[j];
	}

	double mean = sum / nn;
	double stdev = sqrt(sumSq / nn - mean * mean);
	double minBlack = mean - stdev * 1.5;
	double maxBlack = mean + stdev * 1.5;

	const int one = 128;
	const int xBump = 0;
	const int padding = 10;
	const int xBigBump = 4;
	const int xFarLeft = -(one + 4) * 4 - 34;
	const int xBumpedLeft = xFarLeft + 21;
	const int slant = one * 0.25 + 24;
	const int gap = 16;

	const int roughPos[16][2] =
	{{xBump, (one + padding) * 3 + 23 + 21 + 21 + gap + slant},
		{xBump, (one + padding) * 2 + 23 + 21 + gap + slant},
		{xBump, (one + padding) * 1 + 23 + gap + slant},
		{xBump, (one + padding) * 0 + gap + slant},
		{xBigBump, -(one + padding) * 1 + slant},
		{xBigBump, -(one + padding) * 2 - 29 + slant},
		{xBigBump, -(one + padding) * 3 - 29 - 27 + slant},
		{xBigBump, -(one + padding) * 4 - 29 - 27 - 27 + slant},
		{xBumpedLeft, -(one + padding) * 1},
		{xBumpedLeft, -(one + padding) * 2 - 29},
		{xBumpedLeft, -(one + padding) * 3 - 29 - 27},
		{xBumpedLeft, -(one + padding) * 4 - 29 - 27 - 27},
		{xFarLeft, (one + padding) * 3 + 21 + 21 + 21 + gap},
		{xFarLeft, (one + padding) * 2 + 21 + 21 + gap},
		{xFarLeft, (one + padding) * 1 + 21 + gap},
		{xFarLeft, (one + padding) * 0 + gap}};

	const int axisDir[16][2] =
	{{-1, -1},
		{-1, -1},
		{-1, -1},
		{-1, -1},
		{-1, -1},
		{-1, -1},
		{-1, -1},
		{-1, -1},
		{+1, +1},
		{+1, +1},
		{+1, +1},
		{+1, +1},
		{+1, +1},
		{+1, +1},
		{+1, +1},
		{+1, +1}};

	int width = 1400;
	int height = 1400;

	PNGFile png = PNGFile("max_agipd_file.png", (int)width, (int)height);
	png.setCentre(width / 2 + slant, height / 2 - slant);
	png.setPlain();

	for (int n = 0; n < nAGIPDmodules; n++)
	{
		float *sumPointer = &pngData[0] + (pdata[n] - pdata[0]);
		int cornerX = roughPos[n][0];
		int cornerY = roughPos[n][1];
		int xDir = axisDir[n][0];
		int yDir = axisDir[n][1];

		if (xDir == -1) cornerX += modulen[1];
		if (yDir == -1) cornerY += modulen[0];

		int rawCornerY = n * (int)modulen[1];

		for (int i = 0; i < modulen[0]; i++)
		{
			for (int j = 0; j < modulen[1]; j++)
			{
				long index = j * modulen[0] + i;
				double value = sumPointer[index];
				double percentage = (value - minBlack) / (maxBlack - minBlack);
				if (percentage > 1) percentage = 1;
				if (percentage < 0) percentage = 0;
				percentage = 1 - percentage;
				png_byte greyness = percentage * 255;


				png.setPixelColourRelative(cornerX + j * xDir, cornerY + i * yDir, greyness, greyness, greyness);
			}
		}
		png.drawText("do not use for any real analysis", 0, 0);
	}

	png.writeImageOutput();

	std::cout << "Pixel stdev: " << stdev << std::endl;
	std::cout << "Mean pixel value: " << mean << std::endl;

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

		writePNG(cellAveData[i], "ave_agipd_cell_" + i_to_str(i));
	}
}

float *cAgipdReader::getCellAverage(int i)
{
	return cellAveData[i];
}
