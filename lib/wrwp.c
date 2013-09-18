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
 * @file
 * @author Gunther Haase, SMHI
 * @date 2013-02-06
 */

#include "wrwp.h"

PolarVolume_t* wrwp(PolarVolume_t* inobj) {
	PolarVolume_t* result = NULL;
	PolarScan_t* scan = NULL;
	PolarScanParam_t* vrad = NULL;
	PolarScanParam_t* dbz = NULL;
	PolarNavigator_t* polnav = NULL;
	int nrhs = NRHS, lda = LDA, ldb = LDB, info;
	int nscans = 0, nbins = 0, nrays, nv, nz, i, iz, is, ib, ir;
	double rscale, elangle, gain, offset, nodata, undetect, val;
	double d, h;
	double alpha, beta, gamma, vvel, vdir, vstd, zsum, zmean, zstd;

	result = RAVE_OBJECT_COPY (inobj);
	polnav = RAVE_OBJECT_NEW (&PolarNavigator_TYPE);
	nscans = PolarVolume_getNumberOfScans (inobj);

	// loop over atmospheric layers
	for (iz=0; iz<HMAX; iz+=DZ) {

		/* allocate memory and initialize with zeros */
		double *A = RAVE_CALLOC ((size_t)(NOR*NOC), sizeof (double));
		double *b = RAVE_CALLOC ((size_t)(NOR), sizeof (double));
		double *v = RAVE_CALLOC ((size_t)(NOR), sizeof (double));
		double *z = RAVE_CALLOC ((size_t)(NOR), sizeof (double));
		double *az = RAVE_CALLOC ((size_t)(NOR), sizeof (double));

		vdir = -9999.0; /*NAN;*/
		vvel = -9999.0; /*NAN;*/
		vstd = 0.0;
		zsum = 0.0;
		zmean = -9999.0; /*NAN;*/
		zstd = 0.0;
		nv = 0;
		nz = 0;

		// loop over scans
		for (is=0; is<nscans; is++) {
			scan = PolarVolume_getScan (inobj, is);
			nbins = PolarScan_getNbins (scan);
			nrays = PolarScan_getNrays (scan);
			rscale = PolarScan_getRscale (scan);
			elangle = PolarScan_getElangle (scan);

			// radial wind scans
			if (PolarScan_hasParameter (scan, "VRAD")) {
				vrad = PolarScan_getParameter (scan, "VRAD");
				gain = PolarScanParam_getGain (vrad);
				offset = PolarScanParam_getOffset (vrad);
				nodata = PolarScanParam_getNodata (vrad);
				undetect = PolarScanParam_getUndetect (vrad);

				for (ir=0; ir<nrays; ir++) {
					for (ib=0; ib<nbins; ib++) {
						PolarNavigator_reToDh (polnav, (ib+0.5)*rscale, elangle, &d, &h);
						PolarScanParam_getValue (vrad, ib, ir, &val);
						if ((h>=iz) && (h<iz+DZ) && (d>=DMIN) && (d<=DMAX) && (elangle*RAD2DEG>=EMIN) &&
							(val!=nodata) && (val!=undetect) && (abs(offset+gain*val)>=VMIN)) {
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
			}

			// reflectivity scans
			if (PolarScan_hasParameter (scan, "DBZH")) {
				dbz = PolarScan_getParameter (scan, "DBZH");
				gain = PolarScanParam_getGain (dbz);
				offset = PolarScanParam_getOffset (dbz);
				nodata = PolarScanParam_getNodata (dbz);
				undetect = PolarScanParam_getUndetect (dbz);

				for (ir=0; ir<nrays; ir++) {
					for (ib=0; ib<nbins; ib++) {
						PolarNavigator_reToDh (polnav, (ib+0.5)*rscale, elangle, &d, &h);
						PolarScanParam_getValue (dbz, ib, ir, &val);
						if ((h>=iz) && (h<iz+DZ) && (d>=DMIN) && (d<=DMAX) && (elangle*RAD2DEG>=EMIN) &&
							(val!=nodata) && (val!=undetect)) {
							*(z+nz) = dBZ2Z (offset+gain*val);
							zsum = zsum + *(z+nz);
							nz = nz+1;
						}
					}
				}
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
			info = LAPACKE_dgels (LAPACK_ROW_MAJOR, 'N', NOR, NOC, nrhs, A, lda, b, ldb);

			/* parameter of the wind model */
			alpha = sqrt (pow (*(b),2) + pow (*(b+1),2));
			beta = atan2 (*(b+1), *b);
			gamma = *(b+2);

			/* wind velocity */
			vvel = alpha;

	        /* wind direction */
	        vdir = 0;
	        if (alpha < 0) vdir = (PI/2-beta) * RAD2DEG;
	        else if (alpha > 0) vdir = (3*PI/2-beta) * RAD2DEG;
	        if (vdir < 0) vdir = vdir + 360;
	        else if (vdir > 360) vdir = vdir - 360;

	        /* standard deviation of the wind velocity */
	        for (i=0; i<nv; i++) {
	        	vstd = vstd + pow (*(v+i) - (gamma+alpha*sin (*(az+i)+beta)),2);
	        }
	        vstd = sqrt (vstd/(nv-1));
		}

		// reflectivity calculations
		if (nz>1) {

			/* standard deviation of reflectivity */
			for (i=0; i<nz; i++) {
				zstd = zstd + pow (*(z+i) - (zsum/nz),2);
			}
			zmean = Z2dBZ (zsum/nz);
			zstd = sqrt (zstd/(nz-1));
			zstd = Z2dBZ (zstd);
		}

		if ((nv<NMIN) || (nz<NMIN))
			printf("%6d %6d %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f \n", iz+DZ/2, 0, -9999., -9999., -9999., -9999., -9999., -9999.);
		else
			printf("%6d %6d %8.2f %8.2f %8.2f %8.2f %8.2f %8.2f \n", iz+DZ/2, nv, vvel, vstd, vdir, -9999., zmean, zstd);

		RAVE_FREE (A);
		RAVE_FREE (b);
		RAVE_FREE (v);
		RAVE_FREE (z);
		RAVE_FREE (az);

	}
	return result;
}
