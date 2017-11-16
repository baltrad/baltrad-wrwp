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

#define PYWRWP_MODULE /**< include correct part of pywrwp.h */
#include "pywrwp.h"

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

/**
 * Returns the native Wrwp_t instance.
 * @param[in] pywrwp - the python wrwp instance
 * @returns the native wrwp instance.
 */
static Wrwp_t*
PyWrwp_GetNative(PyWrwp* pywrwp)
{
  RAVE_ASSERT((pywrwp != NULL), "pywrwp == NULL");
  return RAVE_OBJECT_COPY(pywrwp->wrwp);
}

/**
 * Creates a python wrwp from a native wrwp or will create an
 * initial native wrwp if p is NULL.
 * @param[in] p - the native wrwp (or NULL)
 * @returns the python wrwp product generator.
 */
static PyWrwp*
PyWrwp_New(Wrwp_t* p)
{
  PyWrwp* result = NULL;
  Wrwp_t* cp = NULL;

  if (p == NULL) {
    cp = RAVE_OBJECT_NEW(&Wrwp_TYPE);
    if (cp == NULL) {
      RAVE_CRITICAL0("Failed to allocate memory for wrwp.");
      raiseException_returnNULL(PyExc_MemoryError, "Failed to allocate memory for wrwp.");
    }
  } else {
    cp = RAVE_OBJECT_COPY(p);
    result = RAVE_OBJECT_GETBINDING(p); // If p already have a binding, then this should only be increfed.
    if (result != NULL) {
      Py_INCREF(result);
    }
  }

  if (result == NULL) {
    result = PyObject_NEW(PyWrwp, &PyWrwp_Type);
    if (result != NULL) {
      PYRAVE_DEBUG_OBJECT_CREATED;
      result->wrwp = RAVE_OBJECT_COPY(cp);
      RAVE_OBJECT_BIND(result->wrwp, result);
    } else {
      RAVE_CRITICAL0("Failed to create PyWrwp instance");
      raiseException_gotoTag(done, PyExc_MemoryError, "Failed to allocate memory for PyWrwp.");
    }
  }

done:
  RAVE_OBJECT_RELEASE(cp);
  return result;
}

/**
 * Deallocates the wrwp generator
 * @param[in] obj the object to deallocate.
 */
static void _pywrwp_dealloc(PyWrwp* obj)
{
  /*Nothing yet*/
  if (obj == NULL) {
    return;
  }
  PYRAVE_DEBUG_OBJECT_DESTROYED;
  RAVE_OBJECT_UNBIND(obj->wrwp, obj);
  RAVE_OBJECT_RELEASE(obj->wrwp);
  PyObject_Del(obj);
}

/**
 * Creates a new instance of the wrwp generator.
 * @param[in] self this instance.
 * @param[in] args arguments for creation (NOT USED).
 * @return the object on success, otherwise NULL
 */
static PyObject* _pywrwp_new(PyObject* self, PyObject* args)
{
  PyWrwp* result = PyWrwp_New(NULL);
  return (PyObject*)result;
}


static PyObject* _pywrwp_generate(PyWrwp* self, PyObject* args)
{
	PyObject* obj = NULL;
	PyVerticalProfile* pyvp = NULL;
	VerticalProfile_t* vp = NULL;
	char* fieldsToGenerate = NULL;

	if(!PyArg_ParseTuple(args, "O|z", &obj, &fieldsToGenerate)) {
		return NULL;
	}

	if (!PyPolarVolume_Check(obj)) {
	  raiseException_returnNULL(PyExc_AttributeError, "In argument must be a polar volume");
	}

	vp = Wrwp_generate(self->wrwp, ((PyPolarVolume*)obj)->pvol, fieldsToGenerate);

	if (vp == NULL) {
	  raiseException_gotoTag(done, PyExc_RuntimeError, "Failed to generate vertical profile");
	}

	pyvp = PyVerticalProfile_New(vp);

done:
  RAVE_OBJECT_RELEASE(vp);
	return (PyObject*)pyvp;
}

/**
 * All methods a wrwp generator can have
 */
static struct PyMethodDef _pywrwp_methods[] =
{
  {"generate", (PyCFunction)_pywrwp_generate, 1},
  {NULL, NULL } /* sentinel */
};

/**
 * Returns the specified attribute in the wrwp generator
 * @param[in] self - the wrwp generator
 */
static PyObject* _pywrwp_getattr(PyWrwp* self, char* name)
{
  PyObject* res = NULL;
  if (strcmp("dz", name) == 0) {
    return PyInt_FromLong(Wrwp_getDZ(self->wrwp));
  } else if (strcmp("hmax", name) == 0) {
    return PyInt_FromLong(Wrwp_getHMAX(self->wrwp));
  } else if (strcmp("dmin", name) == 0) {
    return PyInt_FromLong(Wrwp_getDMIN(self->wrwp));
  } else if (strcmp("dmax", name) == 0) {
    return PyInt_FromLong(Wrwp_getDMAX(self->wrwp));
  } else if (strcmp("emin", name) == 0) {
    return PyFloat_FromDouble(Wrwp_getEMIN(self->wrwp));
  } else if (strcmp("vmin", name) == 0) {
    return PyFloat_FromDouble(Wrwp_getVMIN(self->wrwp));
  }
  res = Py_FindMethod(_pywrwp_methods, (PyObject*) self, name);
  if (res)
    return res;

  PyErr_Clear();
  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}

