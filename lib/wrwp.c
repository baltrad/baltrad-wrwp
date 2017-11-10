/* --------------------------------------------------------------------
Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI

This is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with HLHDF.  If not, see <http://www.gnu.org/licenses/>.
------------------------------------------------------------------------*/

/** Function for deriving weather radar wind and reflectivity profiles
 *
 * @file
 * @author Gunther Haase, SMHI
 * @date 2013-02-06
 *
 * @co-author Ulf E. Nordh, SMHI
 * @date 2017-02-23, started overhaul of the code to achieve better
 * resemblance with N2 and requirements from customer E-profile
 * 
 */

#include "wrwp.h"
#include "vertical_profile.h"
#include "rave_debug.h"
#include "rave_alloc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rave_attribute.h"
/**
 * Represents one wrwp generator
 */
struct _Wrwp_t {
  RAVE_OBJECT_HEAD /** Always on top */
  int dz; /**< Height interval for deriving a profile [m] */
  int hmax; /**< Maximum height of the profile [m] */
  int dmin; /**< Minimum distance for deriving a profile [m] */
  int dmax; /**< Maximum distance for deriving a profile [m]*/
  double emin; /**< Minimum elevation angle [deg] */
  double vmin; /**< Radial velocity threshold [m/s] */
  double nodata_VP; /**< Nodata value for vertical profile */
  double gain_VP; /**< Gain for VP fields */
  double offset_VP; /**< Offset for VP fields */
  double undetect_VP; /**<Undetect for VP fields */
};

/*@{ Private functions */
/**
 * Constructor
 */
static int Wrwp_constructor(RaveCoreObject* obj)
{
  Wrwp_t* wrwp = (Wrwp_t*)obj;
  wrwp->hmax = HMAX;
  wrwp->dz = DZ;
  wrwp->dmin = DMIN;
  wrwp->dmax = DMAX;
  wrwp->emin = EMIN;
  wrwp->vmin = VMIN;
  wrwp->nodata_VP = NODATA_VP;
  wrwp->gain_VP = GAIN_VP;
  wrwp->offset_VP = OFFSET_VP;
  wrwp->undetect_VP = UNDETECT_VP;
  return 1;
}

/**
 * Destructor
 */
static void Wrwp_destructor(RaveCoreObject* obj)
{
}

static int WrwpInternal_findAndAddAttribute(VerticalProfile_t* vp, PolarVolume_t* pvol, const char* name, double minSelAng)
{
  int nscans = PolarVolume_getNumberOfScans(pvol);
  int i = 0;
  int found = 0;
  double elangle = 0.0;
  
  for (i = 0; i < nscans && found == 0; i++) {
    PolarScan_t* scan = PolarVolume_getScan(pvol, i);
    if (scan != NULL && PolarScan_hasAttribute(scan, name)) {
        elangle = PolarScan_getElangle(scan);
        
        /* Filter with respect to the selected min elangle
           that is given in the web GUI route for WRWP generation */
        if ((elangle * RAD2DEG) >= minSelAng) {
          RaveAttribute_t* attr = PolarScan_getAttribute(scan, name);
          VerticalProfile_addAttribute(vp, attr);
          found = 1;
          RAVE_OBJECT_RELEASE(attr);
        }
    }
    RAVE_OBJECT_RELEASE(scan);
  }
  return found;
}

static int WrwpInternal_addIntAttribute(VerticalProfile_t* vp, const char* name, int value)
{
  RaveAttribute_t* attr = RaveAttributeHelp_createLong(name, value);
  int result = 0;
  if (attr != NULL) {
    result = VerticalProfile_addAttribute(vp, attr);
  }
  RAVE_OBJECT_RELEASE(attr);
  return result;
}

/* Function that adds various quantities under a field's what in order to
   better resemble the function existing in vertical profiles from N2 */
static int WrwpInternal_addDoubleAttr2Field(RaveField_t* field, const char* name, double quantity)
{
  int result = 0;
  RaveAttribute_t* attr = RaveAttributeHelp_createDouble(name, quantity);
  result = RaveField_addAttribute(field, attr);
  if (attr == NULL || !result) {
    RAVE_ERROR0("Failed to add what/quantity attribute to field");
    goto done;
  }
  RAVE_OBJECT_RELEASE(attr);
done:
  return result;
}

