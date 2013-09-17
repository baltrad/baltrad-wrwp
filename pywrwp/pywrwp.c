/* --------------------------------------------------------------------
Copyright (C) 2013 Swedish Meteorological and Hydrological Institute, SMHI

This is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with baltrad-wrwp.  If not, see <http://www.gnu.org/licenses/>.
------------------------------------------------------------------------*/

/**
 * Python module
 * @file
 * @author Anders Henja, SMHI
 * @date 2013-09-17
 */
#include <Python.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <pypolarvolume.h>
#include <pyverticalprofile.h>
#include "wrwp.h"

#include <arrayobject.h>
#include "pyrave_debug.h"
#include "rave_alloc.h"
#include "rave.h"

/**
 * Debug this module
 */
PYRAVE_DEBUG_MODULE("_wrwp");

/**
 * Sets a python exception and goto tag
 */
#define raiseException_gotoTag(tag, type, msg) \
{PyErr_SetString(type, msg); goto tag;}

/**
 * Sets python exception and returns NULL
 */
#define raiseException_returnNULL(type, msg) \
{PyErr_SetString(type, msg); return NULL;}

/**
 * Error object for reporting errors to the python interpreeter
 */
static PyObject* ErrorObject;

/*@{ Weather radar wind profiles */
static PyObject* _pywrwp_wrwp(PyObject* self, PyObject* args)
{
	PyObject* obj = NULL;
	//PyPolarVolume* pyobj = NULL;
	if(!PyArg_ParseTuple(args, "O",&obj)) {
		return NULL;
	}
	if (!PyPolarVolume_Check(obj)) {
	  raiseException_returnNULL(PyExc_AttributeError, "In argument must be a polar volume");
	}
	//pyobj = (PyPolarVolume*)obj;

	return NULL;
}

/*@} End of Weather radar wind profiles */

/// --------------------------------------------------------------------
/// Module setup
/// --------------------------------------------------------------------
/*@{ Module setup */
static PyMethodDef functions[] = {
  {"wrwp", (PyCFunction)_pywrwp_wrwp, 1},
  {NULL,NULL} /*Sentinel*/
};

/**
 * Initializes polar volume.
 */
void init_wrwp(void)
{
  PyObject *module=NULL;

  module = Py_InitModule("_wrwp", functions);
  if (module == NULL) {
    return;
  }
  ErrorObject = PyString_FromString("_wrwp.error");
  if (ErrorObject == NULL || PyDict_SetItemString(PyModule_GetDict(module), "error", ErrorObject) != 0) {
    Py_FatalError("Can't define _wrwp.error");
  }

  import_array(); /*To make sure I get access to Numeric*/
  import_pypolarvolume();
  import_pyverticalprofile();
  PYRAVE_DEBUG_INITIALIZE;
}
/*@} End of Module setup */
