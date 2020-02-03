/* --------------------------------------------------------------------
Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with baltrad-wrwp.  If not, see <http://www.gnu.org/licenses/>.
------------------------------------------------------------------------*/

/** Header file for deriving weather radar wind and reflectivity profiles
 * @file
 * @author Gunther Haase, SMHI
 * @date 2013-02-06
 *
 * @author Ulf E. Nordh, SMHI
 * @date 2017-02-23, started overhaul of the code to achieve better
 * resemblance with N2 and requirements from customer E-profile
 */
#ifndef WRWP_H
#define WRWP_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <lapacke.h>
#include <cblas.h>

#include "rave_io.h"
#include "rave_attribute.h"
#include "cartesian.h"
#include "polarvolume.h"
#include "vertical_profile.h"
#include "projectionregistry.h"
#include "arearegistry.h"
#include "projection.h"
#include "area.h"
#include "rave_object.h"
#include "rave_types.h"
#include "rave_alloc.h"
#include "polarnav.h"
#include "raveutil.h"


/******************************************************************************/
/*Definition of standard parameters.                                          */
/******************************************************************************/

#define DEG2RAD     DEG_TO_RAD      /* Degrees to radians. From PROJ.4 */
#define RAD2DEG     RAD_TO_DEG      /* Radians to degrees. From PROJ.4 */
#define NOR         40000           /* Number of rows in matrix A used in the computation */
#define NOC         3               /* Number of columns in matrix A used in the computation */
#define NRHS        1               /* Number of right-hand sides; that is, the number of columns in matrix B used in the computation */
#define LDA         NOC             /* Leading dimension of the array specified for a */
#define LDB         NRHS            /* Leading dimension of the array specified for b */

/**
 * Defines a weather radar wind product generator
 */
typedef struct _Wrwp_t Wrwp_t;

/**
 * Type definition to use when creating a rave object.
 */
extern RaveCoreObjectType Wrwp_TYPE;

/**
 * Returns the height interval for deriving a profile [m]
 * @param[in] self - self
 * @return the height interval (default value is DZ)
 */
int Wrwp_getDZ(Wrwp_t* self);

/**
 * Sets the height interval for deriving a profile [m]
 * @param[in] self - self
 * @param[in] dz - the height interval
 */
void Wrwp_setDZ(Wrwp_t* self, int dz);

/**
 * Returns the nodata value used for vertical profiles
 * @param[in] self - self
 * @return the nodata value
 */
int Wrwp_getNODATA_VP(Wrwp_t* self);

/**
 * Sets the nodata value for the profile
 * @param[in] self - self
 * @param[in] nodata_VP - the nodata value
 */
void Wrwp_setNODATA_VP(Wrwp_t* self, int nodata_vp);

/**
 * Returns the undetect value used for vertical profiles
 * @param[in] self - self
 * @return the undetect value
 */
int Wrwp_getUNDETECT_VP(Wrwp_t* self);

/**
 * Sets the undetect value for the profile
 * @param[in] self - self
 * @param[in] undetect_VP - the nodata value
 */
void Wrwp_setUNDETECT_VP(Wrwp_t* self, int nodata_vp);

/**
 * Returns the gain value used for vertical profiles
 * @param[in] self - self
 * @return the gain value
 */
double Wrwp_getGAIN_VP(Wrwp_t* self);

/**
 * Sets the gain value for the vertical profile
 * @param[in] self - self
 * @param[in] gain_VP - the gain value
 */
void Wrwp_setGAIN_VP(Wrwp_t* self, double gain_vp);

/**
 * Returns the offset value used for vertical profiles
 * @param[in] self - self
 * @return the offset value
 */
double Wrwp_getOFFSET_VP(Wrwp_t* self);

/**
 * Sets the offset value for the profile
 * @param[in] self - self
 * @param[in] offset_VP - the offset value
 */
void Wrwp_setOFFSET_VP(Wrwp_t* self, double offset_vp);

/**
 * Sets maximum allowed velocity for each layer in the profile [m/s]
 * @param[in] self - self
 * @param[in] ff_max - maximum allowed velocity for each layer in the profile
 */
void Wrwp_setFF_MAX(Wrwp_t* self, double ff_max);

