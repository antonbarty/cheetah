//
//  Created by Anton Barty on 24/8/17.
//	Distributed under the GPLv3 license
//


//#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <sstream>
#include "agipd_module_reader.h"


/*
 *	LPD at FXE
 *	R0126-AGG01-S00002.h5
 *	/INSTRUMENT/DETLAB_LAB_DAQ-0/DET/0:xtdf/image/data Dataset {5040, 1, 256, 256}
 *	/INSTRUMENT/DETLAB_LAB_DAQ-0/DET/0:xtdf/image/length Dataset {5040/Inf, 1}
 *	/INSTRUMENT/DETLAB_LAB_DAQ-0/DET/0:xtdf/image/pulseId Dataset {5040/Inf, 1}
 *	/INSTRUMENT/DETLAB_LAB_DAQ-0/DET/0:xtdf/image/status Dataset {5040/Inf, 1}
 *	/INSTRUMENT/DETLAB_LAB_DAQ-0/DET/0:xtdf/image/trainId Dataset {5040/Inf, 1}
 */

/*
 *	AGIPD at SPB
 *	RAW-R0185-AGIPD00-S00000.h5
 *	/INSTRUMENT/SPB_DET_AGIPD1M-1/DET/0CH0:xtdf/image/cellId Dataset {15000/Inf, 1}
 *	/INSTRUMENT/SPB_DET_AGIPD1M-1/DET/0CH0:xtdf/image/data Dataset {15000/Inf, 2, 512, 128}
 *	/INSTRUMENT/SPB_DET_AGIPD1M-1/DET/0CH0:xtdf/image/length Dataset {15000/Inf, 1}
 *	/INSTRUMENT/SPB_DET_AGIPD1M-1/DET/0CH0:xtdf/image/pulseId Dataset {15000/Inf, 1}
 *	/INSTRUMENT/SPB_DET_AGIPD1M-1/DET/0CH0:xtdf/image/status Dataset {15000/Inf, 1}
 *	/INSTRUMENT/SPB_DET_AGIPD1M-1/DET/0CH0:xtdf/image/trainId Dataset {15000/Inf, 1}
 */


/*
// LPD at FXE
static char h5_trainId_field[] = "/INSTRUMENT/DETLAB_LAB_DAQ-0/DET/0:xtdf/image/trainId";
static char h5_pulseId_field[] = "/INSTRUMENT/DETLAB_LAB_DAQ-0/DET/0:xtdf/image/pulseId";
static char h5_image_data_field[] = "/INSTRUMENT/DETLAB_LAB_DAQ-0/DET/0:xtdf/image/data";
static char h5_image_status_field[] = "/INSTRUMENT/DETLAB_LAB_DAQ-0/DET/0:xtdf/image/status";
*/

// AGIPD at SPB
/*
static char h5_trainId_field[] = "/INSTRUMENT/SPB_DET_AGIPD1M-1/DET/0CH0:xtdf/image/trainId";
static char h5_pulseId_field[] = "/INSTRUMENT/SPB_DET_AGIPD1M-1/DET/0CH0:xtdf/image/pulseId";
static char h5_cellId_field[] = "/INSTRUMENT/SPB_DET_AGIPD1M-1/DET/0CH0:xtdf/image/cellId";
static char h5_image_data_field[] = "/INSTRUMENT/SPB_DET_AGIPD1M-1/DET/0CH0:xtdf/image/data";
static char h5_image_status_field[] = "/INSTRUMENT/SPB_DET_AGIPD1M-1/DET/0CH0:xtdf/image/status";
*/

static char h5_instrument_prefix[] = "/INSTRUMENT/SPB_DET_AGIPD1M-1/DET/";
static char h5_image_prefix[] = "CH0:xtdf/image/";
static char h5_trainId_suffix[] = "trainId";
static char h5_pulseId_suffix[] = "pulseId";
static char h5_cellId_suffix[] = "cellId";
static char h5_image_data_suffix[] = "data";
static char h5_image_status_suffix[] = "status";

