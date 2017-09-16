//
//  cheetah-euxfel
//
//  Created by Anton Barty on 24/8/17.
//	Distributed under the GPLv3 license
//

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include "agipd_read.h"
#include "cheetah.h"



// This is for parsing getopt_long()
struct tCheetahEuXFELparams {
	std::vector<std::string> inputFiles;
	std::string iniFile;
	int verbose;
} CheetahEuXFELparams;
void parse_config(int, char *[], tCheetahEuXFELparams*);
void waitForCheetahWorkers(cGlobal*);



//static char testfile[]="R0126-AGG01-S00002.h5";


// Main entry point for EuXFEL version of Cheetah
// Usage:
// > cheetah-euxfel -i inifile.ini <*AGIPD00*.h5>
//
int main(int argc, char* argv[]) {
	
	
	std::cout << "Cheetah interface for EuXFEL\n";
	std::cout << "Anton Barty, September 2015-\n";

	// Parse configurations
	parse_config(argc, argv, &CheetahEuXFELparams);
	
	
	// For testing
	std::cout << "----------------" << std::endl;
	std::cout << "Program name: " << argv[0] << std::endl;
	std::cout << "cheetah.ini file: " << CheetahEuXFELparams.iniFile << std::endl;
	std::cout << "Input files: " << std::endl;
	for(int i=0; i< CheetahEuXFELparams.inputFiles.size(); i++) {
		std::cout << "\t " << CheetahEuXFELparams.inputFiles[i] << std::endl;
	}
	
	std::cout << "----------------" << std::endl;

	
	// Initialize Cheetah
	std::cout << "Setting up Cheetah" << std::endl;
	static cGlobal cheetahGlobal;
	static long frameNumber = 0;
	static time_t startT;
	time(&startT);

	strcpy(cheetahGlobal.configFile, argv[2]);				// FIX ME
	strcpy(cheetahGlobal.experimentID, "XFEL2012");			// FIX ME
	cheetahGlobal.runNumber = 1;							// FIX ME
	strcpy(cheetahGlobal.facility,"EuXFEL");				// FIX ME

	cheetahInit(&cheetahGlobal);
	

	static char testfile[]="RAW-R0283-AGIPD00-S00000.h5";
	
	
	// Initialise AGIPD frame reading stuff
	cAgipdReader agipd;
	agipd.verbose=1;

	//  Files for calibration stuff
	//	Will pick up darkcal and gaincal filenames from cheetah.ini: maintains the same 'feel'as before
	//	Specify file for AGIPD00 - files for other modules are guessed automatically
	//	cheetahGlobal.detector[0] defaults to "No_file_specified" if nothing set in cheetah.ini
	agipd.gaincalFile = cheetahGlobal.detector[0].darkcalFile;
	agipd.darkcalFile = cheetahGlobal.detector[0].gaincalFile;
	
	
	// Loop through all listed *AGIPD00*.h5 files
	for(long fnum=0; fnum<CheetahEuXFELparams.inputFiles.size(); fnum++) {

		
		// Open the file
		std::cout << "Opening " << CheetahEuXFELparams.inputFiles[fnum] << std::endl;
		agipd.open((char *)CheetahEuXFELparams.inputFiles[fnum].c_str());
		agipd.setSkip(1);

		// Guess the run number
		long	pos;
		long	runNumber;
		pos = CheetahEuXFELparams.inputFiles[fnum].rfind("RAW-R");
		runNumber = atoi(CheetahEuXFELparams.inputFiles[fnum].substr(pos+5,4).c_str());
		std::cout << "This is run number " << runNumber << std::endl;
		//runNumber = atoi(moduleFilename[i].substr(pos+5,4).c_str());

		
		
		// Process frames in this file
		std::cout << "Reading individual frames\n";
		while (agipd.nextFrame())
		{
			if (!agipd.goodFrame())
			{
				continue;
			}

			// Add more sensible event name
			
			cEventData * eventData = cheetahNewEvent(&cheetahGlobal);
			eventData->frameNumber = frameNumber;
			strcpy(eventData->eventname,CheetahEuXFELparams.inputFiles[fnum].c_str());
			eventData->runNumber = runNumber;
			eventData->nPeaks = 0;
			eventData->pumpLaserCode = 0;
			eventData->pumpLaserDelay = 0;
			eventData->photonEnergyeV = cheetahGlobal.defaultPhotonEnergyeV;
			eventData->wavelengthA = 0;
			eventData->pGlobal = &cheetahGlobal;
			//eventData->detectorZ = 15e-3;
			
			// Add train and pulse ID to event data.
			eventData->trainID = agipd.currentTrain;
			eventData->pulseID = agipd.currentPulse;
			eventData->cellID = agipd.currentCell;
			
			//
			if(strcmp(cheetahGlobal.pumpLaserScheme, "xfel_pulseid") == 0) {
				if(agipd.currentPulse >= 0 && agipd.currentPulse < cheetahGlobal.nPowderClasses-1) {
					eventData->pumpLaserCode = agipd.currentPulse;
					eventData->powderClass = agipd.currentPulse;
				}
				else {
					continue;
				}
			}

			
			// Check image dimensions
			int detId = 0;
			if (agipd.n0 != cheetahGlobal.detector[detId].pix_nx || agipd.n1 != cheetahGlobal.detector[detId].pix_ny) {
				printf("Error: File image dimensions of %zu x %zu did not match detector dimensions of %li x %li\n", agipd.n0, agipd.n1, eventData->pGlobal->detector[detId].pix_nx, eventData->pGlobal->detector[detId].pix_ny);
				continue;
			}

			// Allocate memory for image data
			memcpy(eventData->detector[detId].data_raw, agipd.data, agipd.nn*sizeof(float));
			eventData->detector[detId].data_raw_is_float = true;
					   
			
			// Process event
			cheetahProcessEventMultithreaded(&cheetahGlobal, eventData);
		}
	
		usleep(5000);
		std::cout << "Closing AGIPD modules" << std::endl;
		agipd.close();
		std::cout << "Finished with " << CheetahEuXFELparams.inputFiles[fnum] << std::endl;

	}
	// File loop

	
	/*
		// Test code for reading an individual AGIPD module
		if(false) {
	 cAgipdModuleReader	agipdModule;
	 agipdModule.verbose = 1;
	 agipdModule.open(testfile);
	 agipdModule.readHeaders();
	 
	 // Read stack
	 //agipdModule.readImageStack();
	 
	 
	 // Read images
	 std::cout << "Reading individual frames\n";
	 for(long i=0; i<10; i++) {
	 agipdModule.readFrame(i);
	 }
	 
	 // Close
	 agipdModule.close();
		}
	 */
	
	
	
	
	waitForCheetahWorkers(&cheetahGlobal);
	if(cheetahGlobal.saveCXI) {
		printf("Writing accumulated CXIDB file\n");
		writeAccumulatedCXI(&cheetahGlobal);
		closeCXIFiles(&cheetahGlobal);
	}

	
	
	// Cleanup
	cheetahExit(&cheetahGlobal);
	printf("Clean Exit\n");
	return 0;
}


