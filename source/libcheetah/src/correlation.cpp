/*
 *  correlation.cpp
 *  cheetah
 *
 *  Created by Haiguang Liu on 04/22/13.
 *  Copyright 2011 CFEL. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <hdf5.h>
#include <stdlib.h>
// #include <fftw3.h>

#include "detectorObject.h"
#include "cheetahGlobal.h"
#include "cheetahEvent.h"
#include "cheetahmodules.h"
#include "median.h"



/*
 *  Calculate radial averages
 *  (repeated once for each different data type)
 */

void calculateAutoCorrelation(cEventData *eventData, cGlobal *global) {

   cPixelDetectorCommon     *detector;
 	DETECTOR_LOOP {
        detector = &global->detector[detID];    
        float   *corrected_data = eventData->detector[detID].corrected_data;
        long    *polar_map = global->detector[detID].polar_map;
        long	 pix_nn = global->detector[detID].pix_nn;
        long	 polar_nn = global->detector[detID].polar_nn;

        if( global->debugLevel > 2) {
          float *pix_x = detector->pix_x;
          float *pix_y = detector->pix_y;
          for(int ii=0;ii<polar_nn;ii++) {
            if( polar_map[ii] > 0 )
            printf("%f %f map2\n", pix_x[polar_map[ii]],pix_y[polar_map[ii]]);
          }
          exit(1);
        }
        // Mask for where to calculate average
        int     *mask = (int *) calloc(pix_nn, sizeof(int));
        for(long i=0; i<pix_nn; i++){
	  mask[i] = isNoneOfBitOptionsSet(eventData->detector[detID].pixelmask[i],(PIXEL_IS_TO_BE_IGNORED | PIXEL_IS_BAD));
        }
        calculateAutoCorrelation(corrected_data, polar_map, pix_nn, polar_nn, mask, detector);
        
        // Remember to free the mask
        free(mask); 
    }
   
}
    

void calculateAutoCorrelation(float *data, long *polar_map, long pix_nn, long polar_nn, int *mask, cPixelDetectorCommon* detector){
    
	// allocate memory for polar intensities
   double *intensities_in_polar = (double*) calloc( polar_nn, sizeof(double) );
   double *this_autocorrelation = (double*) calloc( polar_nn, sizeof(double) );
   if( intensities_in_polar == NULL )
   {
     STATUS( "memory allocation failed in autocorrelation calculation\n" );
     return ;
   }
   // map the values to polar pixels
   for(long ii=0;ii<polar_nn;ii++){
     if( polar_map[ii] >0 ) {
	    if( mask[ polar_map[ii] ] != 0 )
         intensities_in_polar[ii] = data[ polar_map[ii] ];
     }
   }
/*
   float *pix_x = detector->pix_x;
   float *pix_y = detector->pix_y;
   for(int ii=0;ii<polar_nn;ii++) {
   if( polar_map[ii] > 0 )
   printf("%f %f %f map2\n", pix_x[polar_map[ii]],pix_y[polar_map[ii]],data[polar_map[ii] ]);
   }
   exit(1);
*/
   // compute autocorrelation using FFTW library, add to detector member array
   // to be done

   pthread_mutex_lock(&detector->autocorrelation_mutex);
   for(long ii=0;ii<polar_nn;ii++){
     detector->polarIntensities[ii] += intensities_in_polar[ii]; 
     detector->autocorrelation[ii] += this_autocorrelation[ii]; 
   }
   //writeSimpleHDF5("test-polar.h5", detector->polarIntensities, detector->nAngularBins, detector->nRadialBins, H5T_NATIVE_FLOAT);
   //writeSimpleHDF5("test-data.h5", data, detector->pix_nx, detector->pix_ny, H5T_NATIVE_FLOAT);
   //exit(1);
   detector->autocorrelationCounter++;
   pthread_mutex_unlock(&detector->autocorrelation_mutex);
   free(intensities_in_polar);	
   free(this_autocorrelation);	
}



/*
 *	Wrapper for saving auto-correlations
 */
/*
void saveAutoCorrelation(cGlobal *global) {
    if(!global->saveAutoCorrelation)
        return;
    
    printf("Saving auto-correlation\n");
    int powderType=0, detID=0;
    saveAutoCorrelation(global, powderType, detID);

    DETECTOR_LOOP {
        for(long powderType=0; powderType < global->nPowderClasses; powderType++) {
        }
    }
}
*/



/*
 *  Save auto-correlation
 */
// void saveAutoCorrelation(cGlobal *global, int powderClass, int detID) {  // saved
void saveAutoCorrelation(cGlobal *global) {

    int powderClass=0, detID=0;

    cPixelDetectorCommon     *detector = &global->detector[detID];

    pthread_mutex_lock(&detector->autocorrelation_mutex);

    char	filename[1024];
    long    frameNum = detector->autocorrelationCounter;

    sprintf(filename,"r%04u-autocorrelation-detector%d-class%i-frame%ld.h5", global->runNumber, detID, powderClass,frameNum);
    printf("Saving auto-correlation: %s\n", filename);
    writeSimpleHDF5(filename, detector->autocorrelation, detector->nAngularBins, detector->nRadialBins, H5T_NATIVE_FLOAT);

   sprintf(filename,"r%04u-polar-detector%d-class%i-frame%ld.h5", global->runNumber, detID, powderClass,frameNum);
   printf("Saving Polar pixel intensity: %s\n", filename);
   writeSimpleHDF5(filename, detector->polarIntensities, detector->nAngularBins, detector->nRadialBins, H5T_NATIVE_FLOAT);
   // compute auto-correlation using fft
    pthread_mutex_unlock(&detector->autocorrelation_mutex);			

}


