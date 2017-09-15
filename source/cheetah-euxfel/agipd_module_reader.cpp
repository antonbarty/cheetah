//
//  Created by Anton Barty on 24/8/17.
//	Distributed under the GPLv3 license
//


//#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <sstream>
#include "PNGFile.h"

#include "agipd_read.h"

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

	verbose = 0;
};

cAgipdModuleReader::~cAgipdModuleReader(){
	std::cout << "\tModule destructor called" << std::endl;
	cAgipdModuleReader::close();
};

/*
 *	Open HDF5 file and perfom some checks
 */
void cAgipdModuleReader::open(char filename[], int mNum){
	
	std::cout << "Opening " << filename << std::endl;
	cAgipdModuleReader::filename = filename;
	
	// Does the file exist?
	FILE *testfp;
	testfp = fopen(filename, "r");
	if (testfp == NULL) {
		printf("cAgipdReader::open: File does not exist (%s)\n",filename);
		exit(1);
	}
	fclose(testfp);
	
	
	// Is it an HDF5 file
	htri_t file_test;
	file_test = H5Fis_hdf5(filename);
	if(file_test == 0){
		printf("cAgipdReader::open: Not an HDF5 file (%s)\n",filename);
		exit(1);
	}
	
	// Open the file
	h5_file_id = H5Fopen(filename,H5F_ACC_RDONLY,H5P_DEFAULT);
	if(h5_file_id < 0){
		printf("cAgipdReader::open: Could not open HDF5 file (%s)\n",filename);
		exit(1);
	}
	//std::cout << "\tFile found\n";

	
	/*
	// Check whether needed data sets exist
	//H5LTpath_valid ( hid_t loc_id, const char *path, hbool_t check_object_valid)
	//if (!H5LTfind_dataset(h5_file_id, h5_trainId_field)) {
	if (!H5LTpath_valid(h5_file_id, h5_trainId_field, false)) {
		printf("cAgipdReader::open: No field named %s\n", h5_trainId_field);
		exit(1);
	}
	if (!H5LTfind_dataset(h5_file_id, h5_pulseId_field)) {
		printf("cAgipdReader::open: No field named %s\n", h5_pulseId_field);
		exit(1);
	}
	if (!H5LTfind_dataset(h5_file_id, h5_image_data_field)) {
		printf("cAgipdReader::open: No field named %s\n", h5_image_data_field);
		exit(1);
	}
	if (!H5LTfind_dataset(h5_file_id, h5_image_status_field)) {
		printf("cAgipdReader::open: No field named %s\n", h5_image_status_field);
		exit(1);
	}
	*/
	std::cout << "\tFile check OK\n";

	/* Now to make our new field names using the module number */

	std::string prefix = h5_instrument_prefix + i_to_str(mNum) + h5_image_prefix;
	h5_trainId_field = prefix + h5_trainId_suffix;

	h5_pulseId_field = prefix + h5_pulseId_suffix;

	h5_cellId_field = prefix + h5_cellId_suffix;

	h5_image_data_field = prefix + h5_image_data_suffix;

	h5_image_status_field = prefix + h5_image_status_suffix;

	if (true)
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
	H5LTget_dataset_info(h5_file_id, h5_image_data_field.c_str(), dims, &dataclass, &datasize);

	if (h5_file_id < 0)
	{
		std::cout << "Get dataset info for image - failed.\n";
		exit(0);
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
	if(data==NULL)
		return;
	if(h5_file_id==NULL)
		return;
	

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



void cAgipdModuleReader::readHeaders(void){

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



void* cAgipdModuleReader::checkAllocRead(char fieldName[], long targetnframes, hid_t h5_type_id, size_t targetsize) {
	int ndims;
	hsize_t dims[4];
	size_t	datasize;
	H5T_class_t dataclass;

	// Get size and type of this data set
	H5LTget_dataset_ndims(h5_file_id, fieldName, &ndims);
	H5LTget_dataset_info(h5_file_id, fieldName, dims, &dataclass, &datasize);
	
	if(verbose) {
		std::cout << "\t" << fieldName << "\n";
		printf("\tndims=%i  (",ndims);
		for(int i=0; i<ndims; i++) printf("%llu,", dims[i]);
		printf(")    dataclass=%i    datasize=%zu    targetsize=%zu\n", dataclass, datasize, targetsize);
	}
	
	// Check number of events
	if(dims[0] != targetnframes && verbose) {
		printf("\tSize of this data set does not match desired number of frames\n");
		printf("\tNumber of events in this dataset: %llu\n", dims[0]);
		printf("\tContinuing anyway, feel free to return NULL if you really want to fail\n");

		//	return NULL;
	}

	// Check size of data unit (crude way to check data types match)
	if(datasize != targetsize && verbose) {
		printf("\tSize of data elements does not match desired size\n");
		printf("\ttargetsize=%li, datasize=%li\n", targetsize, datasize);
		printf("\tContinuing anyway, feel free to return NULL if you really want to fail\n");
		//	return NULL;
	}

	// Number of data points to allocate
	long n=1;
	for(int i = 0;i<ndims;i++) {
		n *= dims[i];
	}

	// Debug check of array size
	if(false) {
		for(int i = 0;i<ndims;i++) printf("%llu x ", dims[i]);
		printf(" = %li\n", n);
	}

	// Allocate required memory
	void *databuffer = malloc(n*targetsize);
	
	
	// Read data space
	// herr_t H5LTread_dataset ( hid_t loc_id, const char *dset_name, hid_t type_id, void *buffer )
	herr_t herr;
	herr = H5LTread_dataset(h5_file_id, fieldName, h5_type_id, databuffer);
	if(herr < 0) {
		printf("\tError reading data set\n");
		return NULL;
	}
	
	return databuffer;
}
// cAgipdReader::checkAllocRead


void* cAgipdModuleReader::checkAllocReadHyperslab(char fieldName[], int ndims, hsize_t *slab_start, hsize_t *slab_size, hid_t h5_type_id, size_t targetsize) {
	
	
	// Open the dataset
	hid_t dataset_id;
	hid_t dataspace_id;
	dataset_id = H5Dopen(h5_file_id, fieldName, H5P_DEFAULT);
	dataspace_id = H5Dget_space(dataset_id);
	
	
	// Test dimensions of data
	int h5_ndims;
	h5_ndims = H5Sget_simple_extent_ndims(dataspace_id);
	hsize_t h5_dims[h5_ndims];
	H5Sget_simple_extent_dims(dataspace_id, h5_dims, NULL);
	
	//long nelements = 1;
	//for(int i = 0;i<ndims;i++)
	//	nelements *= dims[i];
	
	
	// Say something?
	if(false) {
		std::cout << "\tndims = " << ndims << std::endl;
		std::cout << "\tdims = ";
		for(int i = 0; i<h5_ndims;i++) std::cout << " " << h5_dims[i];
		std::cout << std::endl;
		std::cout << "\tfs,ss = " << n0 << "," << n1 << std::endl;
	}
	
	
	// Checks
	if(h5_ndims != ndims) {
		std::cout << "\tcheckAllocReadHyperslab error: dimensions of data sets do not match requested dimensions (oops)\n";
		H5Dclose(dataset_id);
		H5Sclose (dataspace_id);
		return NULL;
	}

	for(int i=0; i<h5_ndims; i++) {
		if(slab_start[i] < 0 || slab_start[i]+slab_size[i] > h5_dims[i]){
			std::cout << "\tcheckAllocReadHyperslab error: One array dimension runs out of bounds (oops), dim=" << i << std::endl;
			H5Dclose(dataset_id);
			H5Sclose (dataspace_id);
			return NULL;
		}
	}
	
	
	/*
	 *	Define and select the hyperslab to use for reading.
	 *
	 *	herr_t H5Sselect_hyperslab(hid_t space_id, H5S_seloper_t op, const hsize_t *start, const hsize_t *stride, const hsize_t *count, const hsize_t *block )
	 *	selection operator op determines how the new selection is to be combined with the already existing selection for the dataspace. H5S_SELECT_SET	Replaces the existing selection with the parameters from this call.
	 *	start array specifies the offset of the starting element of the specified hyperslab.
	 *	count array determines how many blocks to select from the dataspace, in each dimension.
	 *	stride array determines how many elements to move in each dimension.  If the stride parameter is NULL, a contiguous hyperslab is selected (as if each value in the stride array were set to 1).
	 *	block array determines the size of the element block selected from the dataspace. If the block parameter is set to NULL, the block size defaults to a single element in each dimension
	 */
	hsize_t		count[4];
	count[0] = 1;
	count[1] = 1;
	count[2] = 1;
	count[3] = 1;
	H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, slab_start, NULL, count, slab_size);
	
	
	// Allocate space
	long nelements = 1;
	for(int i = 0;i<ndims;i++)
		nelements *= slab_size[i];

	void *databuffer = malloc(nelements*targetsize);;
	
	
	// Define how to map the hyperslab into memory
	// See https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Read
	hid_t		memspace_id;
	hsize_t     start_mem[4];
	start_mem[0] = 0;
	start_mem[1] = 0;
	start_mem[2] = 0;
	start_mem[3] = 0;
	memspace_id = H5Screate_simple(ndims, slab_size, NULL);
	H5Sselect_hyperslab (memspace_id, H5S_SELECT_SET, start_mem, NULL, slab_size, NULL);
	
	
	/*
	 * Read the data using the previously defined hyperslab.
	 */
	H5Dread(dataset_id, h5_type_id, memspace_id, dataspace_id, H5P_DEFAULT, databuffer);
	
	
	// Cleanup
	H5Dclose(dataset_id);
	H5Sclose (dataspace_id);
	H5Sclose (memspace_id);

	// Return
	return databuffer;

}
// cAgipdReader::checkAllocReadHyperslab


void cAgipdModuleReader::readImageStack(void){
	
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

};

