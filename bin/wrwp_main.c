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

/** Main function for deriving weather radar wind and reflectivity profiles
 * @file
 * @author Günther Haase, SMHI
 * @date 2011-11-29
 */
int main (int argc,char *argv[]) {
	RaveIO_t* raveio = RaveIO_open (argv[1]);
	PolarVolume_t* inobj = NULL;
	PolarVolume_t* result = NULL;

	if (argc<3) {
		printf ("Usage: %s <input ODIM_H5 polar volume> <output ODIM_H5 polar volume> \n",argv[0]);
		exit (1);
	}

	/*Opening of HDF5 radar input file.*/
	if (RaveIO_getObjectType (raveio) == Rave_ObjectType_PVOL) {
		inobj = (PolarVolume_t*)RaveIO_getObject (raveio);
	}
	else {
		printf ("Input file is not a polar volume. Giving up ...\n");
		RAVE_OBJECT_RELEASE (inobj);
		return 1;
	}
	RaveIO_close (raveio);

	result = wrwp (inobj);
	if (inobj == NULL) {
		printf ("Could not derive wind profile %s, exiting ...\n", argv[1]);
		exit (1);
	}

	RaveIO_setObject (raveio, (RaveCoreObject*)result);
	RaveIO_save (raveio, argv[2]);

	RAVE_OBJECT_RELEASE (raveio);
	RAVE_OBJECT_RELEASE (inobj);
	RAVE_OBJECT_RELEASE (result);

	exit (0);
}
