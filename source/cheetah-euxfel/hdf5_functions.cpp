//
//  hdf5_functions.cpp
//  agipd
//
//  Created by Helen Ginn on 17/09/2017.
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

#include "hdf5_functions.h"
#include <cstdlib>
#include <iostream>
#include <sstream>



/*
 *	Get dataset dimensions (but do not read the data)
 */
void cHDF5Functions::getDatasetDims(char fieldName[], int *h5_ndims, hsize_t *h5_dims){
	// Need these to call the real function, then throw them away
	size_t	datasize;
	H5T_class_t dataclass;
	
	// Now read the data set dimensions
	cHDF5Functions::getDatasetDims(fieldName, h5_ndims, h5_dims, &dataclass, &datasize);
}
	
/*
 *	Get dataset dimensions, dataclass and element size
 */
void cHDF5Functions::getDatasetDims(char fieldName[], int *h5_ndims, hsize_t *h5_dims, H5T_class_t *dataclass, size_t *datasize){

	// Need these variables
	int success1, success2;

	// get dataset size, dimensions, datatype, etc...
	success1 = H5LTget_dataset_ndims(h5_file_id, fieldName, h5_ndims);
	success2 = H5LTget_dataset_info(h5_file_id, fieldName, h5_dims, dataclass, datasize);
	if (success1 < 0 || success2 < 0) {
		std::cout << "Error: could not determine data sizes of HDF5 field " << fieldName << std::endl;
	}
}


/*
 *	Read dataset, whatever its dimensions may be
 * 	Must pass &ndims and pointer hsize_t *dims[ndims]
 *	Returns void* pointer to read data.
 *	Also updates &ndims, dims[ndims] and checks data size matches what you are expecting
 *	Watch for memory leaks in the routines that call this function - we can not check whether the
 *	variable to which the return value is assigned has already been allocated
 *	Also watch for reading huge datasets when you don't have enough memory (we don't check...)
 */