/**
 * Returns the specified attribute in the wrwp generator
 */
static int _pywrwp_setattr(PyWrwp* self, char* name, PyObject* val)
{
  int result = -1;
  if (name == NULL) {
    goto done;
  }
  if (strcmp("dz", name) == 0) {
    if (PyInt_Check(val)) {
      Wrwp_setDZ(self->wrwp, PyInt_AsLong(val));
    } else {
      raiseException_gotoTag(done, PyExc_TypeError, "dz must be an integer");
    }
  } else if (strcmp("hmax", name) == 0) {
    if (PyInt_Check(val)) {
      Wrwp_setHMAX(self->wrwp, PyInt_AsLong(val));
    } else {
      raiseException_gotoTag(done, PyExc_TypeError, "hmax must be an integer");
    }
  } else if (strcmp("dmin", name) == 0) {
    if (PyInt_Check(val)) {
      Wrwp_setDMIN(self->wrwp, PyInt_AsLong(val));
    } else {
      raiseException_gotoTag(done, PyExc_TypeError, "dmin must be an integer");
    }
  } else if (strcmp("dmax", name) == 0) {
    if (PyInt_Check(val)) {
      Wrwp_setDMAX(self->wrwp, PyInt_AsLong(val));
    } else {
      raiseException_gotoTag(done, PyExc_TypeError, "dmax must be an integer");
    }
  } else if (strcmp("emin", name) == 0) {
    if (PyFloat_Check(val)) {
      Wrwp_setEMIN(self->wrwp, PyFloat_AsDouble(val));
    } else if (PyInt_Check(val)) {
      Wrwp_setEMIN(self->wrwp, (double)PyInt_AsLong(val));
    } else {
      raiseException_gotoTag(done, PyExc_TypeError, "emin must be an integer or a float");
    }
  } else if (strcmp("vmin", name) == 0) {
    if (PyFloat_Check(val)) {
      Wrwp_setVMIN(self->wrwp, PyFloat_AsDouble(val));
    } else if (PyInt_Check(val)) {
      Wrwp_setVMIN(self->wrwp, (double)PyInt_AsLong(val));
    } else {
      raiseException_gotoTag(done, PyExc_TypeError, "vmin must be an integer or a float");
    }
  }

  result = 0;
done:
  return result;
}
/*@} End of Weather radar wind profiles */

/*@{ Type definitions */
PyTypeObject PyWrwp_Type =
{
  PyObject_HEAD_INIT(NULL)0, /*ob_size*/
  "WrwpCore", /*tp_name*/
  sizeof(PyWrwp), /*tp_size*/
  0, /*tp_itemsize*/
  /* methods */
  (destructor)_pywrwp_dealloc, /*tp_dealloc*/
  0, /*tp_print*/
  (getattrfunc)_pywrwp_getattr, /*tp_getattr*/
  (setattrfunc)_pywrwp_setattr, /*tp_setattr*/
  0, /*tp_compare*/
  0, /*tp_repr*/
  0, /*tp_as_number */
  0,
  0, /*tp_as_mapping */
  0 /*tp_hash*/
};

/*@} End of Type definitions */

/// --------------------------------------------------------------------
/// Module setup
/// --------------------------------------------------------------------
/*@{ Module setup */
static PyMethodDef functions[] = {
  {"new", (PyCFunction)_pywrwp_new, 1},
  {NULL,NULL} /*Sentinel*/
};

PyMODINIT_FUNC
init_wrwp(void)
{
  PyObject *module=NULL,*dictionary=NULL;
  static void *PyWrwp_API[PyWrwp_API_pointers];
  PyObject *c_api_object = NULL;
  PyWrwp_Type.ob_type = &PyType_Type;

  module = Py_InitModule("_wrwp", functions);
  if (module == NULL) {
    return;
  }
  PyWrwp_API[PyWrwp_Type_NUM] = (void*)&PyWrwp_Type;
  PyWrwp_API[PyWrwp_GetNative_NUM] = (void *)PyWrwp_GetNative;
  PyWrwp_API[PyWrwp_New_NUM] = (void*)PyWrwp_New;

  c_api_object = PyCObject_FromVoidPtr((void *)PyWrwp_API, NULL);

  if (c_api_object != NULL) {
    PyModule_AddObject(module, "_C_API", c_api_object);
  }

  dictionary = PyModule_GetDict(module);
  ErrorObject = PyString_FromString("_wrwp.error");
  if (ErrorObject == NULL || PyDict_SetItemString(dictionary, "error", ErrorObject) != 0) {
    Py_FatalError("Can't define _wrwp.error");
  }

  import_array();
  import_pypolarvolume();
  import_pyverticalprofile();
  PYRAVE_DEBUG_INITIALIZE;
}
/*@} End of Module setup */
