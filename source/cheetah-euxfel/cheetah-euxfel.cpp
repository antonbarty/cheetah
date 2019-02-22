//
//  cheetah-euxfel
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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <cxxabi.h>
#include "agipd_reader.h"
#include "cheetah.h"

#include <execinfo.h>
#include <signal.h>


// Error handler
void handler(int sig) {
	void *buffer[100];
	int size;

	// get void*'s for all entries on the stack
	size = backtrace(buffer, 100);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d: %s\n", sig, strsignal(sig));
	backtrace_symbols_fd(buffer, size, STDERR_FILENO);
    
    // Try to de-mangle the strings
	int status = -1;
    char **string;
    string = backtrace_symbols(buffer, size);
    if(string != NULL) {
        fprintf(stderr, "-----------------More readable version-----------------\n");
        fprintf(stderr, "Error: signal %d: %s\n", sig, strsignal(sig));
        for (long j = 0; j < size; j++) {
            //printf("%s\n", strings[j]);
			char *demangledName = abi::__cxa_demangle(string[j], NULL, NULL, &status );
			std::cout << demangledName << std::endl;
			free(demangledName);
        }
		fprintf(stderr, "-------------------------------------------------------\n");
    }
    
	exit(1);
}


// This is for parsing getopt_long()
struct tCheetahEuXFELparams {
	std::vector<std::string> inputFiles;
	std::string iniFile;
    std::string calibFile;
    std::string exptName;
	std::string dataFormat;
    int frameStride;
    int frameSkip;
	int verbose;
	bool nogainswitch;
} CheetahEuXFELparams;
void parse_config(int, char *[], tCheetahEuXFELparams*);
void waitForCheetahWorkers(cGlobal*);



//static char testfile[]="R0126-AGG01-S00002.h5";