void* cHDF5Functions::checkAllocReadDataset(char fieldName[], int *h5_ndims, hsize_t *h5_dims, hid_t h5_type_id, size_t targetsize){
	
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

	// Diagnostic
	if(true) {
		std::cout << "\tField name: " << fieldName << std::endl;
		std::cout << "\tndims = " << ndims << ", size = [";
		for(int i=0; i<ndims; i++) {
			std::cout << dims[i];
			if(i < ndims-1) std::cout << ", ";
		}
		std::cout << "]" << std::endl;
	}
	
	// 	Check size of actual data and requested size:
	//	Quick way to avoid a segmentation fult, and crude way to check data types match
	if(datasize != targetsize) {
		printf("\tSize of data elements does not match desired size\n");
		printf("\t%s\n", fieldName);
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
	long long gb = (1024*1024*1024);
	long long max_mem = max_gb*gb;
	if(nelements*targetsize > max_mem){
		std::cout << "Error: Suspiciously large memory request:" << std::endl;
		std::cout << nelements << "x" << targetsize << " bytes exceeds " << max_gb << "GB" << std::endl;
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
        std::cout << "\tndims=" << ndims << ", h5_ndims=" << h5_ndims << std::endl;
        std::cout << "\tIn field " << fieldName << std::endl;
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

hid_t cHDF5Functions::fileCheckAndOpen(char *filename)
{
	// Does the file exist?
	FILE *testfp;
	testfp = fopen(filename, "r");
	if (testfp == NULL) {
		printf("cAgipdReader::open: File does not exist (%s)\n", filename);
		printf("Module set to blank.\n");
        fileOK = false;
		noData = true;
		return NULL;
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
		return NULL;
	}

    // Set a more sensible cache size
    long    sensibleCacheSize = 64*1024*1024;
    hid_t dapl = H5Pcreate(H5P_FILE_ACCESS);
    //H5Pset_chunk_cache(dapl, H5D_CHUNK_CACHE_NSLOTS_DEFAULT, 256*1024*1024, H5D_CHUNK_CACHE_W0_DEFAULT);
    //H5Pset_chunk_cache(dapl_id, 12421, 16*1024*1024, H5D_CHUNK_CACHE_W0_DEFAULT);
    H5Pset_cache(dapl, NULL, 12421, sensibleCacheSize, 1);
    printf("\tFile cache set to %0.1f MB\n", sensibleCacheSize/(1024.*1024.));

    
    
	// Open the file
    hid_t file_id;
	//file_id = H5Fopen(filename,H5F_ACC_RDONLY,H5P_DEFAULT);
    file_id = H5Fopen(filename,H5F_ACC_RDONLY,dapl);
	if(file_id < 0){
		printf("cAgipdReader::open: Could not open HDF5 file (%s)\n",filename);
		printf("Module set to blank.\n");
		noData = true;
		return NULL;
	}
	//std::cout << "\tFile found\n";
    
    // If we get this far without error...
    fileOK = true;
    noData = false;
	return file_id;
}


/*
 *  Functions related to cHDF5dataset
 */

cHDF5dataset::cHDF5dataset() {
    
}
cHDF5dataset::~cHDF5dataset(){
    close();
}

// Open dataset
void cHDF5dataset::open(char *filename, char *dataset) {
    
    // Open and check file
    h5_file_id = fileCheckAndOpen(filename);
    if (!h5_file_id) {
        datasetOK = false;
        return;
    }
    
    h5_filename = filename;
    h5_fieldname = dataset;

    
    // Open the dataset
    h5_dataset_id = H5Dopen(h5_file_id, dataset, H5P_DEFAULT);
    if (h5_dataset_id < 0) {
        datasetOK = false;
    }
    h5_dataspace_id = H5Dget_space(h5_dataset_id);
    
    
    // Retrieve dataset dimensions
    h5_ndims = H5Sget_simple_extent_ndims(h5_dataspace_id);
    h5_dims = (hsize_t*) calloc(h5_ndims, sizeof(hsize_t));
    H5Sget_simple_extent_dims(h5_dataspace_id, h5_dims, NULL);
    
    // Size and type of one element
    hid_t h5_type;
    h5_type = H5Dget_type(h5_dataset_id);
    h5_size = H5Tget_size(h5_type);
    
    // Set this only once all has opened OK
    datasetOK = true;
    
    
    // Debugging
    if(true) {
        printf("Opening dataset\n");
        printf("\tFile %s\n", filename);
        printf("\tDataset %s\n", dataset);
        printf("\tDimensions (");
        for(int i=0; i<h5_ndims; i++, printf(" x ")) printf("%li", h5_dims[i]);
        //for(int i=0; i<h5_ndims; i++) printf("%li x ", h5_dims[i]);
        printf(") of size %i bytes\n", h5_size);
    }
}


void cHDF5dataset::setChunkCacheSize(void){
    
    
    // Is the dataset chuncked to begin with?
    hid_t plist;
    plist = H5Dget_create_plist(h5_dataset_id);
    if (H5D_CHUNKED != H5Pget_layout(plist)) {
        printf("\tDataset %s is not chunked\n", h5_fieldname.c_str());
        return;
    }
    
    // If so, find out the chunk dimensions
    int chunk_ndims;
    hsize_t chunk_dims[10];
    chunk_ndims = H5Pget_chunk(plist, 10, chunk_dims);
    printf("\tChunk dimensions (");
    for(int i=0; i<chunk_ndims; i++) printf("%li x ", chunk_dims[i]);
    printf(")");
    
    
    // Figure out required chunk memory
    long    chunk_mem=1;
    for(int i=0; i<chunk_ndims; i++) chunk_mem *= chunk_dims[i];
    chunk_mem *= h5_size;
    printf(" = %0.1f MB \n", (float) chunk_mem / (1024.*1024.));
    
    // Make sure we allocate a sensible cache size
    chunk_mem *= 2;
    if(chunk_mem < h5_minCacheSize) {
        chunk_mem = h5_minCacheSize;
        printf("\tchunk_mem less than a sensible size, will set to %li bytes\n", chunk_mem);
    }
    if(chunk_mem > h5_ridiculousCacheSize) {
        chunk_mem = h5_ridiculousCacheSize;
        printf("\tchunk_mem more than a sensible size, will set to %li bytes\n", chunk_mem);
    }
    //printf("\tWill set chunk buffer to %0.1f MB \n", (float) chunk_mem / (1024.*1024.));

    
    // Looks like we need to close and re-open the dataset with a new chunck cache.  So be it...
    H5Dclose(h5_dataset_id);
    H5Sclose(h5_dataspace_id);
    

    // Set up property list
    hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
    H5Pset_chunk_cache(dapl, 12421, chunk_mem, 1);
    //H5Pset_chunk_cache(dapl, 12421, chunk_mem, H5D_CHUNK_CACHE_W0_DEFAULT);
    //H5Pset_chunk_cache(dapl, H5D_CHUNK_CACHE_NSLOTS_DEFAULT, 256*1024*1024, H5D_CHUNK_CACHE_W0_DEFAULT);
    
    
    // Open the dataset again with new property list
    h5_dataset_id = H5Dopen(h5_file_id, (char*) h5_fieldname.c_str(), dapl);
    if (h5_dataset_id < 0) {
        datasetOK = false;
        return;
    }
    h5_dataspace_id = H5Dget_space(h5_dataset_id);

    printf("\tDataset reopened with dataset cache of %0.1f MB \n", (float) chunk_mem / (1024.*1024.));

}


void* cHDF5dataset::checkAllocReadHyperslab(int ndims, hsize_t *slab_start, hsize_t *slab_size, hid_t h5_type_id, size_t targetsize){
    
    
    // Checks
    if(h5_ndims != ndims) {
        std::cout << "\tcheckAllocReadHyperslab error: dimensions of data sets do not match requested dimensions (oops)\n";
        std::cout << "\tndims=" << ndims << ", h5_ndims=" << h5_ndims << std::endl;
        std::cout << "\tIn field " << h5_fieldname << std::endl;
        return NULL;
    }
    
    for(int i=0; i<h5_ndims; i++) {
        if(slab_start[i] < 0 || slab_start[i]+slab_size[i] > h5_dims[i]){
            std::cout << "\tcheckAllocReadHyperslab error: One array dimension runs out of bounds (oops), dim=" << i << std::endl;
            return NULL;
        }
    }
    
    

    /*
     *    Define and select the hyperslab to use for reading.
     *
     *    herr_t H5Sselect_hyperslab(hid_t space_id, H5S_seloper_t op, const hsize_t *start, const hsize_t *stride, const hsize_t *count, const hsize_t *block )
     *    selection operator op determines how the new selection is to be combined with the already existing selection for the dataspace. H5S_SELECT_SET    Replaces the existing selection with the parameters from this call.
     *    start array specifies the offset of the starting element of the specified hyperslab.
     *    count array determines how many blocks to select from the dataspace, in each dimension.
     *    stride array determines how many elements to move in each dimension.  If the stride parameter is NULL, a contiguous hyperslab is selected (as if each value in the stride array were set to 1).
     *    block array determines the size of the element block selected from the dataspace. If the block parameter is set to NULL, the block size defaults to a single element in each dimension
     */
    hsize_t        count[4];
    count[0] = 1;
    count[1] = 1;
    count[2] = 1;
    count[3] = 1;
    H5Sselect_hyperslab(h5_dataspace_id, H5S_SELECT_SET, slab_start, NULL, count, slab_size);
    
    
    // Allocate space into which data will be read
    long nelements = 1;
    for(int i = 0;i<ndims;i++)
        nelements *= slab_size[i];
    
    void *databuffer = malloc(nelements*targetsize);;
    
    
    // Define how to map the hyperslab into memory
    // See https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Read
    hid_t        memspace_id;
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
    H5Dread(h5_dataset_id, h5_type_id, memspace_id, h5_dataspace_id, H5P_DEFAULT, databuffer);
    
    
    // Cleanup
    H5Sclose (memspace_id);
    
    // Return
    return databuffer;

    
}




void cHDF5dataset::close(void){
    
    // Do nothing on closing a non-open dataset
    if(datasetOK == false) {
        return;
    }
    
    // Close datasets
    H5Dclose(h5_dataset_id);
    H5Sclose(h5_dataspace_id);
    
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
    
    datasetOK = false;
};
