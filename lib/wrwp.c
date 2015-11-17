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
 */

#include "wrwp.h"
#include "rave_debug.h"
#include "rave_alloc.h"
#include <string.h>

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
  return 1;
}

/**
 * Destructor
 */
static void Wrwp_destructor(RaveCoreObject* obj)
{
}

static int WrwpInternal_findAndAddAttribute(VerticalProfile_t* vp, PolarVolume_t* pvol, const char* name)
{
  int nscans = PolarVolume_getNumberOfScans(pvol);
  int i = 0;
  int found = 0;
  for (i = 0; i < nscans && found == 0; i++) {
    PolarScan_t* scan = PolarVolume_getScan(pvol, i);
    if (scan != NULL && PolarScan_hasAttribute(scan, name)) {
      RaveAttribute_t* attr = PolarScan_getAttribute(scan, name);
      VerticalProfile_addAttribute(vp, attr);
      found = 1;
      RAVE_OBJECT_RELEASE(attr);
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
	double rscale, elangle, gain, offset, nodata, undetect, val;
	double d, h;
	double alpha, beta, gamma, vvel, vdir, vstd, zsum, zmean, zstd;
	int ysize = 0, yindex = 0;

	RaveField_t *ff_field = NULL, *ff_dev_field = NULL, *dd_field = NULL;
	RaveField_t *dbzh_field = NULL, *dbzh_dev_field = NULL, *nz_field = NULL, *nv_field = NULL;

	RAVE_ASSERT((self != NULL), "self == NULL");

	ff_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  ff_dev_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  dd_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  dbzh_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  dbzh_dev_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  nz_field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  nv_field = RAVE_OBJECT_NEW(&RaveField_TYPE);

  if (ff_field == NULL || ff_dev_field == NULL || dd_field == NULL ||
      dbzh_field == NULL || dbzh_dev_field == NULL || nz_field == NULL || nv_field == NULL) {
    RAVE_ERROR0("Failed to allocate memory for the resulting vp fields");
    goto done;
  }
  ysize = self->hmax / self->dz;
  if (!RaveField_createData(ff_field, 1, ysize, RaveDataType_DOUBLE) ||
      !RaveField_createData(ff_dev_field, 1, ysize, RaveDataType_DOUBLE) ||
      !RaveField_createData(dd_field, 1, ysize, RaveDataType_DOUBLE) ||
      !RaveField_createData(dbzh_field, 1, ysize, RaveDataType_DOUBLE) ||
      !RaveField_createData(dbzh_dev_field, 1, ysize, RaveDataType_DOUBLE) ||
      !RaveField_createData(nz_field, 1, ysize, RaveDataType_INT) ||
      !RaveField_createData(nv_field, 1, ysize, RaveDataType_INT)) {
    RAVE_ERROR0("Failed to allocate arrays for the resulting vp fields");
    goto done;
  }

	polnav = RAVE_OBJECT_NEW (&PolarNavigator_TYPE);
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

    // loop over scans
    for (is = 0; is < nscans; is++) {
      scan = PolarVolume_getScan(inobj, is);
      nbins = PolarScan_getNbins(scan);
      nrays = PolarScan_getNrays(scan);
      rscale = PolarScan_getRscale(scan);
      elangle = PolarScan_getElangle(scan);

      // radial wind scans
      if (PolarScan_hasParameter(scan, "VRAD")) {
        vrad = PolarScan_getParameter(scan, "VRAD");
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
            PolarNavigator_reToDh (polnav, (ib+0.5)*rscale, elangle, &d, &h);
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

    if ((nv < NMIN) || (nz < NMIN)) {
      RaveField_setValue(ff_field, 0, yindex, -9999.0);
      RaveField_setValue(ff_dev_field, 0, yindex, -9999.0);
      RaveField_setValue(dd_field, 0, yindex, -9999.0);
      RaveField_setValue(dbzh_field, 0, yindex, -9999.0);
      RaveField_setValue(dbzh_dev_field, 0, yindex, -9999.0);
      RaveField_setValue(nz_field, 0, yindex, 0);
      RaveField_setValue(nv_field, 0, yindex, 0);
    } else {
      RaveField_setValue(ff_field, 0, yindex, vvel);
      RaveField_setValue(ff_dev_field, 0, yindex, vstd);
      RaveField_setValue(dd_field, 0, yindex, vdir);
      RaveField_setValue(dbzh_field, 0, yindex, zmean);
      RaveField_setValue(dbzh_dev_field, 0, yindex, zstd);
      RaveField_setValue(nz_field, 0, yindex, nz);
      RaveField_setValue(nv_field, 0, yindex, nv);
    }

/*    if ((nv < NMIN) || (nz < NMIN))
      printf("%6d %6d %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f \n", iz+self->dz/2, 0, -9999., -9999., -9999., -9999., -9999., -9999.);
    else
      printf("%6d %6d %8.2f %8.2f %8.2f %8.2f %8.2f %8.2f \n", iz+self->dz/2, nv, vvel, vstd, vdir, -9999., zmean, zstd);*/

    RAVE_FREE(A);
    RAVE_FREE(b);
    RAVE_FREE(v);
    RAVE_FREE(z);
    RAVE_FREE(az);

    yindex++; /* Next interval*/
  }

  result = RAVE_OBJECT_NEW(&VerticalProfile_TYPE);
  if (result != NULL) {
    if (!VerticalProfile_setFF(result, ff_field) ||
        !VerticalProfile_setFFDev(result, ff_dev_field) ||
        !VerticalProfile_setDD(result, dd_field) ||
        !VerticalProfile_setDBZ(result, dbzh_field) ||
        !VerticalProfile_setDBZDev(result, dbzh_dev_field)) {
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
  VerticalProfile_setMinheight(result, self->dz / 2);
  VerticalProfile_setMaxheight(result, self->hmax);
  VerticalProfile_setDate(result, PolarVolume_getDate(inobj));
  VerticalProfile_setTime(result, PolarVolume_getTime(inobj));

  WrwpInternal_findAndAddAttribute(result, inobj, "how/highprf");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/lowprf");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/pulsewidth");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/wavelength");

  WrwpInternal_findAndAddAttribute(result, inobj, "how/RXbandwidth");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/RXloss");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/TXloss");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/antgain");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/azmethod");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/binmethod");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/malfunc");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/nomTXpower");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/radar_msg");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/radconstH");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/radomeloss");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/rpm");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/software");
  WrwpInternal_findAndAddAttribute(result, inobj, "how/system");

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

  return result;
}

/*@} End of Interface functions */

RaveCoreObjectType Wrwp_TYPE = {
    "Wrwp",
    sizeof(Wrwp_t),
    Wrwp_constructor,
    Wrwp_destructor
};