cAgipdModuleReader::cAgipdModuleReader(void){
	h5_file_id = 0;
	pulseIDlist = NULL;
	trainIDlist = NULL;
	cellIDlist = NULL;
	statusIDlist = NULL;
	data = NULL;
	digitalGain = NULL;
	rawDetectorData = true;
	noData = false;
	verbose = 0;

	calibGainFactor = NULL;

	darkcalFilename = "No_file_specified";
	gaincalFilename = "No_file_specified";
};

cAgipdModuleReader::~cAgipdModuleReader(){
	std::cout << "\tModule destructor called" << std::endl;
	cAgipdModuleReader::close();
};

/*
 *	Open HDF5 file and perfom some checks
 */
void cAgipdModuleReader::open(char filename[], int mNum)
{
	
	std::cout << "Opening " << filename << std::endl;
	cAgipdModuleReader::filename = filename;
	
	bool check = fileCheck(filename);

	if (check)
	{
		std::cout << "\tFile check OK\n";
	}
	else
	{
		std::cout << "Dodgy file (never mind)" << std::endl;
	}

	/* Now to make our new field names using the module number */

	std::string prefix = h5_instrument_prefix + i_to_str(mNum) + h5_image_prefix;
	h5_trainId_field = prefix + h5_trainId_suffix;

	h5_pulseId_field = prefix + h5_pulseId_suffix;

	h5_cellId_field = prefix + h5_cellId_suffix;

	h5_image_data_field = prefix + h5_image_data_suffix;

	h5_image_status_field = prefix + h5_image_status_suffix;

	if (false)
	{
		std::cout << h5_trainId_field << std::endl;
		std::cout << h5_pulseId_field << std::endl;
		std::cout << h5_cellId_field << std::endl;
		std::cout << h5_image_data_field << std::endl;
		std::cout << h5_image_status_field << std::endl;
	}

	std::string baseName = getBaseFilename(cAgipdModuleReader::filename);

	// filename starts with 'RAW_' we have raw data
	if(baseName.substr(0,3) == "RAW") {
		std::cout << "\tData is RAW detector data" << std::endl;
		rawDetectorData = true;
	}
	else {
		std::cout << "\tData might have been calibrated" << std::endl;
		rawDetectorData = false;
	}
	
	// Find data set size
	// Use size of image data or length of pulseID to determine the number of frames in this data set
	// We know that there should be this many events in each field, use this for checking purposes
	int ndims;
	hsize_t dims[4];
	size_t	datasize;
	H5T_class_t dataclass;
	// Pulse ID gives only number of frames
	//H5LTget_dataset_info(h5_file_id, h5_pulseId_field, dims, &dataclass, &datasize);
	//nframes = dims[0];
	// Image data gives number of frames, logical block size, stack depth
	int success = H5LTget_dataset_info(h5_file_id, h5_image_data_field.c_str(), dims, &dataclass, &datasize);

	if (success < 0)
	{
		std::cout << "Get dataset info for image - failed. Module set to blank.\n";
		noData = true;
		return;
	}

	nframes = dims[0];
	nstack = dims[1];
	n0 = dims[3];
	n1 = dims[2];

	if (!rawDetectorData)
	{
		n0 = dims[2];
		n1 = dims[1];
		nstack = 1;
	}

	nn = n0*n1;
	
	printf("\tNumber of frames: %li\n", nframes);
	printf("\tImage block size: %lix%li\n", n0, n1);
	printf("\tStack depth: %li\n", nstack);

};
// cAgipdReader::open


void cAgipdModuleReader::close(void){
	if (noData) return;

	if(data==NULL)
		return;
	if(h5_file_id==NULL)
		return;

	if (calibrator != NULL)
	{
		delete calibrator;
		calibrator = NULL;
	}

	if(verbose) {
		std::cout << "\tClosing " << filename << "\n";
	}

	// Free array memory
	if(data != NULL) {
		std::cout << "\tFreeing memory " << filename << "\n";
		free(pulseIDlist);
		free(trainIDlist);
		free(cellIDlist);
		free(statusIDlist);
		free(data);
		free(digitalGain);
		
		// Pointers to NULL
		pulseIDlist = NULL;
		trainIDlist = NULL;
		cellIDlist = NULL;
		statusIDlist = NULL;
		data = NULL;
		digitalGain = NULL;
	}
	
	
	if (h5_file_id != 0) {
		std::cout << "\tClosing HDF5 links " << filename << "\n";
		// Cleanup stale IDs
		hid_t ids[256];
		long n_ids = H5Fget_obj_ids(h5_file_id, H5F_OBJ_ALL, 256, ids);
		for (long i=0; i<n_ids; i++ ) {
			hid_t id;
			H5I_type_t type;
			id = ids[i];
			type = H5Iget_type(id);
			if ( type == H5I_GROUP )
				H5Gclose(id);
			if ( type == H5I_DATASET )
				H5Dclose(id);
			if ( type == H5I_DATASPACE )
				H5Sclose(id);
			if ( type == H5I_DATATYPE )
				H5Dclose(id);
		}

		// Close HDF5 file
		H5Fclose(h5_file_id);
		h5_file_id = 0;
	}
};
// cAgipdReader::close



