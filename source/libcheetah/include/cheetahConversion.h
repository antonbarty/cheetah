/*
 * cheetahConversion.h
 *
 *  Created on: 22.12.2015
 *      Author: Yaro
 */

#ifndef INCLUDE_CHEETAHCONVERSION_H_
#define INCLUDE_CHEETAHCONVERSION_H_

#include "cheetah/detectorGeometry.h"
#include <eigen3/Eigen/Dense>

void cheetahGetDetectorGeometryMatrix(const float* pix_x, const float* pix_y, const detectorRawSize_cheetah_t detectorRawSize_cheetah,
        Eigen::Vector2f** detectorGeometryMatrix);
void cheetahDeleteDetectorGeometryMatrix(Eigen::Vector2f* detectorGeometryMatrix);

#endif /* INCLUDE_CHEETAHCONVERSION_H_ */