/**
 * Returns maximum allowed velocity for each layer in the profile [m/s]
 * @param[in] self - self
 * @return maximum allowed velocity for each layer in the profile [m/s] (default is FF_MAX)
 */
double Wrwp_getFF_MAX(Wrwp_t* self);

/**
 * Sets maximum height of the profile [m]
 * @param[in] self - self
 * @param[in] hmax - maximum height of the profile [m]
 */
void Wrwp_setHMAX(Wrwp_t* self, int hmax);

/**
 * Returns maximum height of the profile [m]
 * @param[in] self - self
 * @return maximum height of the profile (default is HMAX)
 */
int Wrwp_getHMAX(Wrwp_t* self);

/**
 * Sets minimum number of points for wind
 * @param[in] self - self
 * @param[in] nmin_wnd - minimum number of points for wind
 */
void Wrwp_setNMIN_WND(Wrwp_t* self, int nmin_wnd);

/**
 * Returns minimum number of points for wind
 * @param[in] self - self
 * @return minimum number of points for wind (default NMIN_WND)
 */
int Wrwp_getNMIN_WND(Wrwp_t* self);

/**
 * Sets minimum number of points for reflectivity
 * @param[in] self - self
 * @param[in] nmin_ref - minimum number of points for reflectivity
 */
void Wrwp_setNMIN_REF(Wrwp_t* self, int nmin_ref);

/**
 * Returns minimum number of points for reflectivity
 * @param[in] self - self
 * @return minimum number of points for reflectivity (default NMIN_REF)
 */
int Wrwp_getNMIN_REF(Wrwp_t* self);

/**
 * Sets minimum distance for deriving a profile [m]
 * @param[in] self - self
 * @param[in] dmin - minimum distance for deriving a profile [m]
 */
void Wrwp_setDMIN(Wrwp_t* self, int dmin);

/**
 * Returns minimum distance for deriving a profile [m]
 * @param[in] self - self
 * @return minimum distance for deriving a profile [m] (default DMIN)
 */
int Wrwp_getDMIN(Wrwp_t* self);

/**
 * Sets maximum distance for deriving a profile [m]
 * @param[in] self - self
 * @param[in] dmax - maximum distance for deriving a profile [m]
 */
void Wrwp_setDMAX(Wrwp_t* self, int dmax);

/**
 * Returns maximum distance for deriving a profile [m]
 * @param[in] self - self
 * @return maximum distance for deriving a profile [m] (default DMAX)
 */
int Wrwp_getDMAX(Wrwp_t* self);

/**
 * Sets minimum elevation angle [deg]
 * @param[in] self - self
 * @param[in] emin - minimum elevation angle [deg]
 */
void Wrwp_setEMIN(Wrwp_t* self, double emin);

/**
 * Returns minimum elevation angle [deg]
 * @param[in] self - self
 * @return Minimum elevation angle [deg] (default EMIN)
 */
double Wrwp_getEMIN(Wrwp_t* self);

/**
 * Sets maximum elevation angle [deg]
 * @param[in] self - self
 * @param[in] emax - maximum elevation angle [deg]
 */
void Wrwp_setEMAX(Wrwp_t* self, double emax);

/**
 * Returns maximum elevation angle [deg]
 * @param[in] self - self
 * @return Maximum elevation angle [deg] (default EMAX)
 */
double Wrwp_getEMAX(Wrwp_t* self);

/**
 * Sets radial velocity threshold [m/s]
 * @param[in] self - self
 * @param[in] vmin - radial velocity threshold [m/s]
 */
void Wrwp_setVMIN(Wrwp_t* self, double vmin);

/**
 * Returns radial velocity threshold [m/s]
 * @param[in] self - self
 * @return Radial velocity threshold [m/s] (default VMIN)
 */
double Wrwp_getVMIN(Wrwp_t* self);

/**
 * Function for deriving wind and reflectivity profiles from polar volume data
 * @param[in] self - self
 * @param[in] iobj - input volume
 * @param[in] fieldsToGenerate - an comma-separated list of quantities. If NULL, then default
 * behaviour is to add ff,ff_dev,dd,dbzh and dbzh_dev
 * @returns the wind profile
 */
VerticalProfile_t* Wrwp_generate(Wrwp_t* self, PolarVolume_t* inobj, const char* fieldsToGenerate);

#endif