void cAgipdModuleReader::readHeaders(void)
{
	if (noData) return;

	//std::cout << "Reading headers " << filename << "\n";
	std::cout << "Reading vector fields from " << filename << "\n";
	
	// PulseID (H5T_STD_U64LE)
	pulseIDlist = (uint64_t*) checkAllocRead((char *)h5_pulseId_field.c_str(), nframes, H5T_STD_U64LE, sizeof(uint64_t));

	// TrainID (H5T_STD_U64LE)
	trainIDlist = (uint64_t*) checkAllocRead((char *)h5_trainId_field.c_str(), nframes, H5T_STD_U64LE, sizeof(uint64_t));

	// CellID (H5T_STD_U64LE)
	cellIDlist = (uint16_t*) checkAllocRead((char *)h5_cellId_field.c_str(), nframes, H5T_STD_U16LE, sizeof(uint16_t));

	// Status (H5T_STD_U16LE)
	statusIDlist = (uint16_t*) checkAllocRead((char *)h5_image_status_field.c_str(), nframes, H5T_STD_U16LE, sizeof(uint16_t));
	
	if(verbose) {
		std::cout << "\tDone reading vector fields in " << filename << "\n";
	}

};
// cAgipdReader::readHeaders


void cAgipdModuleReader::readDarkcal(char *filename){
	if (strcmp(filename, "No_file_specified") == 0)
	{
		return;
	}
	darkcalFilename = filename;

	printf("\tDarkcal file: %s\n", darkcalFilename.c_str());
	printf("\tGaincal file: %s\n", gaincalFilename.c_str());

	// checkAllocRead the dark setting field for this module into an array....
	//	[max-exfl014:barty]user/kuhnm/dark> h5ls -r dark_AGIPD00_agipd_2017-09-16.h5
	//	/                        Group
	//	/offset                  Dataset {3, 30, 128, 512}
	//	/stddev                  Dataset {3, 30, 128, 512}
	//	/threshold               Dataset {2, 30, 128, 512}
	
	// Check sanity of this line - it has not been edited for content!
	//calibDarkOffset = (float*) checkAllocRead((char *)h5_cellId_field.c_str(), nframes, H5T_STD_U16LE, sizeof(float*));

	calibrator = new cAgipdCalibrator(darkcalFilename, *this);
	calibrator->open();
}
// cAgipdModuleReader::readDarkcal


void cAgipdModuleReader::readGaincal(char *filename){
	if(strcmp(filename, "No_file_specified")) {
		return;
	}
	gaincalFilename = filename;
	
	// checkAllocRead the gain setting field for this module into an array....

	// Check sanity of this line - it has not been edited for content!  THink we may need hyperslab instead
	//calibDarkOffset = (float*) checkAllocRead((char *)h5_cellId_field.c_str(), nframes, H5T_STD_U16LE, sizeof(float*));

}
// cAgipdModuleReader::readDarkcal


void cAgipdModuleReader::readImageStack(void){
	
	if (noData) return;

	long mem_bytes = nframes*nn*sizeof(uint16_t);
	
	std::cout << "Reading entire image data array (all frames)\n" ;
	std::cout << "\tMemory required: " << mem_bytes / 1e6 << " (MB)" << std::endl ;

	std::cout << "A useful function perhaps, but broken. Please fix code and re-run." << std::endl;
	exit(1);

	// Read in all image data
//	if(data != NULL) free(data);
//	data = (uint16_t*) checkAllocRead(h5_image_data_field, nframes, H5T_STD_U16LE, sizeof(uint16_t));
//	std::cout << "\tDone reading " << filename << " (all frames)" << std::endl;
	
};
// cAgipdModuleReader::readImageStack


