//
//  Created by Anton Barty on 24/8/17.
//	Distributed under the GPLv3 license
//

#ifndef agipd_h5_hpp
#define agipd_h5_hpp

#include <iostream>
#include <string>
#include <vector>
#include <hdf5.h>
#include <hdf5_hl.h>


/*
 *	This class handles reading and writing for one AGIPD module file
 *	(each module is in a separate file)
 */
class cAgipdModuleReader {
	
public:
	cAgipdModuleReader();
	~cAgipdModuleReader();

	void open(char[]);
	void close(void);
	void readHeaders(void);
	void readImageStack(void);
	void readFrame(long);

// Pubic variables
public:
	long		nframes;
	long		nstack;
	long		n0;
	long		n1;
	long		nn;

	// data and ID for the last read event.  Only updated after readFrame is called! 
	uint16_t	*data;
	uint16_t	*digitalGain;
    uint64_t	trainID;
    uint64_t	pulseID;
    uint16_t	cellID;
    uint16_t	statusID;

	// List of all IDs in file (lookup table, useful to have all in memory)
    uint64_t	*trainIDlist;
    uint64_t	*pulseIDlist;
    uint16_t	*cellIDlist;
    uint16_t	*statusIDlist;

	int			verbose;
	bool		rawDetectorData;

	
// Private variables
private:
	std::string	filename;
	hid_t		h5_file_id;
	long		currentFrame;
	
// Private functions
private:
	void*		checkAllocRead(char[], long, hid_t, size_t);
	void*		checkAllocReadHyperslab(char[], int, hsize_t*, hsize_t*, hid_t, size_t);
	void		readFrameRAW(long);
};




/*
 *	This class reads and assembles all 16 AGIPD modules into one array for one event
 */
class cAgipdReader {

public:
    static const int nAGIPDmodules = 16;

    
public:
	cAgipdReader();
	~cAgipdReader();
	
	void open(char*);
	void close(void);
	void readFrame(long);

	
public:
	// Data slab dimensions
	long		nframes;
	
	// Dimenations and of the composite data slab
	long		dims[2];
	long		n0;
	long		n1;
	long		nn;
	uint16_t	*data;
	uint16_t	*digitalGain;
	uint16_t	*mask;
	
	// Metadata for this event
	uint64_t	trainID;
	uint64_t	pulseID;
	uint16_t	cellID[nAGIPDmodules];
	uint16_t	statusID[nAGIPDmodules];
	
	// Module dimensions (how modules are placed in the 2D slab of data)
	long		nmodules[2];
	long		modulen[2];
	long		modulenn;
	
	// Pointer to the data location for each module for easy memcpy()
	// and those who want to look at the data as a stack of panels
	uint16_t*	pdata[nAGIPDmodules];
	uint16_t*	pgain[nAGIPDmodules];
	uint16_t*	pmask[nAGIPDmodules];
	
	int			verbose;
	bool		rawDetectorData;
	
	
private:
	void				generateModuleFilenames(char[]);

	long				currentFrame;
	std::string			moduleFilename[nAGIPDmodules];
	cAgipdModuleReader	module[nAGIPDmodules];
	bool				moduleOK[nAGIPDmodules];
	
};


#endif /* agipd_h5_hpp */
