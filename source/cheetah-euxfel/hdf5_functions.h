//
//  hdf5_functions.h
//  agipd
//
//  Created by Helen Ginn on 17/09/2017.
//  Copyright (c) 2017 Anton Barty. All rights reserved.
//

#ifndef __agipd__hdf5_functions__
#define __agipd__hdf5_functions__

#include <stdio.h>
#include <hdf5.h>
#include <hdf5_hl.h>

class cHDF5Functions
{
public:
	hid_t		h5_file_id;
	int	        verbose;
    bool        fileOK=false;
	bool        noData=true;

protected:
	void*		checkNeventsAllocRead(char[], long, hid_t, size_t);
	void*		checkAllocReadDataset(char[], int*, hsize_t*, hid_t, size_t);
	void*		checkAllocReadHyperslab(char[], int, hsize_t*, hsize_t*, hid_t, size_t);
	void 		getDatasetDims(char[], int*, hsize_t*);
	void 		getDatasetDims(char[], int*, hsize_t*, H5T_class_t*, size_t*);
	bool        fileCheckAndOpen(char *filename);	
};

#endif /* defined(__agipd__hdf5_functions__) */
