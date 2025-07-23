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
 * modernized Swedish radars. If such a file is encountered, the return is just
 * NULL and the exception following is treated in the pgf.
 * 
 * @date 2020-01-15, corrected an bug in the wind direction calculation. In addition, added functionality
 * for the new parameters EMAX and FF_MAX plus activated the parameter NZ.
 *
 * The majority of the parameters put in the file wrwp.h have been moved into a separate file (wrwp_config.xml) 
 * so that they can be changed without compiling the code. The plugin has been modified so it reads the xml-file 
 * the relevant parameters not set from the web-GUI and a newly created python command-line tool (wrwp_main.py 
 * instead of wrwp_main.c) also reads the xml-file and sets the relevant parameters.
 *
 * @author Hidde Leijnse, KNMI
 * @date 2025-05-22, inserted KNMI algorithms for screening data going into the wind profile fit.*/

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
  int nmin_wnd; /**< Minimum sample size for wind */
  int nmin_ref; /**< Minimum sample size for refl. */
  double emin; /**< Minimum elevation angle [deg] */
  double emax; /**< Maximum elevation angle [deg] */
  double econdmax; /**< Conditional maximum elevation angle [deg] */
  double hthr; /**< Height threshold below which conditional elevantion angle is employed [m] */
  double nimin; /**< Minimum Nyquist interval for use of scan [m/s] */
  int ngapbin; /**< Number of azimuth sector bins for detecting gaps */
  int ngapmin; /**< Minimum number of samples within an azimuth sector bin */
  int maxnstd; /**< Maximum number standard deviations of residuals to include samples */
  double maxvdiff; /**< Maximum deviation of a samples to the fit [m/s] */
  double ff_max; /**<Maximum accepted value for ff (calculated layer velocity) [m/s]*/
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
  wrwp->dmin = DMIN;
  wrwp->dmax = DMAX;
  wrwp->nmin_wnd = NMIN_WND;
  wrwp->nmin_ref = NMIN_REF;
  wrwp->emin = EMIN;
  wrwp->emax = EMAX;
  wrwp->econdmax = ECONDMAX;
  wrwp->hthr = HTHR;
  wrwp->nimin = NIMIN;
  wrwp->ngapbin = NGAPBIN;
  wrwp->ngapmin = NGAPMIN;
  wrwp->maxnstd = MAXNSTD;
  wrwp->maxvdiff = MAXVDIFF;
  wrwp->vmin = VMIN;
  wrwp->ff_max = FF_MAX;
  wrwp->dz = DZ;
  wrwp->hmax = HMAX;
  wrwp->nodata_VP = NODATA_VP;
  wrwp->undetect_VP = UNDETECT_VP;
  wrwp->gain_VP = GAIN_VP; /* The gain cannot be initialized to 0.0! */
  wrwp->offset_VP = OFFSET_VP;
  return 1;
}

/**
 * Destructor
 */
static void Wrwp_destructor(RaveCoreObject* obj)
{
}

