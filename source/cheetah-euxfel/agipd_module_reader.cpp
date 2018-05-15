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

static char h5_index_prefix[] = "/INDEX/SPB_DET_AGIPD1M-1/DET/";
static char h5_index_first_suffix[] = "CH0:xtdf/image/first";
static char h5_index_count_suffix[] = "CH0:xtdf/image/count";


// These two in processed data only
static char h5_image_gain_suffix[] = "gain";
static char h5_image_mask_suffix[] = "mask";


cAgipdModuleReader::cAgipdModuleReader(void){
	h5_file_id = 0;
	pulseIDlist = NULL;
	trainIDlist = NULL;
	cellIDlist = NULL;
	statusIDlist = NULL;
	data = NULL;
	digitalGain = NULL;
	badpixMask = NULL;
	rawDetectorData = true;
	noData = false;
	verbose = 0;
	
	gainDataOffset[0] = 0;
	gainDataOffset[1] = 1;

	_doNotApplyGainSwitch = false;

	calibGainFactor = NULL;

	calibrator = NULL;
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
void cAgipdModuleReader::open(char filename[], int mNum) {
	std::cout << "Opening " << filename << std::endl;
	cAgipdModuleReader::filename = filename;
	
	bool check = fileCheckAndOpen(filename);
	if (check == false) {
        std::cout << "\tWarning: Non-existent or dodgy file - will be skipped" << std::endl;
        // This means:
        // fileOK=false;
        // noData=true;
        return;
	}
	else {
        std::cout << "\tFile check OK\n";
	}

    
	/* Now to make our new field names using the module number */
	std::string imagePrefix = h5_instrument_prefix + i_to_str(mNum) + h5_image_prefix;
	h5_trainId_field = imagePrefix + h5_trainId_suffix;
	h5_pulseId_field = imagePrefix + h5_pulseId_suffix;
	h5_cellId_field = imagePrefix + h5_cellId_suffix;
	h5_image_data_field = imagePrefix + h5_image_data_suffix;
	h5_image_status_field = imagePrefix + h5_image_status_suffix;
    h5_image_gain_field = imagePrefix + h5_image_gain_suffix;
    h5_image_mask_field = imagePrefix + h5_image_mask_suffix;
    
    std::string indexPrefix = h5_index_prefix + i_to_str(mNum);
    h5_index_first_field = indexPrefix + h5_index_first_suffix;
    h5_index_count_field = indexPrefix + h5_index_count_suffix;
    
    
	if (false)
	{
		std::cout << h5_trainId_field << std::endl;
		std::cout << h5_pulseId_field << std::endl;
		std::cout << h5_cellId_field << std::endl;
		std::cout << h5_image_data_field << std::endl;
		std::cout << h5_image_status_field << std::endl;
        std::cout << h5_image_gain_field << std::endl;
        std::cout << h5_image_mask_field << std::endl;
        std::cout << h5_index_first_field << std::endl;
        std::cout << h5_index_count_field << std::endl;
	}

	std::string baseName = getBaseFilename(cAgipdModuleReader::filename);

    // Is this raw data which needs calibrating, or data already calibrated by XFEL?
	// If filename starts with 'RAW_' we have raw data
	if(baseName.substr(0,3) == "RAW") {
		std::cout << "\tData is RAW detector data" << std::endl;
		rawDetectorData = true;
	}
	else {
		std::cout << "\tData is NOT RAW (presuming it is calibrated)" << std::endl;
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
    std::cout << "\tHDF5 data field: " << h5_image_data_field <<std::endl;
	int success = H5LTget_dataset_info(h5_file_id, h5_image_data_field.c_str(), dims, &dataclass, &datasize);

	if (success < 0) {
		std::cout << "Get dataset info for image - failed. Module set to blank.\n";
        fileOK = false;
		noData = true;
		return;
	}

	nframes = dims[0];
	nstack = dims[1];
	n0 = dims[3];
	n1 = dims[2];

    // Layout of calibrated data is different to raw data
	if (!rawDetectorData) {
		n0 = dims[2];
		n1 = dims[1];
		nstack = 1;
	}
	nn = n0*n1;

    
    // Check the index length
    int success2 = H5LTget_dataset_info(h5_file_id, h5_index_first_field.c_str(), dims, &dataclass, &datasize);
    int success3 = H5LTget_dataset_info(h5_file_id, h5_index_count_field.c_str(), dims, &dataclass, &datasize);
    if (success2 < 0 || success3 < 0) {
        std::cout << "Get index info for image - failed. Module set to blank.\n";
        fileOK = false;
        noData = true;
        return;
    }
    
    nindex = dims[0];

    
	printf("\tNumber of frames: %li\n", nframes);
	printf("\tImage block size: %lix%li\n", n0, n1);
	printf("\tStack depth: %li\n", nstack);
    printf("\tIndex depth: %li\n", nindex);

};
// cAgipdReader::open


void cAgipdModuleReader::close(void) {
	if (noData || !fileOK)
        return;

	if(data==NULL)
		return;
	if(h5_file_id==NULL)
		return;

	if (calibrator != NULL) {
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
		free(badpixMask);
		
		// Pointers to NULL
		pulseIDlist = NULL;
		trainIDlist = NULL;
		cellIDlist = NULL;
		statusIDlist = NULL;
		data = NULL;
		digitalGain = NULL;
		badpixMask = NULL;
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
	if (noData)
        return;

	//std::cout << "Reading headers " << filename << "\n";
	std::cout << "Reading vector fields from " << filename << "\n";
	
	// PulseID (H5T_STD_U64LE)
	pulseIDlist = (uint64_t*) checkNeventsAllocRead((char *)h5_pulseId_field.c_str(), nframes, H5T_STD_U64LE, sizeof(uint64_t));

	// TrainID (H5T_STD_U64LE)
	trainIDlist = (uint64_t*) checkNeventsAllocRead((char *)h5_trainId_field.c_str(), nframes, H5T_STD_U64LE, sizeof(uint64_t));

	// CellID (H5T_STD_U64LE)
	cellIDlist = (uint16_t*) checkNeventsAllocRead((char *)h5_cellId_field.c_str(), nframes, H5T_STD_U16LE, sizeof(uint16_t));

	// Status (H5T_STD_U16LE)
	statusIDlist = (uint16_t*) checkNeventsAllocRead((char *)h5_image_status_field.c_str(), nframes, H5T_STD_U16LE, sizeof(uint16_t));
	
	if(verbose) {
		std::cout << "\tDone reading vector fields in " << filename << "\n";
	}

};
// cAgipdReader::readHeaders


void cAgipdModuleReader::readDarkcal(char *filename){
    // If file is absent or not OK, no need to load gains as we will skip anyway
    if(!fileOK)
        return;
    
    // No dark specified?
    if (strcmp(filename, "No_file_specified") == 0) {
		return;
	}
	darkcalFilename = filename;

	printf("\tDarkcal file: %s\n", darkcalFilename.c_str());
	printf("\tGaincal file: %s\n", gaincalFilename.c_str());


	calibrator = new cAgipdCalibrator(darkcalFilename, *this);
	calibrator->readCalibrationData();
}
// cAgipdModuleReader::readDarkcal


void cAgipdModuleReader::readGaincal(char *filename){
    // If file is absent or not OK, no need to load gains as we will skip anyway
    if(!fileOK)
        return;

    // No gain specified?
    if(strcmp(filename, "No_file_specified")) {
		return;
	}

	gaincalFilename = filename;

	std::cout << "Oops... Nothing implemented for reading gain.." << std::endl;
	exit(1);
}
// cAgipdModuleReader::readGaincal


void cAgipdModuleReader::readImageStack(void){
	
	if (noData)
        return;

    
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

    if (noData) {
        return;
    }

	if(frameNum < 0 || frameNum >= nframes) {
		std::cout << "\treadFrame::frameNum out of bounds " << frameNum << std::endl;
		return;
	}
	
	if(verbose) {
		std::cout << "\tReading frame " << frameNum << " from " << filename << std::endl;
	}
	
	currentFrame = frameNum;

	// Update timestamp, status bits and other stuff
	trainID = trainIDlist[frameNum];
	pulseID = pulseIDlist[frameNum];
	cellID = cellIDlist[frameNum];
	statusID = statusIDlist[frameNum];
	

	// Read the data frame
	if (rawDetectorData) {
		readFrameRaw(frameNum);
	}
	else {
		readFrameXFELCalib(frameNum);
	}
}
// cAgipdModuleReader::readFrame


/*
 *	Read one frame of data from RAW files
 */
void cAgipdModuleReader::readFrameRaw(long frameNum) {
    if (noData) {
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
	int ndims = 4;
	
	// Free existing memory
	free(data); data = NULL;
	free(digitalGain); digitalGain = NULL;
	free(badpixMask); badpixMask = NULL;


    // Read data from hyperslab in RAW data file (which is unit16_t, so convert it to float)
	uint16_t *tempdata = NULL;
	tempdata = (uint16_t*) checkAllocReadHyperslab((char *)h5_image_data_field.c_str(), ndims, slab_start, slab_size, H5T_STD_U16LE, sizeof(uint16_t));
	
    if (!tempdata) {
		return;
	}
    
    // Convert uint16_t to float
	data = (float *)malloc(n0 * n1 * sizeof(float));
	for (int i = 0; i < n0 * n1; i++) {
		data[i] = tempdata[i];
	}
	free(tempdata);
	
	// Digital gain is in the second dimension (at least that's the way it was meant to be)
	// For the first few experiments digital gain is actually in the next analog memory location: location configured via gainDataOffset
	slab_start[0] += gainDataOffset[0];
	slab_start[1] += gainDataOffset[1];
	digitalGain = (uint16_t*) checkAllocReadHyperslab((char *)h5_image_data_field.c_str(), ndims, slab_start, slab_size, H5T_STD_U16LE, sizeof(uint16_t));

	// Bad pixel mask added by raw data calibration, or left alone if uncalibrated
    // Pixel good = 0, pixel bad = anything else; deliberate use of calloc() to zero the array
	badpixMask = (uint16_t *)calloc(nn, sizeof(uint16_t));

	
	// Update timestamp, status bits and other stuff
	trainID = trainIDlist[frameNum];
	pulseID = pulseIDlist[frameNum];
	cellID = cellIDlist[frameNum];
	statusID = statusIDlist[frameNum];
	
	
	//	Apply calibration constants (if known).
	applyCalibration(frameNum);
};
// cAgipdModuleReader::readFrameRaw


/*
 *	Read a single frame of XFEL calibrated data
 *	usually found in {$EXPT}/proc
 *  as provided by Steffen Hauf's calibration routines
 */
void cAgipdModuleReader::readFrameXFELCalib(long frameNum) {
	if (noData) {
		return;
	}
	
	// Define hyperslab in RAW data file
	hsize_t     slab_start[4];
	hsize_t		slab_size[4];
	slab_start[0] = frameNum;
	slab_start[1] = 0;
	slab_start[2] = 0;
	slab_size[0] = 1;
	slab_size[1] = n1;
	slab_size[2] = n0;
	int ndims = 3;
	
	// Free existing memory
	free(data); data = NULL;
	free(digitalGain); digitalGain = NULL;
	free(badpixMask); badpixMask = NULL;

    //std::cout << "ndims=" << ndims << ", size=[" << slab_size[0] << ", " << slab_size[1] << ", " << slab_size[2] << std::endl;
    
	// Read data directly from hyperslab in corrected data file (which is already a float)
	data = (float *) checkAllocReadHyperslab((char *)h5_image_data_field.c_str(), ndims, slab_start, slab_size, H5T_IEEE_F32LE, sizeof(float));
    if (!data) {
        return;
    }

	// Digital gain is in a different field and is H5T_STD_U8LE Dataset {7500, 512, 128}
	// Default format is uint16_t so we must convert
	uint8_t *tempgain = NULL;
	tempgain = (uint8_t*) checkAllocReadHyperslab((char *)h5_image_gain_field.c_str(), ndims, slab_start, slab_size, H5T_STD_U8LE, sizeof(uint8_t));
	digitalGain = (uint16_t *)malloc(nn * sizeof(uint16_t));
	for (int i = 0; i < nn; i++) {
		digitalGain[i] = tempgain[i];
	}
	free(tempgain);

	
	// Bad pixel mask is a H5T_STD_U8LE Dataset {7500, 512, 128, 3}  <--- Not any more
	//slab_start[3] = 0;
	//slab_size[3] = 3;
	//ndims = 4;
	uint8_t *tempmask = NULL;
	tempmask = (uint8_t*) checkAllocReadHyperslab((char *)h5_image_mask_field.c_str(), ndims, slab_start, slab_size, H5T_STD_U8LE, sizeof(uint8_t));
	badpixMask = (uint16_t *)calloc(nn, sizeof(uint16_t));
    // Copy across mask
    long nbad = 0;
    for (long i = 0; i < nn; i++) {
        if(tempmask[i] != 0) {
            data[i] = 0;
            badpixMask[i] = 1;
            nbad++;
        }
    }
    free(tempmask);

    
    // Check for screwy intensity values: Sometimes we get +/- 1e9 appearing
    // Bad form to hard code this, but for now it's just a test case
    if(false) {
        for (long i = 0; i < nn; i++) {
            if(data[i] > 1e7 || data[i] < -1e6) {
                data[i] = 0;
                badpixMask[i] = 1;
            }
        }
    }
    
	// Update timestamp, status bits and other stuff
	trainID = trainIDlist[frameNum];
	pulseID = pulseIDlist[frameNum];
	cellID = cellIDlist[frameNum];
	statusID = statusIDlist[frameNum];
};
// cAgipdModuleReader::readFrameXFELCalib



// Apply calibration constants (if known)
// A wrapper for function moved to agipd_calibrator (maybe remove later)
// void cAgipdCalibrator::applyCalibration(int cellID, float *aduData, uint16_t *gainData){...}
void cAgipdModuleReader::applyCalibration(long frameNum) {
    
    // Apply calibrations to raw data, skip if we have pre-calibrated data
    if(rawDetectorData == false)
        return;
    
    // No calibrator = no calibration; return and zero out digital gain
    if(calibrator == NULL) {
        memset(digitalGain, 0, nn*sizeof(uint16_t));
		return;
    }

	// Cell ID for this frame number
	// In this scheme, digital and analog are interleaved - the real calibration constant is in cellID/2
	cellID = cellIDlist[frameNum];
	
	int thisCell = cellID;
    
    // For interleaved gain data (2017) we need to apply this correction to the cellID
	if(cellIDcorrection	!= 1 && cellIDcorrection != 0) {
		thisCell = cellID / cellIDcorrection;
	};

    
	// Apply calibrator for this cell
	calibrator->applyCalibration(thisCell, data, digitalGain, badpixMask);

}
//  cAgipdModuleReader::applyCalibration