/*@} End of Private functions */

/*@{ Interface functions */
void Wrwp_setDZ(Wrwp_t* self, int dz)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->dz = dz;
}

int Wrwp_getDZ(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->dz;
}

void Wrwp_setNODATA_VP(Wrwp_t* self, int nodata_VP)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->nodata_VP = nodata_VP;
}

int Wrwp_getNODATA_VP(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->nodata_VP;
}

void Wrwp_setUNDETECT_VP(Wrwp_t* self, int undetect_VP)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->undetect_VP = undetect_VP;
}

int Wrwp_getUNDETECT_VP(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->undetect_VP;
}

void Wrwp_setGAIN_VP(Wrwp_t* self, int gain_VP)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->gain_VP = gain_VP;
}

int Wrwp_getGAIN_VP(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->gain_VP;
}

void Wrwp_setOFFSET_VP(Wrwp_t* self, int offset_VP)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->offset_VP = offset_VP;
}

int Wrwp_getOFFSET_VP(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->offset_VP;
}

void Wrwp_setHMAX(Wrwp_t* self, int hmax)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->hmax = hmax;
}

int Wrwp_getHMAX(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->hmax;
}

void Wrwp_setDMIN(Wrwp_t* self, int dmin)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->dmin = dmin;
}

int Wrwp_getDMIN(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->dmin;
}

void Wrwp_setDMAX(Wrwp_t* self, int dmax)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->dmax = dmax;
}

int Wrwp_getDMAX(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->dmax;
}

void Wrwp_setEMIN(Wrwp_t* self, double emin)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->emin = emin;
}

double Wrwp_getEMIN(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->emin;
}

void Wrwp_setVMIN(Wrwp_t* self, double vmin)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->vmin = vmin;
}

double Wrwp_getVMIN(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->vmin;
}