static int WrwpInternal_findAndAddAttribute(VerticalProfile_t* vp, PolarVolume_t* pvol, const char* name, double minSelAng, double maxSelAng)
{
  int nscans = PolarVolume_getNumberOfScans(pvol);
  int i = 0;
  int found = 0;
  double elangleForThisScan = 0.0;
  
  for (i = 0; i < nscans && found == 0; i++) {
    PolarScan_t* scan = PolarVolume_getScan(pvol, i);
    if (scan != NULL && PolarScan_hasAttribute(scan, name)) {
        elangleForThisScan = PolarScan_getElangle(scan);
        
        /* Filter with respect to the selected min elangle and max elangle*/
        if ((elangleForThisScan * RAD2DEG) >= minSelAng && (elangleForThisScan * RAD2DEG) <= maxSelAng) {
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

static int WrwpInternal_addDoubleAttribute(VerticalProfile_t* vp, const char* name, double value)
{
  RaveAttribute_t* attr = RaveAttributeHelp_createDouble(name, value);
  int result = 0;
  if (attr != NULL) {
    result = VerticalProfile_addAttribute(vp, attr);
  }
  RAVE_OBJECT_RELEASE(attr);
  return result;
}

static int WrwpInternal_addStringAttribute(VerticalProfile_t* vp, const char* name, const char* value)
{
  RaveAttribute_t* attr = RaveAttributeHelp_createString(name, value);
  int result = 0;
  if (attr != NULL) {
    result = VerticalProfile_addAttribute(vp, attr);
  }
  RAVE_OBJECT_RELEASE(attr);
  return result;
}
 
static char* WrwpInternal_LastcharDel(char* name)
{
  int i = 0;
  while(name[i] != '\0')
  {
  i++;       
  }
  name[i-1] = '\0';
  return name;
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
    RaveList_add(result, RAVE_STRDUP("NV"));
    RaveList_add(result, RAVE_STRDUP("DBZH"));
    RaveList_add(result, RAVE_STRDUP("DBZH_dev"));
    RaveList_add(result, RAVE_STRDUP("NZ"));
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
/* Adds attributes under a fileds what */
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

/* Gathering of startdatetime and enddatetime */
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


int WrwpInternal_getDoubleAttribute(RaveCoreObject* obj, const char* aname, double* tmpd) {
    RaveAttribute_t* attr = NULL;
    int ret = 0;

    if (RAVE_OBJECT_CHECK_TYPE(obj, &PolarVolume_TYPE)) {
        attr = PolarVolume_getAttribute((PolarVolume_t*)obj, aname);
    } else if (RAVE_OBJECT_CHECK_TYPE(obj, &PolarScan_TYPE)) {
        attr = PolarScan_getAttribute((PolarScan_t*)obj, aname);
    } else if (RAVE_OBJECT_CHECK_TYPE(obj, &PolarScanParam_TYPE)) {
        attr = PolarScanParam_getAttribute((PolarScanParam_t*)obj, aname);
    }
    if (attr != NULL) {
        ret = RaveAttribute_getDouble(attr, tmpd);
    }
    RAVE_OBJECT_RELEASE(attr);
    return ret;
}


/**
 * This function detects gaps in the azimuthal distribution. For this, a histogram of the azimuths of the available
 * velocity data is made using 'nGapBin' azimuth bins. Subsequently, the number of data points per azimuth
 * bin is determined. When two consecutive azimuth bins contain less than 'nGapMin' points, a gap is detected.
 * @param[in] az - the azimuths for which there is valid data
 * @param[in] Npnt - the number of azimuths in az
 * @param[in] nGapBin - the number of azimuth bins to use for detecting gaps (bin size is hence (360 / nGapBin)
 * @param[in] nGapMin - the minimum number of valid points within a bin necessary to avoid returning gap = 1
 * @returns true (1) when a gap is detected and false (0) when no gap is found.
 */
int WrwpInternal_azimuthGap(double *az, int Npnt, int nGapBin, int nGapMin)
{
    int gap, Nsector[nGapBin], n, m;
    
    /*Initialize histogram.*/
    gap = 0;
    for (m = 0; m < nGapBin; m++) Nsector[m] = 0;
    
    /*Collect histogram.*/
    for (n = 0; n < Npnt; n++)
    {
        m = (az[n] * RAD2DEG * nGapBin) / 360.0;
        Nsector[m % nGapBin]++;
    }
    
    /*Detect gaps.*/
    gap = 0;
    for (m = 0; m < nGapBin; m++)
    {
        if ((Nsector[m] < nGapMin) && (Nsector[(m + 1) % nGapBin] < nGapMin)) gap = 1;
    }
    
    return gap;
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

void Wrwp_setGAIN_VP(Wrwp_t* self, double gain_VP)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  if (gain_VP == 0.0) {
    RAVE_ERROR0("Trying to set gain to 0.0");
    return;
  }
  self->gain_VP = gain_VP;
}

double Wrwp_getGAIN_VP(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->gain_VP;
}

void Wrwp_setOFFSET_VP(Wrwp_t* self, double offset_VP)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->offset_VP = offset_VP;
}

double Wrwp_getOFFSET_VP(Wrwp_t* self)
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

void Wrwp_setNMIN_WND(Wrwp_t* self, int nmin_wnd)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->nmin_wnd = nmin_wnd;
}

int Wrwp_getNMIN_WND(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->nmin_wnd;
}

void Wrwp_setNMIN_REF(Wrwp_t* self, int nmin_ref)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->nmin_ref = nmin_ref;
}

int Wrwp_getNMIN_REF(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->nmin_ref;
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

void Wrwp_setEMAX(Wrwp_t* self, double emax)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->emax = emax;
}

double Wrwp_getEMAX(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->emax;
}

void Wrwp_setECONDMAX(Wrwp_t* self, double econdmax)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->econdmax = econdmax;
}

double Wrwp_getECONDMAX(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->econdmax;
}

void Wrwp_setHTHR(Wrwp_t* self, double hthr)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->hthr = hthr;
}