// Main entry point for EuXFEL version of Cheetah
// Usage:
// > cheetah-euxfel -i inifile.ini -c calib.ini <*AGIPD00*.h5>
//
int main(int argc, char* argv[]) {
	
	signal(SIGSEGV, handler);
	signal(SIGABRT, handler);

	std::cout << "Cheetah interface for EuXFEL\n";
	std::cout << "Anton Barty and Helen Ginn, September 2015-\n";

	// Parse configurations
	parse_config(argc, argv, &CheetahEuXFELparams);
	
	
	// For testing
	std::cout << "----------------" << std::endl;
	std::cout << "Program name: " << argv[0] << std::endl;
	std::cout << "cheetah.ini file: " << CheetahEuXFELparams.iniFile << std::endl;
    std::cout << "calib.ini file: " << CheetahEuXFELparams.calibFile << std::endl;
	std::cout << "Input files: " << std::endl;
	for(int i=0; i< CheetahEuXFELparams.inputFiles.size(); i++) {
		std::cout << "\t " << CheetahEuXFELparams.inputFiles[i] << std::endl;
	}
	
	std::cout << "----------------" << std::endl;

    // Timing stuff
    cMyTimer timer_dataLoad;
    cMyTimer timer_evtCopy;

	
	// Initialize Cheetah
	std::cout << "Setting up Cheetah" << std::endl;
	static cGlobal cheetahGlobal;
	static long frameNumber = 0;
	static time_t startT;
	time(&startT);

    strcpy(cheetahGlobal.facility,"EuXFEL");
	strcpy(cheetahGlobal.configFile, CheetahEuXFELparams.iniFile.c_str());
    strcpy(cheetahGlobal.calibFile, CheetahEuXFELparams.calibFile.c_str());
	strcpy(cheetahGlobal.experimentID, CheetahEuXFELparams.exptName.c_str());

	cheetahInit(&cheetahGlobal);
	

	//static char testfile[]="RAW-R0283-AGIPD00-S00000.h5";
	
	
	// Initialise AGIPD frame reading stuff
	cAgipdReader agipd;
	agipd.verbose=1;
    agipd.setScheme((char*) CheetahEuXFELparams.dataFormat.c_str());

    // Set command line option overrides
    //if(CheetahEuXFELparams.frameSkip != -1)
    //    agipd.setSkip(CheetahEuXFELparams.frameSkip);
    //if(CheetahEuXFELparams.frameStride != -1)
    //    agipd.setStride(CheetahEuXFELparams.frameStride);
	if(CheetahEuXFELparams.nogainswitch)
		agipd.setDoNotApplyGainSwitch(CheetahEuXFELparams.nogainswitch);

	//  Files for calibration stuff
	//	Will pick up darkcal and gaincal filenames from cheetah.ini: maintains the same 'feel'as before
	//	Specify file for AGIPD00 - files for other modules are guessed automatically
	//	cheetahGlobal.detector[0] defaults to "No_file_specified" if nothing set in cheetah.ini
	agipd.darkcalFile = cheetahGlobal.detector[0].darkcalFile;
	agipd.gaincalFile = cheetahGlobal.detector[0].gaincalFile;
	
    
    
	// Loop through all listed *AGIPD00*.h5 files
	for(long fnum=0; fnum<CheetahEuXFELparams.inputFiles.size(); fnum++) {

		
		// Open the file
		std::cout << "Opening " << CheetahEuXFELparams.inputFiles[fnum] << std::endl;
		agipd.open((char *)CheetahEuXFELparams.inputFiles[fnum].c_str());

        // How big is this file?
        std::cout << "Number of frames in this file: " << agipd.nframes << std::endl;
        
        // Handle empty files, which now appear quite commonly
        if(agipd.nframes == 0) {
            std::cout << "Uh oh - we seem to have an empty file here, agipd.nframes == " << agipd.nframes << std::endl;
            std::cout << "Skipping this file \n" ;
            agipd.close();
            continue;
        }
        
        
		// Guess the run number
		long	pos;
		long	runNumber;
		pos = CheetahEuXFELparams.inputFiles[fnum].rfind("-AGIPD");
		runNumber = atoi(CheetahEuXFELparams.inputFiles[fnum].substr(pos-4,4).c_str());
		std::cout << "This is run number " << runNumber << std::endl;
		cheetahGlobal.runNumber = (int) runNumber;

		
		
		// Process frames in this file
		std::cout << "Reading individual frames\n";
		agipd.resetCurrentFrame();
        timer_dataLoad.start();
		while (agipd.nextFrame()) {
            timer_dataLoad.stop();

			
            // Incrememnt the frame number
            frameNumber++;

            
            if (!agipd.goodFrame()) {
				continue;
			}

			
			// First pulse in a train is junk
			if(agipd.currentPulse == 0) {
				std::cout << "Skipping pulse 0 in train (in cheetah-euxfel.cpp)" << std::endl;
				continue;
			}
            
            if(false) {
                if(agipd.currentCell >= 62) {
                    std::cout << "!! Hack for Orville June 2018: Skipping pulses beyond 62 in train (in cheetah-euxfel.cpp)" << std::endl;
                    continue;
                }
            }

            
			// Set up new Cheetah event
            timer_evtCopy.start();
			cEventData * eventData = cheetahNewEvent(&cheetahGlobal);
			eventData->frameNumber = frameNumber;

            // Add a sensible event name
            std::string eventName = CheetahEuXFELparams.inputFiles[fnum] + "_" + i_to_str(agipd.currentTrain) + "_" + i_to_str(agipd.currentPulse);
			strcpy(eventData->eventname,eventName.c_str());
            
            // Copy other information into event (extract from EuXFEL data once it's available)
			eventData->runNumber = runNumber;
			eventData->nPeaks = 0;
			eventData->pumpLaserCode = 0;
			eventData->pumpLaserDelay = 0;
			eventData->photonEnergyeV = cheetahGlobal.defaultPhotonEnergyeV;
			eventData->wavelengthA = 12400 / cheetahGlobal.defaultPhotonEnergyeV;
			eventData->pGlobal = &cheetahGlobal;
			//eventData->detectorZ = 15e-3;
			
			// Add train and pulse ID to event data.
			eventData->trainID = agipd.currentTrain;
			eventData->pulseID = agipd.currentPulse;
			eventData->cellID = agipd.currentCell;
			
			
            // Sort by XFEL pulse ID.
            // This may not always be a good idea as the number of pulses is currently set to a maximum 15 elsewhere
			if(strcmp(cheetahGlobal.pumpLaserScheme, "xfelpulseid") == 0) {
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

			// Copy AGIPD image into Cheetah event
			memcpy(eventData->detector[detId].data_raw, agipd.data, agipd.nn*sizeof(float));
			eventData->detector[detId].data_raw_is_float = true;
					   
            
            // Copy AGIPD bad pixel mask into Cheetah event
            // We do nothing with the gain information beyond checking how many pixels in each gain stage
            for (long i = 0; i < cheetahGlobal.detector[detId].pix_nn; i++) {
                if(agipd.badpixMask[i] != 0) {
                    eventData->detector[detId].pixelmask[i] = PIXEL_IS_BAD;
                }
            }
            timer_evtCopy.stop();


			// Process event
			cheetahProcessEventMultithreaded(&cheetahGlobal, eventData);
            
            // Timing
            cheetahGlobal.timeProfile.addToTimer(timer_evtCopy.duration, cheetahGlobal.timeProfile.TIMER_EVENTCOPY);
            cheetahGlobal.timeProfile.addToTimer(timer_dataLoad.duration, cheetahGlobal.timeProfile.TIMER_EVENTDATA);
            timer_dataLoad.start();
		}
        // end agipd.nextFrame()
	
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
 *  Print some useful information
 */
void print_help(void){
    std::cout << "Cheetah interface for EuXFEL\n";
    std::cout << "Anton Barty, Helen Ginn, September 2015-\n";
    std::cout << std::endl;
    std::cout << "usage: cheetah-euxfel -i <INIFILE> -c calib.ini *AGIPD00.s*.h5 \n";
    std::cout << std::endl;
    std::cout << "\t--inifile=<file)     Specifies cheetah.ini file to use\n";
    std::cout << "\t--experiment=<name>  String specifying the experiment name (used for lableling and setting the file layout)\n";
    std::cout << "\t--stride=<n>         Process only every <n>th frame\n";
    std::cout << "\t--skip=<n>           Skip the first <n> frame of each .h5 file\n";
	std::cout << "\t--nogainswitch       Disable gain switching calibration (assume all high gain)\n";
	std::cout << "\t--dataformat         Data layout {XFEL2012, XFEL2066}\n";
    std::cout << std::endl;
    std::cout << "End of help\n";
}


/*
 *	Configuration parser (getopt_long)
 */
void parse_config(int argc, char *argv[], tCheetahEuXFELparams *global) {
	
    // Defaults
    global->iniFile = "cheetah.ini";
    global->calibFile = "None";
    global->exptName = "XFEL";
	global->dataFormat = "XFEL2012";
    global->frameStride = -1;
    global->frameSkip = -1;
	global->nogainswitch = false;

    
	// Add getopt-long options
    // three legitimate values: no_argument, required_argument and optional_argument
	const struct option longOpts[] = {
		{ "inifile", required_argument, NULL, 'i' },
        { "calibfile", required_argument, NULL, 'c' },
        { "stride", required_argument, NULL, 0 },
        { "skip", required_argument, NULL, 0 },
        { "experiment", required_argument, NULL, 'e' },
		{ "dataformat", required_argument, NULL, 'f' },
		{ "verbose", no_argument, NULL, 'v' },
		{ "nogainswitch", no_argument, NULL, 'g' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, no_argument, NULL, 0 }
	};
    const char optString[] = "i:c:e:f:vgh?";
	
	int opt;
	int longIndex;
	while( (opt=getopt_long(argc, argv, optString, longOpts, &longIndex )) != -1 ) {
		switch( opt ) {
			case 'v':
				global->verbose++;
				break;
			case 'i':
				global->iniFile = optarg;
                std::cout << "cheetah.ini file set to " << global->iniFile << std::endl;
				break;
            case 'c':
                global->calibFile = optarg;
                std::cout << "calib.ini file set to " << global->calibFile << std::endl;
                break;
			case 'e':
				global->exptName = optarg;
				std::cout << "Experiment name set to " << global->exptName << std::endl;
				break;
			case 'f':
				global->dataFormat = optarg;
				std::cout << "Data format will be " << global->dataFormat << std::endl;
				break;
			case 'g':
				global->nogainswitch = true;
				std::cout << "No gain switching " << global->nogainswitch << std::endl;
				break;
			case 'h':   /* fall-through is intentional */
			case '?':
                print_help();
				exit(1);
				break;
                
			case 0:     /* long option without a short arg */
				if( strcmp( "stride", longOpts[longIndex].name ) == 0 ) {
                    global->frameStride = atoi(optarg);
                    std::cout << "Stride set to " << global->frameStride << std::endl;
				}
                if( strcmp( "skip", longOpts[longIndex].name ) == 0 ) {
                    global->frameSkip = atoi(optarg);
                    std::cout << "Skip set to " << global->frameSkip << std::endl;
                }
				if( strcmp( "nogainswitch", longOpts[longIndex].name ) == 0 ) {
					global->nogainswitch = true;
					std::cout << "No gain switching " << global->nogainswitch << std::endl;
				}
				if( strcmp( "dataformat", longOpts[longIndex].name ) == 0 ) {
					global->dataFormat = true;
					std::cout << "Data format will be " << global->dataFormat << std::endl;
				}
				break;
                
			default:
				/* You won't actually get here. */
				break;
		}
	}
	
    
	// This is where unprocessed arguments end up
	std::cout << "optind: " << optind << std::endl;
	std::cout << "Number of unprocessed arguments: " << argc-optind << std::endl;
	for(long i=optind; i<argc; i++) {
		global->inputFiles.push_back(argv[i]);
		std::cout << "\t" << global->inputFiles.back() << std::endl;
	}

	if(global->inputFiles.size() == 0) {
		std::cout << "No input files specified" << std::endl;
		exit(1);
	}

	std::cout << "cheetah.ini file: " << global->iniFile << std::endl;
    std::cout << "calib.ini file: " << global->calibFile << std::endl;

}


