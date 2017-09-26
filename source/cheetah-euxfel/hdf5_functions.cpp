//
//  hdf5_functions.cpp
//  agipd
//
//  Created by Helen Ginn on 17/09/2017.
//


#include "hdf5_functions.h"
#include <cstdlib>
#include <iostream>
#include <sstream>

/*
 *	Read dataset, whatever its dimensions may be
 * 	Must pass &ndims and pointer hsize_t *dims[ndims]
 *	Returns void* pointer to read data.
 *	Also updates &ndims, dims[ndims] and checks data size matches what you are expecting
 *	Watch for memory leaks in the routines that call this function - we can not check whether the
 *	variable to which the return value is assigned has already been allocated
 *	Also watch for reading huge datasets when you don't have enough memory (we don't check...)
 */
void* cHDF5Functions::allocReadDataset(char fieldName[], int *h5_ndims, hsize_t *h5_dims, hid_t h5_type_id, size_t targetsize){
	
	int success1, success2;
	size_t	datasize;
	H5T_class_t dataclass;

	// get dataset size, dimensions, datatype, etc...
	success1 = H5LTget_dataset_ndims(h5_file_id, fieldName, h5_ndims);
	success2 = H5LTget_dataset_info(h5_file_id, fieldName, h5_dims, &dataclass, &datasize);
	if (success1 < 0 || success2 < 0) {
		std::cout << "Error: could not determine data sizes of HDF5 field " << fieldName << std::endl;
		std::cout << "Returning NULL pointer (which will cause something to fail upstream)" << std::endl;
		return NULL;
	}

	
	// local copy of h5_ndims 
	int ndims = *(h5_ndims);
	hsize_t *dims = h5_dims;

	
	// Check size of actual data and requested size (crude way to check data types match)
	if(datasize != targetsize) {
		printf("\tSize of data elements does not match desired size\n");
		printf("\ttargetsize=%li, datasize=%li\n", targetsize, datasize);
		printf("\tContinuing anyway, feel free to return NULL if you really want to fail\n");
		//	return NULL;
	}

	
	// Number of data points to allocate is product of all dimensions
	long nelements=1;
	for(int i = 0;i<ndims;i++) {
		nelements *= dims[i];
	}
	
	// Throw an error if dataset is suspiciously large (>50GB)
	long max_gb = 50;
	long long gb = (1024L)^3;
	long long max_mem = max_gb*gb;
	if(nelements*targetsize > max_mem){
		std::cout << "Error: Suspiciously large memory request:" << std::endl;
		std::cout << nelements*targetsize << " bytes exceeds " << max_gb << "GB" << std::endl;
		exit(1);
	};
	

	// Allocate required memory
	// You will have a memory leak if variable assigned to this function is already allocated
	void *databuffer = malloc(nelements*targetsize);

	
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

/*
 *	Read dataset, and check whether it has the required number of entries (which is found in h5_dims[0]
 */
void* cHDF5Functions::checkNeventsAllocRead(char fieldName[], long targetnframes, hid_t h5_type_id, size_t targetsize) {
	int h5_ndims;
	hsize_t h5_dims[4];
	size_t	datasize;
	H5T_class_t dataclass;
	int success;
	
	// Get size and type of this data set
	success = H5LTget_dataset_ndims(h5_file_id, fieldName, &h5_ndims);
	if (success < 0) {
		noData = true;
		return NULL;
	}

	H5LTget_dataset_info(h5_file_id, fieldName, h5_dims, &dataclass, &datasize);
	if (success < 0) {
		noData = true;
		return NULL;
	}

	// Diagnostic: print dimensions and data class
	if(verbose) {
		std::cout << "\t" << fieldName << "\n";
		printf("\tndims=%i  (",h5_ndims);
		for(int i=0; i<h5_ndims; i++) printf("%llu,", h5_dims[i]);
		printf(")    dataclass=%i    datasize=%zu    targetsize=%zu\n", dataclass, datasize, targetsize);
	}

	// Check number of events, found in dims[0]
	if(h5_dims[0] != targetnframes && verbose) {
		printf("\tSize of this data set does not match desired number of frames\n");
		printf("\tNumber of events in this dataset: %llu\n", h5_dims[0]);
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
	long nelements=1;
	for(int i = 0;i<h5_ndims;i++) {
		nelements *= h5_dims[i];
	}

	// Debug check of array size
	if(/* DISABLES CODE */ (false)) {
		for(int i = 0;i<h5_ndims;i++) printf("%llu x ", h5_dims[i]);
		printf(" = %li\n", nelements);
	}

	// Allocate required memory
	void *databuffer = malloc(nelements*targetsize);


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

/*
 *	Returns pointer to selected hyperslab of data
 *	Does not (can not) free the return pointer so handle memory leaks in the routine that call this function
 */
void* cHDF5Functions::checkAllocReadHyperslab(char fieldName[], int ndims, hsize_t *slab_start, hsize_t *slab_size, hid_t h5_type_id, size_t targetsize) {


	// Open the dataset
	hid_t dataset_id;
	hid_t dataspace_id;
	dataset_id = H5Dopen(h5_file_id, fieldName, H5P_DEFAULT);
	if (dataset_id < 0) {
		noData = true;
		return NULL;
	}
	dataspace_id = H5Dget_space(dataset_id);


	// Test dimensions of data
	int h5_ndims;
	h5_ndims = H5Sget_simple_extent_ndims(dataspace_id);
	hsize_t h5_dims[h5_ndims];
	H5Sget_simple_extent_dims(dataspace_id, h5_dims, NULL);

	//long nelements = 1;
	//for(int i = 0;i<ndims;i++)
	//	nelements *= dims[i];

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

bool cHDF5Functions::fileCheckAndOpen(char *filename)
{
	// Does the file exist?
	FILE *testfp;
	testfp = fopen(filename, "r");
	if (testfp == NULL) {
		printf("cAgipdReader::open: File does not exist (%s)\n", filename);
		printf("Module set to blank.\n");
        fileOK = false;
		noData = true;
		return false;
	}
	else
	{
		fclose(testfp);
	}

	// Is it an HDF5 file
	htri_t file_test;
	file_test = H5Fis_hdf5(filename);
	if(file_test == 0){
		printf("cAgipdReader::open: Not an HDF5 file (%s)\n",filename);
		printf("Module set to blank.\n");
        fileOK = false;
        noData = true;
		return false;
	}

	// Open the file
	h5_file_id = H5Fopen(filename,H5F_ACC_RDONLY,H5P_DEFAULT);
	if(h5_file_id < 0){
		printf("cAgipdReader::open: Could not open HDF5 file (%s)\n",filename);
		printf("Module set to blank.\n");
		noData = true;
		return false;
	}
	//std::cout << "\tFile found\n";
    
    // If we get this far without error...
    fileOK = true;
    noData = false;
	return true;
}
