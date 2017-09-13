//
//  Created by Anton Barty on 24/8/17.
//	Distributed under the GPLv3 license
//

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "agipd_read.h"



struct tGlobal {
	std::vector<std::string> inputFiles;
	char *outFileName;
	int random;
	int verbose;
} global;
void parse_config(int, char *[], tGlobal *);


//static char testfile[]="R0126-AGG01-S00002.h5";



int main(int argc, char* argv[]) {
	
	
	// Parse configurations
	std::cout << "AGIPD file reader test\n";
	parse_config(argc, argv, &global);
	
	static char testfile[]="RAW-R0283-AGIPD00-S00000.h5";
	
	cAgipdReader agipd;
	agipd.verbose=1;
	agipd.open(testfile);

	std::cout << "Reading individual frames\n";
	for(long i=0; i<20; i++) {
		agipd.readFrame(i);
	}
	agipd.close();
	
	
	
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
	
	return 0;
}


/*
 *	Configuration parser (getopt_long)
 */
void parse_config(int argc, char *argv[], tGlobal *global) {

	// Add getopt-long options
	const struct option longOpts[] = {
		{ "output", required_argument, NULL, 'o' },
		{ "verbose", no_argument, NULL, 'v' },
		{ "random", no_argument, NULL, 0 },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, no_argument, NULL, 0 }
	};
	const char optString[] = "Il:o:vh?";
	
	int opt;
	int longIndex;
	while( (opt=getopt_long(argc, argv, optString, longOpts, &longIndex )) != -1 ) {
		switch( opt ) {
			case 'o':
				global->outFileName = optarg;
				break;
			case 'v':
				global->verbose++;
				break;
			case 'h':   /* fall-through is intentional */
			case '?':
				// Display_usage();
				break;
			case 0:     /* long option without a short arg */
				if( strcmp( "random", longOpts[longIndex].name ) == 0 ) {
					global->random = 1;
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
	
}