double Wrwp_getHTHR(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->hthr;
}

void Wrwp_setNIMIN(Wrwp_t* self, double nimin)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->nimin = nimin;
}

double Wrwp_getNIMIN(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->nimin;
}

void Wrwp_setNGAPBIN(Wrwp_t* self, int ngapbin)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->ngapbin = ngapbin;
}

int Wrwp_getNGAPBIN(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->ngapbin;
}

void Wrwp_setNGAPMIN(Wrwp_t* self, int ngapmin)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->ngapmin = ngapmin;
}

int Wrwp_getNGAPMIN(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->ngapmin;
}

void Wrwp_setMAXNSTD(Wrwp_t* self, int maxnstd)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->maxnstd = maxnstd;
}

int Wrwp_getMAXNSTD(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->maxnstd;
}

void Wrwp_setMAXVDIFF(Wrwp_t* self, double maxvdiff)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->maxvdiff = maxvdiff;
}

double Wrwp_getMAXVDIFF(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->maxvdiff;
}

void Wrwp_setFF_MAX(Wrwp_t* self, double ff_max)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->ff_max = ff_max;
}

double Wrwp_getFF_MAX(Wrwp_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->ff_max;
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

/* Main code for vertical profile generation */
VerticalProfile_t* Wrwp_generate(Wrwp_t* self, PolarVolume_t* inobj, const char* wrwpMethod, const char* fieldsToGenerate)
{
  VerticalProfile_t* result = NULL;
  PolarNavigator_t* polnav = NULL;
  int nrhs = NRHS, lda = LDA, ldb = LDB;
  int nscans = 0, nv, nz, i, iz, is, ib, ir, n, m, p;
  int firstInit;

  double gain, offset, nodata, undetect, val, NI, chisq, Vdifmax;
  double d, h;
  double alpha, beta, gamma, vvel, vdir, vstd, zsum, zmean, zstd;
  double centerOfLayer=0.0, u_wnd_comp=0.0, v_wnd_comp=0.0, vdir_rad=0.0;
  int ysize = 0, yindex = 0;
  int countAcceptedScans = 0; /* counter for accepted scans i.e. scans with elangle >= selected
                                 minimum elevatiuon angle and <= selected maximum elevation angle and not being set as malfunc */

  const char* product = "VP";

  RaveDateTime_t *firstStartDT = NULL, *lastEndDT = NULL;

  /* Field definitions */
  RaveField_t *nv_field = NULL, *hght_field = NULL;
  RaveField_t *uwnd_field = NULL, *vwnd_field = NULL;
  RaveField_t *ff_field = NULL, *ff_dev_field = NULL, *dd_field = NULL;
  RaveField_t *dbzh_field = NULL, *dbzh_dev_field = NULL, *nz_field = NULL;

  RaveList_t* wantedFields = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");
  RAVE_ASSERT((self->gain_VP != 0.0), "gain_VP == 0.0");

  wantedFields = WrwpInternal_createFieldsList(fieldsToGenerate);

  if (WrwpInternal_containsField(wantedFields, "NV")) nv_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "HGHT")) hght_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "UWND")) uwnd_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "VWND")) vwnd_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "ff")) ff_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "ff_dev")) ff_dev_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "dd")) dd_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "DBZH")) dbzh_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "DBZH_dev")) dbzh_dev_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (WrwpInternal_containsField(wantedFields, "NZ")) nz_field = RAVE_OBJECT_NEW(&RaveField_TYPE);

  if ((WrwpInternal_containsField(wantedFields, "NV") && nv_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "HGHT") && hght_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "UWND") && uwnd_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "VWND") && vwnd_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "ff") && ff_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "ff_dev") && ff_dev_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "dd") && dd_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "DBZH") && dbzh_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "DBZH_dev") && dbzh_dev_field == NULL) ||
      (WrwpInternal_containsField(wantedFields, "NZ") && nz_field == NULL))
  {
    RAVE_ERROR0("Failed to allocate memory for the resulting vp fields");
    goto done;
  }

  /* Set the spacing */
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

  // Allocate memory, and initialize with zeros, for the char array holding the accepted elevation angles.
  // Define and initialize also the accessory strings and the counter.
  char *theUsedElevationAngles = RAVE_CALLOC((size_t)(100), sizeof (char));
  char angle[6] = {'\0'};
  char comma[2] = ",";
  int firstAngleCounter = 0;
  double acceptedAngle = 0.0;
  
  // Some definitions  and initializations of things used for putting a unique set of tasks into an array
  #define len(A) (sizeof(A) / sizeof(A[0]))
  char *taskArgs[] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
  int scanNumber = 0;
  int foundTask = 0;
  int ntask = 0;

  // Loop over the atmospheric layers
    // NOTE: looping over all height layers, and then looping over all elevations, azimuths, and ranges may be inefficient in terms of CPU use. With a little more memory use, this could be reduced. Not sure if this is at all relevant, but it could be an option to look into if necessary.
  for (iz = 0; iz < self->hmax; iz += self->dz) {
    /* allocate memory and initialize with zeros */
    double *A = RAVE_CALLOC((size_t)(NOR*NOC), sizeof (double));
    double *Atmp = RAVE_CALLOC((size_t)(NOR*NOC), sizeof (double));
    double *b = RAVE_CALLOC((size_t)(NOR), sizeof (double));
    double *v = RAVE_CALLOC((size_t)(NOR), sizeof (double));
    double *vfit = RAVE_CALLOC((size_t)(NOR), sizeof (double));
    double *z = RAVE_CALLOC((size_t)(NOR), sizeof (double));
    double *az = RAVE_CALLOC((size_t)(NOR), sizeof (double));
    double *el = RAVE_CALLOC((size_t)(NOR), sizeof (double));

    vdir = -9999.0;
    vvel = -9999.0;
    vstd = 0.0;
    zsum = 0.0;
    zmean = -9999.0;
    zstd = 0.0;
    nv = 0;
    nz = 0;
    
    /* Define the center height of each vertical layer, this will later
       become the HGHT array */
    centerOfLayer = iz + (self->dz / 2.0);
    firstInit = 0;

    // Loop over scans in the polar volume
    for (is = 0; is < nscans; is++) {
      char* malfuncString = NULL;
      char* taskString = NULL;
      PolarScan_t* scan = PolarVolume_getScan(inobj, is);      
      long nbins = PolarScan_getNbins(scan);
      long nrays = PolarScan_getNrays(scan);
      double rscale = PolarScan_getRscale(scan);
      double elangleForThisScan = PolarScan_getElangle(scan);
      
      if (elangleForThisScan * RAD2DEG >= self->emin && elangleForThisScan * RAD2DEG <= self->emax) { /* We only do the calculation for scans with elangle >= the minimum one AND elangle <= the maximum one */
        RaveDateTime_t* startDTofThisScan = WrwpInternal_getStartDateTimeFromScan(scan);
        RaveDateTime_t* endDTofThisScan = WrwpInternal_getEndDateTimeFromScan(scan);        
        RaveAttribute_t* malfuncattr = PolarScan_getAttribute(scan, "how/malfunc");
        RaveAttribute_t* taskattr = PolarScan_getAttribute(scan, "how/task");

        if (malfuncattr != NULL) {
          RaveAttribute_getString(malfuncattr, &malfuncString); /* Set the malfuncString if attr is not NULL */
          RAVE_OBJECT_RELEASE(malfuncattr);
        }

        if (taskattr != NULL) {
          RaveAttribute_getString(taskattr, &taskString); /* Set the taskString if attr is not NULL */
          RAVE_OBJECT_RELEASE(taskattr);
        }

        if (malfuncString == NULL || strcmp(malfuncString, "False") == 0) { /* Assuming malfuncString = NULL means no malfunc */
          countAcceptedScans = countAcceptedScans + 1;

          if (iz == 0) { /* Collect the elevation angles only for the first atmospheric layer */
            acceptedAngle = elangleForThisScan * RAD2DEG;
            sprintf(angle, "%2.1f", acceptedAngle);

            if (firstAngleCounter == 0) {
              strcat(theUsedElevationAngles, angle);
              firstAngleCounter = 1;
              taskArgs[scanNumber] = taskString;
              scanNumber = scanNumber + 1; 
            }
            else {
              strcat(theUsedElevationAngles, comma);
              strcat(theUsedElevationAngles, angle);
              for (ntask=0; ntask < len(taskArgs); ntask++) {
                if (taskArgs[ntask]) {
                  if (strcmp(taskArgs[ntask], taskString) == 0) {
                    foundTask = 1;
                    break;
                  }
                }
              }
              if (!foundTask) {
                taskArgs[scanNumber] = taskString;
                foundTask = 0; 
              } 
              scanNumber = scanNumber + 1;
            }
          }
   
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
            
            // KNMI algorithm: check for minimum Nyquist interval
            NI = fabs(offset);
            if (strcmp(wrwpMethod, "KNMI") == 0) {
                if (!WrwpInternal_getDoubleAttribute((RaveCoreObject*)scan, "how/NI", &NI)) {
                    if (!WrwpInternal_getDoubleAttribute((RaveCoreObject*)inobj, "how/NI", &NI)) {
                        NI = fabs(offset);
                    }
                }
            }
            if ((strcmp(wrwpMethod, "KNMI") != 0) || (NI >= self->nimin)) {
            for (ir = 0; ir < nrays; ir++) {
              for (ib = 0; ib < nbins; ib++) {
                PolarNavigator_reToDh(polnav, (ib+0.5)*rscale, elangleForThisScan, &d, &h);
                PolarScanParam_getValue(vrad, ib, ir, &val);
                if (((strcmp(wrwpMethod, "KNMI") != 0) || (elangleForThisScan * RAD2DEG <= self->econdmax) || (h >= self->hthr)) && ((h >= iz) &&
                    (h < iz + self->dz) &&
                    (d >= self->dmin) &&
                    (d <= self->dmax) &&
                    (val != nodata) &&
                    (val != undetect) &&
                    (abs(offset + gain * val) >= self->vmin))) {
                  if (nv < NOR) {
                    *(v+nv) = offset+gain*val;
                    *(az+nv) = 360./nrays*ir*DEG2RAD;
                    *(el+nv) = elangleForThisScan;
                    *(A+nv*NOC) = sin(*(az+nv));
                    *(A+nv*NOC+1) = cos(*(az+nv));
                    *(A+nv*NOC+2) = 1;
                    if (strcmp(wrwpMethod, "KNMI") == 0) {
                        *(A+nv*NOC) *= cos(elangleForThisScan);
                        *(A+nv*NOC+1) *= cos(elangleForThisScan);
                        *(A+nv*NOC+2) *= sin(elangleForThisScan);
                    }
                    *(b+nv) = *(v+nv);
                    nv = nv+1;
                  } else {
                    RAVE_ERROR0("NV too great, ignoring value");
                  }
                }
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
                  if (nz < NOR) {
                    *(z+nz) = dBZ2Z(offset+gain*val);
                    zsum = zsum + *(z+nz);
                    nz = nz+1;
                  } else {
                    RAVE_ERROR0("NZ too great, ignoring value");
                  }
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
      RAVE_FREE(Atmp);             /* we skip it and return 0 directly */
      RAVE_FREE(b);
      RAVE_FREE(v);
      RAVE_FREE(vfit);
      RAVE_FREE(z);
      RAVE_FREE(az);
      RAVE_FREE(el);
      RAVE_INFO0("Could not find any acceptable scans, dropping out...");
      goto done;
    }
      
    // KNMI processing: check for azimuth gaps
    if (strcmp(wrwpMethod, "KNMI") == 0) if (WrwpInternal_azimuthGap(az, nv, self->ngapbin, self->ngapmin)) nv = 0;

    /* Perform radial wind calculations and reflectivity calculations */
    if (nv>3) {
      //***************************************************************
      // fitting: y = gamma+alpha*sin(x+beta)                         *
      // alpha -> amplitude                                           *
      // beta -> phase shift                                          *
      // gamma -> consider an y-shift due to the terminal velocity of *
      //          falling rain drops                                  *
      //***************************************************************

        
        if (strcmp(wrwpMethod, "KNMI") == 0) {
            // Do first fit
            for (i = 0; i < (nv * NOC); i++) Atmp[i] = A[i];
            LAPACKE_dgels(LAPACK_ROW_MAJOR, 'N', nv, NOC, nrhs, Atmp, lda, b, ldb);
            
            // Compute vfit and chi-squared
            chisq = 0.0;
            for (i = 0; i < nv; i++) {
                vfit[i] = b[0] * sin(az[i]) * cos(el[i]) + b[1] * cos(az[i]) * cos(el[i]) + b[2] * sin(el[i]);
                chisq += (v[i] - vfit[i]) * (v[i] - vfit[i]);
            }
            chisq /= (nv - NOC);
            
            // Remove outiers
            if (self->maxnstd > 0) Vdifmax = self->maxnstd * sqrt(chisq);
            else Vdifmax = self->maxvdiff;
            n = 0;
            for (m = 0; m < nv; m++)
            {
                if (fabs(v[m] - vfit[m]) < Vdifmax)
                {
                    v[n] = v[m];
                    b[n] = v[m];
                    az[n] = az[m];
                    el[n] = el[m];
                    for (p = 0; p < NOC; p++) Atmp[p + NOC * n] = A[p + NOC * m];
                    n++;
                }
            }
            nv = n;
            
            if (nv > 3) {
                // Check for azimuth gaps and redo fitting if no gaps are there
                if (WrwpInternal_azimuthGap(az, nv, self->ngapbin, self->ngapmin)) nv = 0;
                else {
                    LAPACKE_dgels(LAPACK_ROW_MAJOR, 'N', nv, NOC, nrhs, Atmp, lda, b, ldb);
                    chisq = 0.0;
                    for (i = 0; i < nv; i++) {
                        vfit[i] = b[0] * sin(az[i]) * cos(el[i]) + b[1] * cos(az[i]) * cos(el[i]) + b[2] * sin(el[i]);
                        chisq += (v[i] - vfit[i]) * (v[i] - vfit[i]);
                    }
                    chisq /= (nv - NOC);
                }
            }
        } else {
            /* QR decomposition */
            /*info = */
            LAPACKE_dgels(LAPACK_ROW_MAJOR, 'N', NOR, NOC, nrhs, A, lda, b, ldb);
            chisq = 0.0;
            for (i = 0; i < nv; i++) {
                vfit[i] = b[0] * sin(az[i]) + b[1] * cos(az[i]) + b[2];
                chisq += (v[i] - vfit[i]) * (v[i] - vfit[i]);
            }
            chisq /= nv;
        }
    }
        
    if (nv > 3) {
      /* parameter of the wind model */
      alpha = sqrt(pow(*(b),2) + pow(*(b+1),2));
      beta = atan2(*(b+1), *b);
      gamma = *(b+2);

      /* wind velocity */
      vvel = alpha;

      /* wind direction */
      vdir = 0;
      if (alpha < 0) {
        vdir = (M_PI/2-beta) * RAD2DEG;
      } else if (alpha > 0) {
        vdir = (3*M_PI/2-beta) * RAD2DEG;
      }
      if (vdir < 0) {
        vdir = vdir + 360;
      } else if (vdir > 360) {
        vdir = vdir - 360;
      }

      /* RMSE of the wind velocity*/
      vstd = sqrt (chisq);
      
      /* Calculate the x-component (East) and y-component (North) of the wind 
         velocity using the wind direction and the magnitude of the wind velocity */
      vdir_rad = vdir * DEG2RAD;
      u_wnd_comp = vvel * sin(vdir_rad - M_PI);
      v_wnd_comp = vvel * cos(vdir_rad - M_PI);
    }

    // reflectivity calculations
    if (nz > 0) {
      /* RMSE of the reflectivity */
      for (i = 0; i < nz; i++) {
        zstd = zstd + pow(*(z+i) - (zsum/nz),2);
      }
      zmean = Z2dBZ(zsum/nz);
      zstd = sqrt(zstd/nz);
      zstd = Z2dBZ(zstd);
    }

    /* Set the hght_field values */
    if (hght_field != NULL) RaveField_setValue(hght_field, 0, yindex, centerOfLayer / 1000.0); /* in km */

    /* If the number of points for wind is smaller than the threshold nmin_wnd or the calculated wind velocity is larger than */
    /* threshold ff_max, set nodata, otherwise set values. */
    if (((strcmp(wrwpMethod, "KNMI") != 0) && ((nv < self->nmin_wnd) || (vvel > self->ff_max))) || ((strcmp(wrwpMethod, "KNMI") == 0) && (nv <= 3))) {
      if (nv_field != NULL) RaveField_setValue(nv_field, 0, yindex, -1.0); /* nodata for counter */
      if (uwnd_field != NULL) RaveField_setValue(uwnd_field, 0, yindex, self->nodata_VP);
      if (vwnd_field != NULL) RaveField_setValue(vwnd_field, 0, yindex, self->nodata_VP);
      if (ff_field != NULL) RaveField_setValue(ff_field, 0, yindex, self->nodata_VP);
      if (ff_dev_field != NULL) RaveField_setValue(ff_dev_field, 0, yindex, self->nodata_VP);
      if (dd_field != NULL) RaveField_setValue(dd_field, 0, yindex, self->nodata_VP);
    } else {
      if (nv_field != NULL) RaveField_setValue(nv_field, 0, yindex, nv);
      if (uwnd_field != NULL) RaveField_setValue(uwnd_field, 0, yindex, (u_wnd_comp - self->offset_VP)/self->gain_VP);
      if (vwnd_field != NULL) RaveField_setValue(vwnd_field, 0, yindex, (v_wnd_comp - self->offset_VP)/self->gain_VP);
      if (ff_field != NULL) RaveField_setValue(ff_field, 0, yindex, (vvel - self->offset_VP)/self->gain_VP);
      if (ff_dev_field != NULL) RaveField_setValue(ff_dev_field, 0, yindex, (vstd - self->offset_VP)/self->gain_VP);
      if (dd_field != NULL) RaveField_setValue(dd_field, 0, yindex, (vdir - self->offset_VP)/self->gain_VP);
    }

    /* If the number of points for reflectivity is larger than threshold, set values, else set nodata */
    if (nz < self->nmin_ref) {
      if (nz_field != NULL) RaveField_setValue(nz_field, 0, yindex, -1.0);
      if (dbzh_field != NULL) RaveField_setValue(dbzh_field, 0, yindex, self->nodata_VP);
      if (dbzh_dev_field != NULL) RaveField_setValue(dbzh_dev_field, 0, yindex, self->nodata_VP);
    } else {
      if (nz_field != NULL) RaveField_setValue(nz_field, 0, yindex, nz);
      if (dbzh_field != NULL) RaveField_setValue(dbzh_field, 0, yindex, (zmean - self->offset_VP)/self->gain_VP);
      if (dbzh_dev_field != NULL) RaveField_setValue(dbzh_dev_field, 0, yindex, (zstd - self->offset_VP)/self->gain_VP);
    }

    RAVE_FREE(A);
    RAVE_FREE(Atmp);
    RAVE_FREE(b);
    RAVE_FREE(v);
    RAVE_FREE(vfit);
    RAVE_FREE(z);
    RAVE_FREE(az);
    RAVE_FREE(el);


    yindex++;   
  }

  if (uwnd_field) WrwpInternal_addNodataUndetectGainOffset(uwnd_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (vwnd_field) WrwpInternal_addNodataUndetectGainOffset(vwnd_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (hght_field) WrwpInternal_addNodataUndetectGainOffset(hght_field, -9999.0, -9999.0, 1.0, 0.0);
  if (nv_field) WrwpInternal_addNodataUndetectGainOffset(nv_field, -1.0, -1.0, 1.0, 0.0);
  if (ff_field) WrwpInternal_addNodataUndetectGainOffset(ff_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (ff_dev_field) WrwpInternal_addNodataUndetectGainOffset(ff_dev_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (dd_field) WrwpInternal_addNodataUndetectGainOffset(dd_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (dbzh_field) WrwpInternal_addNodataUndetectGainOffset(dbzh_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (dbzh_dev_field) WrwpInternal_addNodataUndetectGainOffset(dbzh_dev_field, self->nodata_VP, self->undetect_VP, self->gain_VP, self->offset_VP);
  if (nz_field) WrwpInternal_addNodataUndetectGainOffset(nz_field, -1.0, -1.0, 1.0, 0.0);

  result = RAVE_OBJECT_NEW(&VerticalProfile_TYPE);
  if (result != NULL) {
    VerticalProfile_setLevels(result, ysize);

    /* Below are the fields that are possible to inject into the profile, note */
    /* that the presence of the nz_field violates the ODIM spec which defines only one sample size. */
    /* We allow anyway TWO sample size arrays i.e. nv (named n in the output profile) and nz, current spec */
    /* contains only one sample size array n, with obvious consequences IF a */
    /* combined (wind + refl), or a pure refl, profile is created */

    if ((uwnd_field != NULL && !VerticalProfile_setUWND(result, uwnd_field)) ||
        (vwnd_field != NULL && !VerticalProfile_setVWND(result, vwnd_field)) ||
        (nv_field != NULL && !VerticalProfile_setNV(result, nv_field)) ||
        (nz_field != NULL && !VerticalProfile_setNZ(result, nz_field)) ||
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
   
  /* Supported but not included how attributes, add when needed*/
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/highprf", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/lowprf", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/pulsewidth", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/wavelength", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/RXbandwidth", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/RXlossH", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/TXlossH", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/antgainH", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/azmethod", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/binmethod", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/malfunc", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/nomTXpower", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/radar_msg", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/radconstH", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/radomelossH", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/rpm", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/software", self->emin, self->emax);
  //WrwpInternal_findAndAddAttribute(result, inobj, "how/system", self->emin, self->emax);
  
  /*Dealing with the array of strings containing uniques how/task attribute
    This is converted to a string that will represent the unique tasks for scans in the volume
    Note that for radar data not having this attribute in their volumes, it will not be written 
    Initialization of some useful variables */
  ntask = len(taskArgs);
  char finalTasks[300] = {'\0'};
  int taskCount = 0;

  for (i = 0; i < ntask; i++) {
    if (taskArgs[i]) {
      taskCount = taskCount + 1;
      strcat(finalTasks, taskArgs[i]);
      strcat(finalTasks, comma);
    }
  }
  if (taskCount != 0) {
    WrwpInternal_LastcharDel(finalTasks); 
    WrwpInternal_addStringAttribute(result, "how/task", finalTasks);
  }
  /* how attributes requested by Eprofile */
  WrwpInternal_addStringAttribute(result, "how/angles", theUsedElevationAngles);
  WrwpInternal_addDoubleAttribute(result, "how/minrange", (double)Wrwp_getDMIN(self) / 1000.0); /* km */
  WrwpInternal_addDoubleAttribute(result, "how/maxrange", (double)Wrwp_getDMAX(self) / 1000.0); /* km */
  RAVE_FREE(theUsedElevationAngles);

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