VerticalProfile_t* Wrwp_generate(Wrwp_t* self, PolarVolume_t* inobj) {
  VerticalProfile_t* result = NULL;
  PolarScan_t* scan = NULL;
  PolarScanParam_t* vrad = NULL;
  PolarScanParam_t* dbz = NULL;
  PolarNavigator_t* polnav = NULL;
  int nrhs = NRHS, lda = LDA, ldb = LDB;
  int nscans = 0, nbins = 0, nrays, nv, nz, i, iz, is, ib, ir;
  int findEarliestTime;
  int findLatestTime;
  int firstInit;

  double rscale, elangle, gain, offset, nodata, undetect, val;
  double d, h, elangleForThisScan;
  double alpha, beta, gamma, vvel, vdir, vstd, zsum, zmean, zstd;
  double centerOfLayer, u_wnd_comp, v_wnd_comp, vdir_rad;
  int ysize = 0, yindex = 0;
  
  const char* starttime = NULL;
  const char* endtime = NULL;
  const char* startdate = NULL;
  const char* enddate = NULL;
  const char* product = "VP";
  
  const char* startTimeOfThisScan = NULL;
  const char* endTimeOfThisScan = NULL;
  const char* startDateOfThisScan = NULL;
  const char* endDateOfThisScan = NULL;
    
  const char* starttimeFirst = NULL;
  const char* endtimeFirst = NULL;
  const char* startdateFirst = NULL;
  const char* enddateFirst = NULL;
      
  const char* starttimeNext = NULL;
  const char* endtimeNext = NULL;
  const char* startdateNext = NULL;
  const char* enddateNext = NULL;
  
  char* startDateTimeStrFirst = NULL;
  char* endDateTimeStrFirst = NULL;
  
  char* startDateTimeStrNext = NULL;
  char* endDateTimeStrNext = NULL;
   
  char starttimeArrFirst[10];
  char endtimeArrFirst[10]; 
  char startdateArrFirst[10];
  char enddateArrFirst[10];  
 
  char starttimeArrNext[10];
  char endtimeArrNext[10];
  char startdateArrNext[10];
  char enddateArrNext[10];

  char* malfuncString = NULL;
   
  /* The four field below are the ones fulfilling the requirements from the user E-profiles */
  RaveField_t *nv_field = NULL, *HGHT_field = NULL;
  RaveField_t *UWND_field = NULL, *VWND_field = NULL;
  
  /* The following fields are removed from the VP but kept in the code
     for convenience reasons, add to initialization when needed:     
     *RaveField_t *ff_field = NULL, *ff_dev_field = NULL, *dd_field = NULL;
      RaveField_t *dbzh_field = NULL, *dbzh_dev_field = NULL, *nz_field = NULL; */

  RAVE_ASSERT((self != NULL), "self == NULL");

  nv_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  HGHT_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  UWND_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  VWND_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
    
  /* Field defs kept in the code for convenience reasons, add when needed:
  ff_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  ff_dev_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  dd_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  dbzh_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  dbzh_dev_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  nz_field = RAVE_OBJECT_NEW(&RaveField_TYPE); */

  if (nv_field == NULL || HGHT_field == NULL || UWND_field == NULL ||
      VWND_field == NULL) {
    /* Removed, add when needed:ff_field == NULL || ff_dev_field == NULL || 
       dd_field == NULL || dbzh_field == NULL || dbzh_dev_field == NULL || 
       nz_field == NULL || */
    RAVE_ERROR0("Failed to allocate memory for the resulting vp fields");
    goto done;
  }
  ysize = self->hmax / self->dz;
  if (!RaveField_createData(nv_field, 1, ysize, RaveDataType_INT) || 
      !RaveField_createData(HGHT_field, 1, ysize, RaveDataType_DOUBLE) ||
      !RaveField_createData(UWND_field, 1, ysize, RaveDataType_DOUBLE) ||
      !RaveField_createData(VWND_field, 1, ysize, RaveDataType_DOUBLE)) {
      /* Add when needed: !RaveField_createData(ff_field, 1, ysize, RaveDataType_DOUBLE) ||
      !RaveField_createData(ff_dev_field, 1, ysize, RaveDataType_DOUBLE) ||
      !RaveField_createData(dd_field, 1, ysize, RaveDataType_DOUBLE) ||
      !RaveField_createData(dbzh_field, 1, ysize, RaveDataType_DOUBLE) ||
      !RaveField_createData(dbzh_dev_field, 1, ysize, RaveDataType_DOUBLE) ||
      !RaveField_createData(nz_field, 1, ysize, RaveDataType_INT) || */
    RAVE_ERROR0("Failed to allocate arrays for the resulting vp fields");
    goto done;
  }

	polnav = RAVE_OBJECT_NEW(&PolarNavigator_TYPE);
	PolarNavigator_setLat0(polnav, PolarVolume_getLatitude(inobj));
	PolarNavigator_setLon0(polnav, PolarVolume_getLongitude(inobj));
	PolarNavigator_setAlt0(polnav, PolarVolume_getHeight(inobj));
	    
	nscans = PolarVolume_getNumberOfScans (inobj);
    

	// We use yindex for filling in the arrays even though we loop to hmax...
	yindex = 0;


	// loop over atmospheric layers
  for (iz = 0; iz < self->hmax; iz += self->dz) {
    /* allocate memory and initialize with zeros */
    double *A = RAVE_CALLOC((size_t)(NOR*NOC), sizeof (double));
    double *b = RAVE_CALLOC((size_t)(NOR), sizeof (double));
    double *v = RAVE_CALLOC((size_t)(NOR), sizeof (double));
    double *z = RAVE_CALLOC((size_t)(NOR), sizeof (double));
    double *az = RAVE_CALLOC((size_t)(NOR), sizeof (double));

    vdir = -9999.0;
    vvel = -9999.0;
    vstd = 0.0;
    zsum = 0.0;
    zmean = -9999.0;
    zstd = 0.0;
    nv = 0;
    nz = 0;
    
    /* Define the center height of each verical layer, this will later
       become the HGHT array */
    centerOfLayer = iz + (self->dz / 2.0);
    firstInit = 0;
    
    // loop over scans
    for (is = 0; is < nscans; is++) {      
      scan = PolarVolume_getScan(inobj, is);
      nbins = PolarScan_getNbins(scan);
      nrays = PolarScan_getNrays(scan);
      rscale = PolarScan_getRscale(scan);
      elangleForThisScan = PolarScan_getElangle(scan);
      elangle = elangleForThisScan;
      startTimeOfThisScan = PolarScan_getStartTime(scan);
      endTimeOfThisScan = PolarScan_getEndTime(scan);
      startDateOfThisScan = PolarScan_getStartDate(scan);
      endDateOfThisScan = PolarScan_getEndDate(scan);
      
      RaveAttribute_t* malfuncattr = PolarScan_getAttribute(scan, "how/malfunc");
      if (malfuncattr != NULL) {
        RaveAttribute_getString(malfuncattr, &malfuncString); /* Set the malfuncString if attr is not NULL */
        RAVE_OBJECT_RELEASE(malfuncattr);
      }
      if (malfuncString == NULL || strcmp(malfuncString, "False") == 0) { /* Assuming malfuncString = NULL means no malfunc */
                  
        if (elangleForThisScan * RAD2DEG >= self->emin && firstInit == 0) {
          /* Initialize using the first scan and define 2 strings for the combined datetime */
          starttimeFirst = startTimeOfThisScan;
          endtimeFirst = endTimeOfThisScan;
          startdateFirst = startDateOfThisScan;
          enddateFirst = endDateOfThisScan;
          firstInit = 1;
        }
        
        if (elangleForThisScan * RAD2DEG >= self->emin && firstInit == 1 && is > 0) {
          starttimeNext = startTimeOfThisScan;
          endtimeNext = endTimeOfThisScan;
          startdateNext = startDateOfThisScan;
          enddateNext = endDateOfThisScan;

          strcpy(starttimeArrFirst, starttimeFirst);
          strcpy(endtimeArrFirst, endtimeFirst);
          strcpy(startdateArrFirst, startdateFirst);
          strcpy(enddateArrFirst, enddateFirst);

          startDateTimeStrFirst = strcat(startdateArrFirst, starttimeArrFirst);
          endDateTimeStrFirst = strcat(enddateArrFirst, endtimeArrFirst);

          strcpy(starttimeArrNext, starttimeNext);
          strcpy(endtimeArrNext, endtimeNext);
          strcpy(startdateArrNext, startdateNext);
          strcpy(enddateArrNext, enddateNext);

          startDateTimeStrNext = strcat(startdateArrNext, starttimeArrNext);
          endDateTimeStrNext = strcat(enddateArrNext, endtimeArrNext);

          /* Find the earliest and latest datetime's and replace the initial ones
             if they represent an earlier starttime/startdate and a later endtime/enddate */
          findEarliestTime = strcmp(startDateTimeStrFirst, startDateTimeStrNext);
          findLatestTime = strcmp(endDateTimeStrFirst, endDateTimeStrNext);

          /* The starttime and startdate */
          if (findEarliestTime > 0) {
            starttimeFirst = starttimeNext;
            startdateFirst = startdateNext;
          }

          /* The endtime and enddate */
          if (findLatestTime < 0) {
            endtimeFirst = endtimeNext;
            enddateFirst = enddateNext;
          }             
        }
                     
        // radial wind scans
        if (PolarScan_hasParameter(scan, "VRAD") || PolarScan_hasParameter(scan, "VRADH")) {
          if (PolarScan_hasParameter(scan, "VRAD")) {
            vrad = PolarScan_getParameter(scan, "VRAD");
          } else {
            vrad = PolarScan_getParameter(scan, "VRADH");
          } 
          gain = PolarScanParam_getGain(vrad);
          offset = PolarScanParam_getOffset(vrad);
          nodata = PolarScanParam_getNodata(vrad);
          undetect = PolarScanParam_getUndetect(vrad);

          for (ir = 0; ir < nrays; ir++) {
            for (ib = 0; ib < nbins; ib++) {
              PolarNavigator_reToDh(polnav, (ib+0.5)*rscale, elangle, &d, &h);
              PolarScanParam_getValue(vrad, ib, ir, &val);

              if ((h >= iz) &&
                  (h < iz + self->dz) &&
                  (d >= self->dmin) &&
                  (d <= self->dmax) &&
                  (elangle * RAD2DEG >= self->emin) &&
                  (val != nodata) &&
                  (val != undetect) &&
                  (abs(offset + gain * val) >= self->vmin)) {
                *(v+nv) = offset+gain*val;
                *(az+nv) = 360./nrays*ir*DEG2RAD;
                *(A+nv*NOC) = sin(*(az+nv));
                *(A+nv*NOC+1) = cos(*(az+nv));
                *(A+nv*NOC+2) = 1;
                *(b+nv) = *(v+nv);
                nv = nv+1;
              }
            }
          }
          RAVE_OBJECT_RELEASE(vrad);
        }

        // reflectivity scans
        if (PolarScan_hasParameter(scan, "DBZH")) {
          dbz = PolarScan_getParameter(scan, "DBZH");
          gain = PolarScanParam_getGain(dbz);
          offset = PolarScanParam_getOffset(dbz);
          nodata = PolarScanParam_getNodata(dbz);
          undetect = PolarScanParam_getUndetect(dbz);

          for (ir = 0; ir < nrays; ir++) {
            for (ib = 0; ib < nbins; ib++) {
              PolarNavigator_reToDh(polnav, (ib+0.5)*rscale, elangle, &d, &h);
              PolarScanParam_getValue (dbz, ib, ir, &val);
              if ((h >= iz) &&
                  (h < iz+self->dz) &&
                  (d >= self->dmin) &&
                  (d <= self->dmax) &&
                  (elangle*RAD2DEG >= self->emin) &&
                  (val != nodata) &&
                  (val != undetect)) {
                *(z+nz) = dBZ2Z(offset+gain*val);
                zsum = zsum + *(z+nz);
                nz = nz+1;
              }
            }
          }
          RAVE_OBJECT_RELEASE(dbz);
        }
        RAVE_OBJECT_RELEASE(scan);
      }
    }

    // radial wind calculations
    if (nv>3) {
      //***************************************************************
      // fitting: y = gamma+alpha*sin(x+beta)                         *
      // alpha -> amplitude                                           *
      // beta -> phase shift                                          *
      // gamma -> consider an y-shift due to the terminal velocity of *
      //          falling rain drops                                  *
      //***************************************************************

      /* QR decomposition */
      /*info = */LAPACKE_dgels(LAPACK_ROW_MAJOR, 'N', NOR, NOC, nrhs, A, lda, b, ldb);

      /* parameter of the wind model */
      alpha = sqrt(pow(*(b),2) + pow(*(b+1),2));
      beta = atan2(*(b+1), *b);
      gamma = *(b+2);

      /* wind velocity */
      vvel = alpha;

      /* wind direction */
      vdir = 0;
      if (alpha < 0) {
        vdir = (PI/2-beta) * RAD2DEG;
      } else if (alpha > 0) {
        vdir = (3*PI/2-beta) * RAD2DEG;
      }
      if (vdir < 0) {
        vdir = vdir + 360;
      } else if (vdir > 360) {
        vdir = vdir - 360;
      }

      /* standard deviation of the wind velocity */
      for (i=0; i<nv; i++) {
        vstd = vstd + pow (*(v+i) - (gamma+alpha*sin (*(az+i)+beta)),2);
      }
      vstd = sqrt (vstd/(nv-1));
    }

    // reflectivity calculations
    if (nz > 1) {
      /* standard deviation of reflectivity */
      for (i = 0; i < nz; i++) {
        zstd = zstd + pow(*(z+i) - (zsum/nz),2);
      }
      zmean = Z2dBZ(zsum/nz);
      zstd = sqrt(zstd/(nz-1));
      zstd = Z2dBZ(zstd);
    }

    /* Calculate the x-component (East) and y-component (North) of the wind 
       velocity using the wind direction and the magnitude of the wind velocity */
    vdir_rad = vdir * DEG2RAD;
    u_wnd_comp = vvel * cos(vdir_rad);
    v_wnd_comp = vvel * sin(vdir_rad);
    
    if ((nv < NMIN) || (nz < NMIN)) {
      /* Add when needed: RaveField_setValue(ff_field, 0, yindex, self->nodata_VP);
      RaveField_setValue(ff_dev_field, 0, yindex, self->nodata_VP);
      RaveField_setValue(dd_field, 0, yindex, self->nodata_VP);
      RaveField_setValue(dbzh_field, 0, yindex, self->nodata_VP);
      RaveField_setValue(dbzh_dev_field, 0, yindex, self->nodata_VP);
      RaveField_setValue(nz_field, 0, yindex, 0); */
      RaveField_setValue(nv_field, 0, yindex, 0);
      RaveField_setValue(HGHT_field, 0, yindex,centerOfLayer);
      RaveField_setValue(UWND_field, 0, yindex,self->nodata_VP);
      RaveField_setValue(VWND_field, 0, yindex,self->nodata_VP);
    } else {
      /* Add wgen needed: RaveField_setValue(ff_field, 0, yindex, vvel);
      RaveField_setValue(ff_dev_field, 0, yindex, vstd);
      RaveField_setValue(dd_field, 0, yindex, vdir);
      RaveField_setValue(dbzh_field, 0, yindex, zmean);
      RaveField_setValue(dbzh_dev_field, 0, yindex, zstd);
      RaveField_setValue(nz_field, 0, yindex, nz); */
      RaveField_setValue(nv_field, 0, yindex, nv);
      RaveField_setValue(HGHT_field, 0, yindex,centerOfLayer);
      RaveField_setValue(UWND_field, 0, yindex,u_wnd_comp);
      RaveField_setValue(VWND_field, 0, yindex,v_wnd_comp);
    }

    RAVE_FREE(A);
    RAVE_FREE(b);
    RAVE_FREE(v);
    RAVE_FREE(z);
    RAVE_FREE(az);

    yindex++; /* Next interval*/
  }

  /* Set values used for /dataset1/what attributes */  
  starttime = starttimeFirst;
  endtime = endtimeFirst;
  startdate = startdateFirst;
  enddate = enddateFirst;

  
  result = RAVE_OBJECT_NEW(&VerticalProfile_TYPE);
  if (result != NULL) {
    if (!VerticalProfile_setUWND(result, UWND_field) ||
        !VerticalProfile_setVWND(result, VWND_field) ||
        !VerticalProfile_setNV(result, nv_field) ||
        !VerticalProfile_setHGHT(result, HGHT_field)) {
        /* Add when needed: !VerticalProfile_setFF(result, ff_field) ||
        !VerticalProfile_setFFDev(result, ff_dev_field) ||
        !VerticalProfile_setDD(result, dd_field) ||
        !VerticalProfile_setDBZ(result, dbzh_field) ||
        !VerticalProfile_setDBZDev(result, dbzh_dev_field)) */
      RAVE_ERROR0("Failed to set vertical profile fields");
      RAVE_OBJECT_RELEASE(result);
    }
  }

  VerticalProfile_setLongitude(result, PolarVolume_getLongitude(inobj));
  VerticalProfile_setLatitude(result, PolarVolume_getLatitude(inobj));
  VerticalProfile_setHeight(result, PolarVolume_getHeight(inobj));
  VerticalProfile_setSource(result, PolarVolume_getSource(inobj));
  VerticalProfile_setInterval(result, self->dz);
  VerticalProfile_setLevels(result, ysize);
  VerticalProfile_setMinheight(result, 0);
  VerticalProfile_setMaxheight(result, self->hmax);
  VerticalProfile_setDate(result, PolarVolume_getDate(inobj));
  VerticalProfile_setTime(result, PolarVolume_getTime(inobj));
  
  /* Set the times and product, starttime is the starttime for the lowest elev
     endtime is the endtime for the highest elev. */
  VerticalProfile_setStartTime(result, starttime);
  VerticalProfile_setEndTime(result, endtime);
  VerticalProfile_setStartDate(result, startdate);
  VerticalProfile_setEndDate(result, enddate);
  VerticalProfile_setProduct(result, product);
   
  /* Need to filter the attribute data with respect to selected min elangle
     Below attributes satisfy reguirements from E-profile, add other when needed */
  
  WrwpInternal_findAndAddAttribute(result, inobj, "how/highprf", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/lowprf", self->emin);
  /*WrwpInternal_findAndAddAttribute(result, inobj, "how/pulsewidth", self->emin); */
  WrwpInternal_findAndAddAttribute(result, inobj, "how/wavelength", self->emin);
 /* WrwpInternal_findAndAddAttribute(result, inobj, "how/RXbandwidth", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/RXlossH", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/TXlossH", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/antgainH", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/azmethod", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/binmethod", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/malfunc", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/nomTXpower", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/radar_msg", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/radconstH", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/radomeloss", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/rpm", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/software", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/system", self->emin);*/

  WrwpInternal_addIntAttribute(result, "how/minrange", Wrwp_getDMIN(self));
  WrwpInternal_addIntAttribute(result, "how/maxrange", Wrwp_getDMAX(self));
  
  
  /* Put in selected attributes in selected fields, currently a nodata, gain and
     offset are defined for UWND and VWND and are put under what/nodata. */
  WrwpInternal_addDoubleAttr2Field(UWND_field, "what/nodata", self->nodata_VP);
  WrwpInternal_addDoubleAttr2Field(UWND_field, "what/gain", self->gain_VP);
  WrwpInternal_addDoubleAttr2Field(UWND_field, "what/offset", self->offset_VP);
  WrwpInternal_addDoubleAttr2Field(UWND_field, "what/undetect", self->undetect_VP);
  
  VerticalProfile_addField(result, UWND_field);
  
  WrwpInternal_addDoubleAttr2Field(VWND_field, "what/nodata", self->nodata_VP);
  WrwpInternal_addDoubleAttr2Field(VWND_field, "what/gain", self->gain_VP);
  WrwpInternal_addDoubleAttr2Field(VWND_field, "what/offset", self->offset_VP);
  WrwpInternal_addDoubleAttr2Field(VWND_field, "what/undetect", self->undetect_VP);
  
  VerticalProfile_addField(result, VWND_field);
  
  WrwpInternal_addDoubleAttr2Field(HGHT_field, "what/nodata", self->nodata_VP);
  WrwpInternal_addDoubleAttr2Field(HGHT_field, "what/gain", self->gain_VP);
  WrwpInternal_addDoubleAttr2Field(HGHT_field, "what/offset", self->offset_VP);
  WrwpInternal_addDoubleAttr2Field(HGHT_field, "what/undetect", self->undetect_VP);
  
  VerticalProfile_addField(result, HGHT_field);
  
  WrwpInternal_addDoubleAttr2Field(nv_field, "what/nodata", self->nodata_VP);
  WrwpInternal_addDoubleAttr2Field(nv_field, "what/gain", self->gain_VP);
  WrwpInternal_addDoubleAttr2Field(nv_field, "what/offset", self->offset_VP);
  WrwpInternal_addDoubleAttr2Field(nv_field, "what/undetect", self->undetect_VP);
  
  VerticalProfile_addField(result, nv_field);

done:
  RAVE_OBJECT_RELEASE(polnav);
  /* 
  RAVE_OBJECT_RELEASE(ff_field);
  RAVE_OBJECT_RELEASE(ff_dev_field);
  RAVE_OBJECT_RELEASE(dd_field);
  RAVE_OBJECT_RELEASE(dbzh_field);
  RAVE_OBJECT_RELEASE(dbzh_dev_field);
  RAVE_OBJECT_RELEASE(nz_field); */
  RAVE_OBJECT_RELEASE(nv_field);
  RAVE_OBJECT_RELEASE(HGHT_field);
  RAVE_OBJECT_RELEASE(UWND_field);
  RAVE_OBJECT_RELEASE(VWND_field);

  return result;
}

/*@} End of Interface functions */

RaveCoreObjectType Wrwp_TYPE = {
    "Wrwp",
    sizeof(Wrwp_t),
    Wrwp_constructor,
    Wrwp_destructor
};