void waitForCheetahWorkers(cGlobal *cheetahGlobal){
	time_t	tstart, tnow;
	time(&tstart);
	double	dtime;
	int p=0, pp=0;
	
	while(cheetahGlobal->nActiveCheetahThreads > 0) {
		p = cheetahGlobal->nActiveCheetahThreads;
		if ( pp != p){
			pp = p;
			printf("Waiting for %li worker threads to finish.\n", cheetahGlobal->nActiveCheetahThreads);
		}
		time(&tnow);
		dtime = difftime(tnow, tstart);
		if(( dtime > ((float) cheetahGlobal->threadTimeoutInSeconds) ) && (cheetahGlobal->threadTimeoutInSeconds > 0)) {
			printf("\t%li threads still active after waiting %f seconds\n", cheetahGlobal->nActiveCheetahThreads, dtime);
			printf("\tGiving up and exiting anyway\n");
			cheetahGlobal->nActiveCheetahThreads = 0;
			break;
		}
		usleep(500000);
	}
	printf("Cheetah workers stopped successfully.\n");
	
}


/*
 *	Configuration parser (getopt_long)
 */
void parse_config(int argc, char *argv[], tCheetahEuXFELparams *global) {
	
	// Add getopt-long options
	const struct option longOpts[] = {
		{ "inifile", required_argument, NULL, 'i' },
		{ "verbose", no_argument, NULL, 'v' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, no_argument, NULL, 0 }
	};
	const char optString[] = "i:vh?";
	
	int opt;
	int longIndex;
	while( (opt=getopt_long(argc, argv, optString, longOpts, &longIndex )) != -1 ) {
		switch( opt ) {
			case 'v':
				global->verbose++;
				break;
			case 'i':
				global->iniFile = optarg;
				break;
			case 'h':   /* fall-through is intentional */
			case '?':
				std::cout << "Provide help files later on" << std::endl;
				exit(1);
				break;
			case 0:     /* long option without a short arg */
				if( strcmp( "random", longOpts[longIndex].name ) == 0 ) {
					//global->random = 1;
				}
				break;
			default:
				/* You won't actually get here. */
				break;
		}
	}
	
	// This is where unprocessed arguments end up
	std::cout << "optind: " << optind << std::endl;
	std::cout << "Number of unprocessed arguments: " << argc << std::endl;
	for(long i=optind; i<argc; i++) {
		global->inputFiles.push_back(argv[i]);
		std::cout << "\t" << global->inputFiles.back() << std::endl;
	}

	if(global->inputFiles.size() == 0) {
		std::cout << "No input files specified" << std::endl;
		exit(1);
	}

	std::cout << "cheetah.ini file: " << global->iniFile << std::endl;

}


