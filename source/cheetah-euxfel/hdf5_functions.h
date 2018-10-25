//
//  hdf5_functions.h
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

#ifndef __agipd__hdf5_functions__
#define __agipd__hdf5_functions__

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <hdf5.h>
#include <hdf5_hl.h>

class cHDF5Functions
{
public:
	hid_t		h5_file_id;
	int	        verbose;
    bool        fileOK=false;
	bool        noData=true;
    bool        useNewDatasetReader=true;
    bool        use_h5LT=true;      // H5LT is convenient but not threadsafe
    
    long        h5_minCacheSize = 2*1024*1024;
    long        h5_ridiculousCacheSize=200*1024*1024;

protected:
    hid_t        fileCheckAndOpen(char *filename);
    void*        checkNeventsAllocRead(char[], long, hid_t, size_t);
    void*        checkAllocReadDataset(char[], int*, hsize_t*, hid_t, size_t);
    void*        checkAllocReadHyperslab(char[], int, hsize_t*, hsize_t*, hid_t, size_t);
    void         getDatasetDims(char[], int*, hsize_t*);
    void         getDatasetDims(char[], int*, hsize_t*, H5T_class_t*, size_t*);
    
    
};
// end cHDF5Functions

/*
 *  This class duplicates some of the above functionalty but enables the dataset
 *  to remain open across the reading of multiple images
 */
class cHDF5dataset : public cHDF5Functions {
public:
    bool     datasetOK=false;
    
    cHDF5dataset();
    ~cHDF5dataset();
    
    void    open(char[],char[]);
    void    setChunkCacheSize(void);
    void*   checkAllocReadHyperslab(int, hsize_t*, hsize_t*, hid_t, size_t);
    void    close(void);
    
private:
    hid_t       h5_file_id;
    hid_t       h5_dataset_id;
    hid_t       h5_dataspace_id;
    hid_t       h5_dapl;        // dapl = Data Access Property List
    int         h5_ndims;
    hsize_t     *h5_dims;
    hid_t       h5_type;
    size_t      h5_size;

    std::string h5_filename;
    std::string h5_fieldname;
    
    long        h5_chunksize;
    long        h5_chunksize_bytes;
};
// end cHDF5dataset

    


#endif /* defined(__agipd__hdf5_functions__) */
