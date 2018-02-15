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
#include "wrwp.h"
#include "rave_debug.h"

/** Main function for deriving weather radar wind and reflectivity profiles
 * @file
 * @author G�nther Haase, SMHI
 * @date 2011-11-29
 */
int main (int argc,char *argv[]) {
	RaveIO_t* raveio = NULL;
	PolarVolume_t* inobj = NULL;
	VerticalProfile_t* result = NULL;
	Wrwp_t* wrwp = NULL;
	int exitcode = 127;

	Rave_initializeDebugger();
	Rave_setDebugLevel(RAVE_DEBUG);

	if (argc<3) {
		printf ("Usage: %s <input ODIM_H5 polar volume> <output ODIM_H5 polar volume> \n",argv[0]);
		exit (1);
	}

	raveio = RaveIO_open(argv[1]);
	if (raveio == NULL) {
	  fprintf(stderr, "Failed to open file = %s\n", argv[1]);
	  goto done;
	}

	wrwp = RAVE_OBJECT_NEW(&Wrwp_TYPE);
	if (wrwp == NULL) {
	  fprintf(stderr, "Failed to create wrwp object\n");
	  goto done;
	}

	/*Opening of HDF5 radar input file.*/
	if (RaveIO_getObjectType(raveio)== Rave_ObjectType_PVOL) {
		inobj = (PolarVolume_t*)RaveIO_getObject(raveio);
	}
	else {
		printf ("Input file is not a polar volume. Giving up ...\n");
		goto done;
	}
	RaveIO_close (raveio);

	result = Wrwp_generate(wrwp, inobj, NULL);
	if (inobj == NULL) {
		printf ("Could not derive wind profile %s, exiting ...\n", argv[1]);
		exit (1);
	}

	RaveIO_setObject(raveio, (RaveCoreObject*)result);

	if (!RaveIO_save(raveio, argv[2]))
	  goto done;

	exitcode = 0;
done:
	RAVE_OBJECT_RELEASE(raveio);
	RAVE_OBJECT_RELEASE(inobj);
	RAVE_OBJECT_RELEASE(result);

	return exitcode;
}
