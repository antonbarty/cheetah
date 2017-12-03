BOOST_HEADER_PATH=/usr/include
CCP4_HEADER_PATH=/gpfs/cfel/cxi/common/cfelsoft-rh7/ccp4/ccp4-7.0/include/ccp4
HDF5_HEADER_PATH=/usr/include
PNG_HEADER_PATH=/usr/include

BOOST_LIBRARY_PATH=/usr/lib64
CCP4_LIBRARY_PATH=/gpfs/cfel/cxi/common/cfelsoft-rh7/ccp4/ccp4-7.0/lib/
HDF5_LIBRARY_PATH=/usr/lib64
PNG_LIBRARY_PATH=/usr/lib64

CPPXFEL_HEADER_PATHS=-I $(BOOST_HEADER_PATH) -I $(CCP4_HEADER_PATH) -I $(HDF5_HEADER_PATH) -I $(PNG_HEADER_PATH) 

CPPXFEL_LIBRARY_PATHS=-L $(BOOST_LIBRARY_PATH) -L $(CCP4_LIBRARY_PATH) -L $(HDF5_LIBRARY_PATH) -L $(PNG_HEADER_PATH)

FLAGS = -g -O3 -std=c++0x

LIBFLAGS = -lhdf5 -lhdf5_hl -lpng

BEFORE = $(CPPXFEL_HEADER_PATHS) $(FLAGS)
AFTER = $(CPPXFEL_LIBRARY_PATHS) $(LIBFLAGS)

all:
	@echo cppxfel header paths = $(CPPXFEL_HEADER_PATHS)
	@echo cppxfel library paths = $(CPPXFEL_LIBRARY_PATHS)
	@echo flags = $(FLAGS)
	@echo library flags = $(LIBFLAGS)

	g++ $(BEFORE) -c PNGFile.cpp
	g++ $(BEFORE) -c agipd_reader.cpp
	g++ $(BEFORE) -c agipd_module_reader.cpp	
	g++ $(BEFORE) -c TextManager.cpp
	g++ $(BEFORE) -c main.cpp	

	g++ -o ../agipd-reader *.o $(AFTER)	
