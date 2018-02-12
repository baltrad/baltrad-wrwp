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
 * @date 2018-02-08, fixed a bug that resulted in a process hang if the
 * incoming polar volume file only contained scans with elevation angle
 * smaller than the one stated in the call to the pgf. These kind of files
 * are rather common when using the central software EDGE coming with the
 * modernized Swedish radars. If suck a file is encountered, the return is just
 * NULL and the exception following is treated in the pgf.
 */

#include "wrwp.h"
#include "vertical_profile.h"
#include "rave_debug.h"
#include "rave_alloc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rave_attribute.h"
#include "rave_utilities.h"
#include "rave_datetime.h"

/**
 * Represents one wrwp generator
 */
struct _Wrwp_t
{
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
  double elangleForThisScan = 0.0;
  
  for (i = 0; i < nscans && found == 0; i++) {
    PolarScan_t* scan = PolarVolume_getScan(pvol, i);
    if (scan != NULL && PolarScan_hasAttribute(scan, name)) {
        elangleForThisScan = PolarScan_getElangle(scan);
        
        /* Filter with respect to the selected min elangle
           that is given in the web GUI route for WRWP generation */
        if ((elangleForThisScan * RAD2DEG) >= minSelAng) {
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
    RAVE_ERROR1("Failed to add %s attribute to field", name);
    goto done;
  }
  RAVE_OBJECT_RELEASE(attr);
done:
  return result;
}

/**
 * Returns a tokenized list (or empty) of fields that is wanted
 * @param[in] fieldsToGenerate - the fields to generate
 * @returns the list of wanted fields
 */
static RaveList_t* WrwpInternal_createFieldsList(const char* fieldsToGenerate)
{
  RaveList_t* result = RaveUtilities_getTrimmedTokens(fieldsToGenerate, ',');
  if (RaveList_size(result) == 0) {
    RaveList_add(result, RAVE_STRDUP("ff"));
    RaveList_add(result, RAVE_STRDUP("ff_dev"));
    RaveList_add(result, RAVE_STRDUP("dd"));
    RaveList_add(result, RAVE_STRDUP("dbzh"));
    RaveList_add(result, RAVE_STRDUP("dbzh_dev"));
    RaveList_add(result, RAVE_STRDUP("nz"));
  }
  return result;
}

/**
 * Returns if the specified id exist as an id in the fieldIds.
 * @param[in] fieldIds - the list of ids
 * @param[in] id - the id to query for
 * @returns if the list of ids contains the specified id
 */
static int WrwpInternal_containsField(RaveList_t* fieldIds, const char* id)
{
  int i = 0, n = 0;
  n = RaveList_size(fieldIds);
  for (i = 0; i < n; i++) {
    if (RaveList_get(fieldIds, i) != NULL && strcmp((char*)RaveList_get(fieldIds, i), id) == 0) {
      return 1;
    }
  }
  return 0;
}

static int WrwpInternal_addNodataUndetectGainOffset(RaveField_t* field, double nodata, double undetect, double gain, double offset)
{
  if (!WrwpInternal_addDoubleAttr2Field(field, "what/nodata", nodata) ||
      !WrwpInternal_addDoubleAttr2Field(field, "what/undetect", undetect) ||
      !WrwpInternal_addDoubleAttr2Field(field, "what/gain", gain) ||
      !WrwpInternal_addDoubleAttr2Field(field, "what/offset", offset)) {
    return 0;
  }
  return 1;
}

static RaveDateTime_t* WrwpInternal_getStartDateTimeFromScan(PolarScan_t* scan)
{
  RaveDateTime_t* dt = RAVE_OBJECT_NEW(&RaveDateTime_TYPE);
  RaveDateTime_t* result = NULL;
  if (dt != NULL) {
    if (!RaveDateTime_setDate(dt, PolarScan_getStartDate(scan)) ||
        !RaveDateTime_setTime(dt, PolarScan_getStartTime(scan))) {
      RAVE_WARNING0("Failed to initialize datetime object with start date/time");
      goto done;
    }
  }
  result = RAVE_OBJECT_COPY(dt);
done:
  RAVE_OBJECT_RELEASE(dt);
  return result;
}

static RaveDateTime_t* WrwpInternal_getEndDateTimeFromScan(PolarScan_t* scan)
{
  RaveDateTime_t* dt = RAVE_OBJECT_NEW(&RaveDateTime_TYPE);
  RaveDateTime_t* result = NULL;
  if (dt != NULL) {
    if (!RaveDateTime_setDate(dt, PolarScan_getEndDate(scan)) ||
        !RaveDateTime_setTime(dt, PolarScan_getEndTime(scan))) {
      RAVE_WARNING0("Failed to initialize datetime object with end date/time");
      goto done;
    }
  }
  result = RAVE_OBJECT_COPY(dt);
done:
  RAVE_OBJECT_RELEASE(dt);
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

VerticalProfile_t* Wrwp_generate(Wrwp_t* self, PolarVolume_t* inobj, const char* fieldsToGenerate)
{
  VerticalProfile_t* result = NULL;
  PolarNavigator_t* polnav = NULL;
  int nrhs = NRHS, lda = LDA, ldb = LDB;
  int nscans = 0, nv, nz, i, iz, is, ib, ir;
  int firstInit;

  double gain, offset, nodata, undetect, val;
  double d, h;
  double alpha, beta, gamma, vvel, vdir, vstd, zsum, zmean, zstd;
  double centerOfLayer, u_wnd_comp, v_wnd_comp, vdir_rad;
  int ysize = 0, yindex = 0;
  int countAcceptedScans = 0; /* counter for accepted scans i.e. scans with elangle >= selected
                                 minimum elevatiuon angle and not being set as malfunc */

  const char* product = "VP";
  RaveDateTime_t *firstStartDT = NULL, *lastEndDT = NULL;

  /* Field defs */
  RaveField_t *nv_field = NULL, *hght_field = NULL;
  RaveField_t *uwnd_field = NULL, *vwnd_field = NULL;
  RaveField_t *ff_field = NULL, *ff_dev_field = NULL, *dd_field = NULL;
  RaveField_t *dbzh_field = NULL, *dbzh_dev_field = NULL, *nz_field = NULL;

  RaveList_t* wantedFields = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");

  wantedFields = WrwpInternal_createFieldsList(fieldsToGenerate);

  if (WrwpInternal_containsField(wantedFields, "NV")) nv_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "HGHT")) hght_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "UWND")) uwnd_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "VWND")) vwnd_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "ff")) ff_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "ff_dev")) ff_dev_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "dd")) dd_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "dbzh")) dbzh_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "dbzh_dev")) dbzh_dev_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "nz")) nz_field = RAVE_OBJECT_NEW(&RaveField_TYPE);

  if ((WrwpInternal_containsField(wantedFields, "NV") && nv_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "HGHT") && hght_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "UWND") && uwnd_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "VWND") && vwnd_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "ff") && ff_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "ff_dev") && ff_dev_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "dd") && dd_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "dbzh") && dbzh_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "dbzh_dev") && dbzh_dev_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "nz") && nz_field == NULL))
  {
    RAVE_ERROR0("Failed to allocate memory for the resulting vp fields");
    goto done;
  }

  ysize = self->hmax / self->dz;

  if ((nv_field != NULL && !RaveField_createData(nv_field, 1, ysize, RaveDataType_INT)) ||
      (hght_field != NULL && !RaveField_createData(hght_field, 1, ysize, RaveDataType_DOUBLE)) ||
      (uwnd_field != NULL && !RaveField_createData(uwnd_field, 1, ysize, RaveDataType_DOUBLE)) ||
      (vwnd_field != NULL && !RaveField_createData(vwnd_field, 1, ysize, RaveDataType_DOUBLE)) ||
      (ff_field != NULL && !RaveField_createData(ff_field, 1, ysize, RaveDataType_DOUBLE)) ||
      (ff_dev_field != NULL && !RaveField_createData(ff_dev_field, 1, ysize, RaveDataType_DOUBLE)) ||
      (dd_field != NULL && !RaveField_createData(dd_field, 1, ysize, RaveDataType_DOUBLE)) ||
      (dbzh_field != NULL && !RaveField_createData(dbzh_field, 1, ysize, RaveDataType_DOUBLE)) ||
      (dbzh_dev_field != NULL && !RaveField_createData(dbzh_dev_field, 1, ysize, RaveDataType_DOUBLE)) ||
      (nz_field != NULL && !RaveField_createData(nz_field, 1, ysize, RaveDataType_DOUBLE))) {
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
      char* malfuncString = NULL;
      PolarScan_t* scan = PolarVolume_getScan(inobj, is);      
      long nbins = PolarScan_getNbins(scan);
      long nrays = PolarScan_getNrays(scan);
      double rscale = PolarScan_getRscale(scan);
      double elangleForThisScan = PolarScan_getElangle(scan);
      
      if (elangleForThisScan * RAD2DEG >= self->emin) { /* We only do the calculation for scans with elangle >= the minimum one */
        RaveDateTime_t* startDTofThisScan = WrwpInternal_getStartDateTimeFromScan(scan);
        RaveDateTime_t* endDTofThisScan = WrwpInternal_getEndDateTimeFromScan(scan);        
        RaveAttribute_t* malfuncattr = PolarScan_getAttribute(scan, "how/malfunc");
        if (malfuncattr != NULL) {
          RaveAttribute_getString(malfuncattr, &malfuncString); /* Set the malfuncString if attr is not NULL */
          RAVE_OBJECT_RELEASE(malfuncattr);
        }
        if (malfuncString == NULL || strcmp(malfuncString, "False") == 0) { /* Assuming malfuncString = NULL means no malfunc */
          countAcceptedScans = countAcceptedScans + 1;
          if (firstInit == 0 && iz==0) { /* We only perform the date time calc at first iz-iteration*/
            /* Initialize using the first accepted scan and define 2 strings for the combined datetime */
            firstStartDT = RAVE_OBJECT_COPY(startDTofThisScan);
            lastEndDT = RAVE_OBJECT_COPY(endDTofThisScan);
            firstInit = 1;
          }
          if (firstInit == 1 && countAcceptedScans > 1 && iz==0) {
            if (RaveDateTime_compare(startDTofThisScan, firstStartDT) < 0) {
              /* if start datetime of this scan is before the first saved start datetime, save this one instead */
              RAVE_OBJECT_RELEASE(firstStartDT);
              firstStartDT = RAVE_OBJECT_COPY(startDTofThisScan);
            }
            if (RaveDateTime_compare(endDTofThisScan, lastEndDT) > 0) {
              /* If end datetime of this scan is after the last saved end datetime, save this one instead */
              RAVE_OBJECT_RELEASE(lastEndDT);
              lastEndDT = RAVE_OBJECT_COPY(endDTofThisScan);
            }
          }
          // radial wind scans
          if (PolarScan_hasParameter(scan, "VRAD") || PolarScan_hasParameter(scan, "VRADH")) {
            PolarScanParam_t* vrad = NULL;
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
                PolarNavigator_reToDh(polnav, (ib+0.5)*rscale, elangleForThisScan, &d, &h);
                PolarScanParam_getValue(vrad, ib, ir, &val);
                if ((h >= iz) &&
                    (h < iz + self->dz) &&
                    (d >= self->dmin) &&
                    (d <= self->dmax) &&
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
            PolarScanParam_t* dbz = PolarScan_getParameter(scan, "DBZH");
            gain = PolarScanParam_getGain(dbz);
            offset = PolarScanParam_getOffset(dbz);
            nodata = PolarScanParam_getNodata(dbz);
            undetect = PolarScanParam_getUndetect(dbz);

            for (ir = 0; ir < nrays; ir++) {
              for (ib = 0; ib < nbins; ib++) {
                PolarNavigator_reToDh(polnav, (ib+0.5)*rscale, elangleForThisScan, &d, &h);
                PolarScanParam_getValue (dbz, ib, ir, &val);
                if ((h >= iz) &&
                    (h < iz+self->dz) &&
                    (d >= self->dmin) &&
                    (d <= self->dmax) &&
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
        }
        RAVE_OBJECT_RELEASE(startDTofThisScan);
        RAVE_OBJECT_RELEASE(endDTofThisScan);
      }
      RAVE_OBJECT_RELEASE(scan); 
    }

    if (countAcceptedScans == 0) { /* Emergency exit if no accepted scans were found */
      result = NULL;               /* If this is the case, we don't bother with checking */
      RAVE_FREE(A);                /* the same thing for the other atmospheric layers, */
      RAVE_FREE(b);                /* we skip it and return 0 directly */
      RAVE_FREE(v);
      RAVE_FREE(z);
      RAVE_FREE(az);
      goto done;
    } 

    /* Perform radial wind calculations and reflectivity calculations */
    if (nv>3) {
      //***************************************************************
      // fitting: y = gamma+alpha*sin(x+beta)                         *
      // alpha -> amplitude                                           *
      // beta -> phase shift                                          *
      // gamma -> consider an y-shift due to the terminal velocity of *
      //          falling rain drops                                  *
      //***************************************************************

      /* QR decomposition */
      /*info = */
      LAPACKE_dgels(LAPACK_ROW_MAJOR, 'N', NOR, NOC, nrhs, A, lda, b, ldb);

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
        vstd = vstd + pow (*(v+i) - (gamma+alpha*sin(*(az+i)+beta)),2);
      }
      vstd = sqrt (vstd/(nv-1));
      
      /* Calculate the x-component (East) and y-component (North) of the wind 
         velocity using the wind direction and the magnitude of the wind velocity */
      vdir_rad = vdir * DEG2RAD;
      u_wnd_comp = vvel * cos(vdir_rad);
      v_wnd_comp = vvel * sin(vdir_rad);
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
           
    if ((nv < NMIN) || (nz < NMIN)) {
      if (nv_field != NULL) RaveField_setValue(nv_field, 0, yindex, 0);
      if (hght_field != NULL) RaveField_setValue(hght_field, 0, yindex,centerOfLayer);
      if (uwnd_field != NULL) RaveField_setValue(uwnd_field, 0, yindex,self->nodata_VP);
      if (vwnd_field != NULL) RaveField_setValue(vwnd_field, 0, yindex,self->nodata_VP);
      if (ff_field != NULL) RaveField_setValue(ff_field, 0, yindex, self->nodata_VP);
      if (ff_dev_field != NULL) RaveField_setValue(ff_dev_field, 0, yindex, self->nodata_VP);
      if (dd_field != NULL) RaveField_setValue(dd_field, 0, yindex, self->nodata_VP);
      if (dbzh_field != NULL) RaveField_setValue(dbzh_field, 0, yindex, self->nodata_VP);
      if (dbzh_dev_field != NULL) RaveField_setValue(dbzh_dev_field, 0, yindex, self->nodata_VP);
      if (nz_field != NULL) RaveField_setValue(nz_field, 0, yindex, 0);
    } else {
      if (nv_field != NULL) RaveField_setValue(nv_field, 0, yindex, nv);
      if (hght_field != NULL) RaveField_setValue(hght_field, 0, yindex,centerOfLayer);
      if (uwnd_field != NULL) RaveField_setValue(uwnd_field, 0, yindex,u_wnd_comp);
      if (vwnd_field != NULL) RaveField_setValue(vwnd_field, 0, yindex,v_wnd_comp);
      if (ff_field != NULL) RaveField_setValue(ff_field, 0, yindex, vvel);
      if (ff_dev_field != NULL) RaveField_setValue(ff_dev_field, 0, yindex, vstd);
      if (dd_field != NULL) RaveField_setValue(dd_field, 0, yindex, vdir);
      if (dbzh_field != NULL) RaveField_setValue(dbzh_field, 0, yindex, zmean);
      if (dbzh_dev_field != NULL) RaveField_setValue(dbzh_dev_field, 0, yindex, zstd);
      if (nz_field != NULL) RaveField_setValue(nz_field, 0, yindex, nz);
    }

    RAVE_FREE(A);
    RAVE_FREE(b);
    RAVE_FREE(v);
    RAVE_FREE(z);
    RAVE_FREE(az);

    yindex++;   
  }

  if (uwnd_field) WrwpInternal_addNodataUndetectGainOffset(uwnd_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (vwnd_field) WrwpInternal_addNodataUndetectGainOffset(vwnd_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (hght_field) WrwpInternal_addNodataUndetectGainOffset(hght_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (nv_field) WrwpInternal_addNodataUndetectGainOffset(nv_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (ff_field) WrwpInternal_addNodataUndetectGainOffset(ff_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (ff_dev_field) WrwpInternal_addNodataUndetectGainOffset(ff_dev_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (dd_field) WrwpInternal_addNodataUndetectGainOffset(dd_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (dbzh_field) WrwpInternal_addNodataUndetectGainOffset(dbzh_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (dbzh_dev_field) WrwpInternal_addNodataUndetectGainOffset(dbzh_dev_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);

  result = RAVE_OBJECT_NEW(&VerticalProfile_TYPE);
  if (result != NULL) {
    if ((uwnd_field != NULL && !VerticalProfile_setUWND(result, uwnd_field)) ||
        (vwnd_field != NULL && !VerticalProfile_setVWND(result, vwnd_field)) ||
        (nv_field != NULL && !VerticalProfile_setNV(result, nv_field)) ||
        (hght_field != NULL && !VerticalProfile_setHGHT(result, hght_field)) ||
        (ff_field != NULL && !VerticalProfile_setFF(result, ff_field)) ||
        (ff_dev_field != NULL && !VerticalProfile_setFFDev(result, ff_dev_field)) ||
        (dd_field != NULL && !VerticalProfile_setDD(result, dd_field)) ||
        (dbzh_field != NULL && !VerticalProfile_setDBZ(result, dbzh_field)) ||
        (dbzh_dev_field != NULL && !VerticalProfile_setDBZDev(result, dbzh_dev_field))) {
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
  VerticalProfile_setStartDate(result, RaveDateTime_getDate(firstStartDT));
  VerticalProfile_setStartTime(result, RaveDateTime_getTime(firstStartDT));
  VerticalProfile_setEndDate(result, RaveDateTime_getDate(lastEndDT));
  VerticalProfile_setEndTime(result, RaveDateTime_getTime(lastEndDT));
  VerticalProfile_setProduct(result, product);
   
  /* Need to filter the attribute data with respect to selected min elangle
     Below attributes satisfy reguirements from E-profile, add other when needed */
  WrwpInternal_findAndAddAttribute(result, inobj, "how/highprf", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/lowprf", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/pulsewidth", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/wavelength", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/RXbandwidth", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/RXlossH", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/TXlossH", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/antgainH", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/azmethod", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/binmethod", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/malfunc", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/nomTXpower", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/radar_msg", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/radconstH", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/radomelossH", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/rpm", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/software", self->emin);
  WrwpInternal_findAndAddAttribute(result, inobj, "how/system", self->emin);
  WrwpInternal_addIntAttribute(result, "how/minrange", Wrwp_getDMIN(self));
  WrwpInternal_addIntAttribute(result, "how/maxrange", Wrwp_getDMAX(self));

done:
  RAVE_OBJECT_RELEASE(polnav);
  RAVE_OBJECT_RELEASE(ff_field);
  RAVE_OBJECT_RELEASE(ff_dev_field);
  RAVE_OBJECT_RELEASE(dd_field);
  RAVE_OBJECT_RELEASE(dbzh_field);
  RAVE_OBJECT_RELEASE(dbzh_dev_field);
  RAVE_OBJECT_RELEASE(nz_field);
  RAVE_OBJECT_RELEASE(nv_field);
  RAVE_OBJECT_RELEASE(hght_field);
  RAVE_OBJECT_RELEASE(uwnd_field);
  RAVE_OBJECT_RELEASE(vwnd_field);
  RAVE_OBJECT_RELEASE(firstStartDT);
  RAVE_OBJECT_RELEASE(lastEndDT);
  RaveList_freeAndDestroy(&wantedFields);

  return result;
}

/*@} End of Interface functions */

RaveCoreObjectType Wrwp_TYPE = {
    "Wrwp",
    sizeof(Wrwp_t),
    Wrwp_constructor,
    Wrwp_destructor
};

