//
//  Created by Anton Barty on 24/8/17.
//	Distributed under the GPLv3 license
//


//#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "agipd_read.h"



cAgipdReader::cAgipdReader(void){
	data = NULL;
	mask = NULL;
	digitalGain = NULL;
	rawDetectorData = true;
	verbose = 0;
};

cAgipdReader::~cAgipdReader(){
	if(data == NULL)
		return;
	close();
};



/*
 *	Create a list of filenames for all 16 AGIPD modules based on the filename for module 00
 *	Small function but isolated here so it's easy to swap conventions if needed
 */
void cAgipdReader::generateModuleFilenames(char *module0filename){
	for(long i=0; i<nAGIPDmodules; i++) {
		moduleFilename[i] = module0filename;
		
		long	pos;
		char	tempstr[10];
		pos = moduleFilename[i].find("AGIPD");
		sprintf(tempstr, "%0.2li", i);
		moduleFilename[i].replace(pos+5,2,tempstr);
		
		if(verbose) {
			printf("\tModule %0.2li = %s\n",i, moduleFilename[i].c_str());
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
		module[i].open((char *) moduleFilename[i].data());
		module[i].readHeaders();
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
		if(module[i].rawDetectorData != rawDetectorData) {
			std::cout << "\tInconsistent data, some is RAW and some is not: " << i << std::endl;
			exit(1);
		}
	}
	
	
	std::cout << "\tChecking image size in each file" << std::endl;
	moduleOK[0] = true;
	for(long i=1; i<nAGIPDmodules; i++) {
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
	long nMismatch = 0;
	for(long i=1; i<nAGIPDmodules; i++) {
		if (moduleOK[i] == false)
			continue;
		
		for(long j=0; j<module[0].nframes; j++){
			if(module[i].pulseIDlist[j] != module[0].pulseIDlist[j] || module[i].trainIDlist[j] != module[0].trainIDlist[j]) {
				std::cout << "\tInconsistent pulse or train ID between modules 0 and " << i << std::endl;
				nMismatch += 1;
			}
		}
	}
	std::cout << "\tNumber of mismatched events " << nMismatch << std::endl;
	
	
	// Allocate memory for data and masks
	data = (uint16_t*) malloc(nn*sizeof(uint16_t));
	mask = (uint16_t*) malloc(nn*sizeof(uint16_t));
	digitalGain = (uint16_t*) malloc(nn*sizeof(uint16_t));
	
	
	// Pointer to the data location for each module
	// (for copying module data and for those who want to look at the data as a stack of panels)
	long offset;
	for(long i=0; i<nAGIPDmodules; i++) {
		offset = modulenn;
		pdata[i] = data + i*offset;
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
	
	// Clean up memory
	free(data);
	free(mask);
	free(digitalGain);
	data = NULL;
	mask = NULL;
	digitalGain = NULL;

}
// cAgipdReader::close()



void cAgipdReader::readFrame(long frameNum){
	if(frameNum < 0 || frameNum >= nframes) {
		std::cout << "\treadFrame::franeNum out of bounds " << frameNum << std::endl;
		return;
	}
	
	std::cout << "\tReading frame " << frameNum << std::endl;
	currentFrame = frameNum;
	
	
	// Loop through modules
	for(long i=0; i<nAGIPDmodules; i++) {
		// Debug text
		if(verbose) {
			std::cout << "\tFrame " << frameNum << " module " << i << std::endl;
		}

		
		// Read the requested frame number (and update metadata in structure)
		module[i].readFrame(frameNum);
		
		
		// Copy across
		cellID[i] = module[i].cellID;
		statusID[i] = module[i].statusID;
		memcpy(pdata[i], module[i].data, modulenn*sizeof(uint16_t));
		memcpy(pgain[i], module[i].digitalGain, modulenn*sizeof(uint16_t));
		
		
		// Mark the entire panel as bad if it's status is nonzero
		if(module[i].statusID == 0) {
			memset(pmask[i], 0, modulenn*sizeof(uint16_t));
		}
		else {
			memset(pmask[i], module[i].statusID, modulenn*sizeof(uint16_t));
		}
	}

	// These are updated only after readFrame is called !!
	trainID = module[0].trainID;
	pulseID = module[0].pulseID;
	

}
