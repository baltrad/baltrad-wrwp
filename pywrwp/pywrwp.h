/* --------------------------------------------------------------------
Copyright (C) 2013 Swedish Meteorological and Hydrological Institute, SMHI,

This file is part of baltrad-wrwp.

baltrad-wrwp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

baltrad-wrwp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with baltrad-wrwp.  If not, see <http://www.gnu.org/licenses/>.
------------------------------------------------------------------------*/
/**
 * Python version of the WRWP API.
 * @file
 * @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
 * @date 2013-09-18
 */
#ifndef PYWRWP_H
#define PYWRWP_H
#include <Python.h>
#include "wrwp.h"

/**
 * The wrwp generator
 */
typedef struct {
  PyObject_HEAD /*Always has to be on top*/
  Wrwp_t* wrwp;  /**< the c-api wrwp generator */
} PyWrwp;

#define PyWrwp_Type_NUM 0                     /**< index for Type */

#define PyWrwp_GetNative_NUM 1                /**< index for GetNative fp */
#define PyWrwp_GetNative_RETURN Wrwp_t*       /**< Return type for GetNative */
#define PyWrwp_GetNative_PROTO (PyWrwp*)      /**< Argument prototype for GetNative */

#define PyWrwp_New_NUM 2                      /**< index for New fp */
#define PyWrwp_New_RETURN PyWrwp*             /**< Return type for New */
#define PyWrwp_New_PROTO (Wrwp_t*)            /**< Argument prototype for New */

#define PyWrwp_API_pointers 3                 /**< total number of C API pointers */

#define PyWrwp_CAPSULE_NAME "_wrwp._C_API"

#ifdef PYWRWP_MODULE
/** declared in pywrwp module */
extern PyTypeObject PyWrwp_Type;

/** checks if the object is a PyWrwp type or not */
#define PyWrwp_Check(op) ((op)->ob_type == &PyWrwp_Type)

/** Prototype for PyWrwp modules GetNative function */
static PyWrwp_GetNative_RETURN PyWrwp_GetNative PyWrwp_GetNative_PROTO;

/** Prototype for PyWrwp modules New function */
static PyWrwp_New_RETURN PyWrwp_New PyWrwp_New_PROTO;

#else
/** static pointer containing the pointers to function pointers and other definitions */
static void **PyWrwp_API;

/**
 * Returns a pointer to the internal polar scan, remember to release the reference
 * when done with the object. (RAVE_OBJECT_RELEASE).
 */
#define PyWrwp_GetNative \
  (*(PyWrwp_GetNative_RETURN (*)PyWrwp_GetNative_PROTO) PyWrwp_API[PyWrwp_GetNative_NUM])

/**
 * Creates a new polar scan instance. Release this object with Py_DECREF.
 * @param[in] wrwp - the Wrwp_t intance.
 * @returns the PyWrwp instance.
 */
#define PyWrwp_New \
  (*(PyWrwp_New_RETURN (*)PyWrwp_New_PROTO) PyWrwp_API[PyWrwp_New_NUM])

/**
 * Checks if the object is a python wrwp generator.
 */
#define PyWrwp_Check(op) \
    (Py_TYPE(op) == &PyWrwp_Type)


#define PyWrwp_Type (*(PyTypeObject*)PyWrwp_API[PyWrwp_Type_NUM])

/**
 * Imports the PyWrwp module (like import _wrwp in python).
 */
#define import_pywrwp() \
    PyWrwp_API = (void **)PyCapsule_Import(PyWrwp_CAPSULE_NAME, 1);

#endif


#endif /* PYWRWP_H */