/*
 *	Read one frame of data
 */
void cAgipdModuleReader::readFrame(long frameNum){
	// Read a single image at position frameNum
	// Will have both fs and ss, and stack...

	if (noData) return;

	if(frameNum < 0 || frameNum >= nframes) {
		std::cout << "\treadFrame::frameNum out of bounds " << frameNum << std::endl;
		return;
	}
	
	if(verbose) {
		std::cout << "\tReading frame " << frameNum << " from " << filename << std::endl;
	}
	
	currentFrame = frameNum;

	// At the moment we only read RAW data files
	if (rawDetectorData)
	{
		readFrameRawOrCalib(frameNum, true);
	}
	else
	{
		readFrameRawOrCalib(frameNum, false);
	}
}
// cAgipdModuleReader::readFrame


/*
 *	Read one frame of data from RAW files
 */
void cAgipdModuleReader::readFrameRawOrCalib(long frameNum, bool isRaw)
{
	if (noData)
	{
		return;
	}

	// Define hyperslab in RAW data file
	hsize_t     slab_start[4];
	hsize_t		slab_size[4];
	slab_start[0] = frameNum;
	slab_start[1] = 0;
	slab_start[2] = 0;
	slab_start[3] = 0;
	slab_size[0] = 1;
	slab_size[1] = 1;
	slab_size[2] = n1;
	slab_size[3] = n0;

	if (!isRaw)
	{
		slab_size[1] = n1;
		slab_size[2] = n0;
	}

	int ndims = isRaw ? 4 : 3;
	hid_t type = isRaw ? H5T_STD_U16LE : H5T_IEEE_F32LE;
	hid_t size = isRaw ? sizeof(uint16_t) : sizeof(float);

	// Free existing memory
	free(data); data = NULL;
	free(digitalGain); digitalGain = NULL;

	if (rawDetectorData)
	{
		uint16_t *tempdata = NULL;
		tempdata = (uint16_t*) checkAllocReadHyperslab((char *)h5_image_data_field.c_str(), ndims, slab_start, slab_size, type, size);

		if (noData || !tempdata)
		{
			return;
		}

		data = (float *)malloc(n0 * n1 * sizeof(float));
		for (int i = 0; i < n0 * n1; i++) {
			data[i] = tempdata[i];
		}

		free(tempdata);
	}
	else
	{
		// Read data from hyperslab
		data = (float *) checkAllocReadHyperslab((char *)h5_image_data_field.c_str(), ndims, slab_start, slab_size, type, size);

		if (noData)
		{
			return;
		}
	}
	
	// Digital gain is in the second dimension (at least in the current layout)
	if(rawDetectorData) {
		slab_start[1] = 1;
		digitalGain = (uint16_t*) checkAllocReadHyperslab((char *)h5_image_data_field.c_str(), ndims, slab_start, slab_size, type, size);
}

	// Update timestamp, status bits and other stuff
	trainID = trainIDlist[frameNum];
	pulseID = pulseIDlist[frameNum];
	cellID = cellIDlist[frameNum];
	statusID = statusIDlist[frameNum];

	//	Apply calibration constants (if known).
	applyCalibration(frameNum);



};

// Apply calibration constants (if known) 
void cAgipdModuleReader::applyCalibration(long frameNum)
{
	if(calibrator == NULL)
		return;

	int thisCell = cellID / 2;
	int thisGain = 0;

	int16_t *offsets = calibrator->gainAndCellPtr(thisGain, thisCell);

	if (offsets == NULL)
	{
		return;
	}

	for (int i = 0; i < nn; i++)
	{
		/*
		if (i < 10)
		{
			std::cout << "Module pixel " << i << " subtracting " << offsets[i] << " from " << data[i] << std::endl;
		}
		 */

		data[i] -= offsets[i];
	}
}
//  cAgipdModuleReader::applyCalibration

