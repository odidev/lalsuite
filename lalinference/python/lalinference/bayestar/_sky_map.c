/*
 * Copyright (C) 2013-2016  Leo Singer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with with program; see the file COPYING. If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 */

#include "config.h"
#include <Python.h>
#include <numpy/arrayobject.h>
#include <chealpix.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_nan.h>
#include <lal/bayestar_sky_map.h>
#include <assert.h>
#include "six.h"


static void capsule_free(PyObject *self)
{
    free(PyCapsule_GetPointer(self, NULL));
}


#define INPUT_LIST_OF_ARRAYS(NAME, NPYTYPE, DEPTH, CHECK) \
{ \
    const Py_ssize_t n = PySequence_Length(NAME##_obj); \
    if (n < 0) \
        goto fail; \
    else if (n != nifos) { \
        PyErr_SetString(PyExc_ValueError, #NAME \
        " appears to be the wrong length for the number of detectors"); \
        goto fail; \
    } \
    for (unsigned int iifo = 0; iifo < nifos; iifo ++) \
    { \
        PyObject *obj = PySequence_ITEM(NAME##_obj, iifo); \
        if (!obj) goto fail; \
        PyArrayObject *npy = (PyArrayObject *) \
            PyArray_ContiguousFromAny(obj, NPYTYPE, DEPTH, DEPTH); \
        Py_XDECREF(obj); \
        if (!npy) goto fail; \
        NAME##_npy[iifo] = npy; \
        { CHECK } \
        NAME[iifo] = PyArray_DATA(npy); \
    } \
}

#define FREE_INPUT_LIST_OF_ARRAYS(NAME) \
{ \
    for (unsigned int iifo = 0; iifo < nifos; iifo ++) \
        Py_XDECREF(NAME##_npy[iifo]); \
}

#define INPUT_VECTOR_NIFOS(CTYPE, NAME, NPYTYPE) \
    NAME##_npy = (PyArrayObject *) \
        PyArray_ContiguousFromAny(NAME##_obj, NPYTYPE, 1, 1); \
    if (!NAME##_npy) goto fail; \
    if (PyArray_DIM(NAME##_npy, 0) != nifos) \
    { \
        PyErr_SetString(PyExc_ValueError, #NAME \
            " appears to be the wrong length for the number of detectors"); \
        goto fail; \
    } \
    const CTYPE *NAME = PyArray_DATA(NAME##_npy);

#define INPUT_VECTOR_DOUBLE_NIFOS(NAME) \
    INPUT_VECTOR_NIFOS(double, NAME, NPY_DOUBLE)


static PyObject *sky_map_toa_phoa_snr(
    PyObject *NPY_UNUSED(module), PyObject *args, PyObject *kwargs)
{
    /* Input arguments */
    long nside = -1;
    long npix;
    double min_distance;
    double max_distance;
    int prior_distance_power;
    double gmst;
    unsigned int nifos;
    unsigned long nsamples = 0;
    double sample_rate;
    PyObject *epochs_obj;
    PyObject *snrs_obj;
    PyObject *responses_obj;
    PyObject *locations_obj;
    PyObject *horizons_obj;

    /* Names of arguments */
    static const char *keywords[] = {"min_distance", "max_distance",
        "prior_distance_power", "gmst", "sample_rate", "epochs", "snrs",
        "responses", "locations", "horizons", "nside", NULL};

    /* Parse arguments */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ddiddOOOOO|l",
        keywords, &min_distance, &max_distance, &prior_distance_power, &gmst,
        &sample_rate, &epochs_obj, &snrs_obj, &responses_obj, &locations_obj,
        &horizons_obj, &nside)) return NULL;

    /* Determine HEALPix resolution, if specified */
    if (nside == -1)
    {
        npix = -1;
    } else {
        npix = nside2npix(nside);
        if (npix == -1)
        {
            PyErr_SetString(PyExc_ValueError, "nside must be a power of 2");
            return NULL;
        }
    }

    /* Determine number of detectors */
    {
        Py_ssize_t n = PySequence_Length(epochs_obj);
        if (n < 0) return NULL;
        nifos = n;
    }

    /* Return value */
    PyObject *out = NULL;

    /* Numpy array objects */
    PyArrayObject *epochs_npy = NULL, *snrs_npy[nifos], *responses_npy[nifos],
        *locations_npy[nifos], *horizons_npy = NULL;
    memset(snrs_npy, 0, sizeof(snrs_npy));
    memset(responses_npy, 0, sizeof(responses_npy));
    memset(locations_npy, 0, sizeof(locations_npy));

    /* Arrays of pointers for inputs with multiple dimensions */
    const float complex *snrs[nifos];
    const float (*responses[nifos])[3];
    const double *locations[nifos];

    /* Gather C-aligned arrays from Numpy types */
    INPUT_VECTOR_DOUBLE_NIFOS(epochs)
    INPUT_LIST_OF_ARRAYS(snrs, NPY_CFLOAT, 1,
        npy_intp dim = PyArray_DIM(npy, 0);
        if (iifo == 0)
            nsamples = dim;
        else if ((unsigned long)dim != nsamples)
        {
            PyErr_SetString(PyExc_ValueError,
                "expected elements of snrs to be vectors of the same length");
            goto fail;
        }
    )
    INPUT_LIST_OF_ARRAYS(responses, NPY_FLOAT, 2,
        if (PyArray_DIM(npy, 0) != 3 || PyArray_DIM(npy, 1) != 3)
        {
            PyErr_SetString(PyExc_ValueError,
                "expected elements of responses to be 3x3 arrays");
            goto fail;
        }
    )
    INPUT_LIST_OF_ARRAYS(locations, NPY_DOUBLE, 1,
        if (PyArray_DIM(npy, 0) != 3)
        {
            PyErr_SetString(PyExc_ValueError,
                "expected elements of locations to be vectors of length 3");
            goto fail;
        }
    )
    INPUT_VECTOR_DOUBLE_NIFOS(horizons)

    /* Call function */
    gsl_error_handler_t *old_handler = gsl_set_error_handler_off();
    double (*ret)[4] = bayestar_sky_map_toa_phoa_snr(&npix, min_distance,
        max_distance, prior_distance_power, gmst, nifos, nsamples, sample_rate,
        epochs, snrs, responses, locations, horizons);
    gsl_set_error_handler(old_handler);

    if (!ret)
        goto fail;

    /* Prepare output object */
    PyObject *capsule = PyCapsule_New(ret, NULL, capsule_free);
    if (!capsule)
        goto fail;

    npy_intp dims[] = {npix, 4};
    out = PyArray_SimpleNewFromData(2, dims, NPY_DOUBLE, ret);
    if (!out)
    {
        Py_DECREF(capsule);
        goto fail;
    }

    if (PyArray_SetBaseObject(out, capsule))
    {
        Py_DECREF(out);
        out = NULL;
        goto fail;
    }

fail: /* Cleanup */
    Py_XDECREF(epochs_npy);
    FREE_INPUT_LIST_OF_ARRAYS(snrs)
    FREE_INPUT_LIST_OF_ARRAYS(responses)
    FREE_INPUT_LIST_OF_ARRAYS(locations)
    Py_XDECREF(horizons_npy);
    return out;
};


static PyObject *log_likelihood_toa_phoa_snr(
    PyObject *NPY_UNUSED(module), PyObject *args, PyObject *kwargs)
{
    /* Input arguments */
    double ra;
    double sin_dec;
    double distance;
    double u;
    double twopsi;
    double t;
    double gmst;
    unsigned int nifos;
    unsigned long nsamples = 0;
    double sample_rate;
    PyObject *epochs_obj;
    PyObject *snrs_obj;
    PyObject *responses_obj;
    PyObject *locations_obj;
    PyObject *horizons_obj;

    /* Names of arguments */
    static const char *keywords[] = {"params", "gmst", "sample_rate", "epochs",
        "snrs", "responses", "locations", "horizons", NULL};

    /* Parse arguments */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "(dddddd)ddOOOOO",
        keywords, &ra, &sin_dec, &distance, &u, &twopsi, &t, &gmst,
        &sample_rate, &epochs_obj, &snrs_obj, &responses_obj, &locations_obj,
        &horizons_obj)) return NULL;

    /* Determine number of detectors */
    {
        Py_ssize_t n = PySequence_Length(epochs_obj);
        if (n < 0) return NULL;
        nifos = n;
    }

    /* Return value */
    PyObject *out = NULL;

    /* Numpy array objects */
    PyArrayObject *epochs_npy = NULL, *snrs_npy[nifos], *responses_npy[nifos],
        *locations_npy[nifos], *horizons_npy = NULL;
    memset(snrs_npy, 0, sizeof(snrs_npy));
    memset(responses_npy, 0, sizeof(responses_npy));
    memset(locations_npy, 0, sizeof(locations_npy));

    /* Arrays of pointers for inputs with multiple dimensions */
    const float complex *snrs[nifos];
    const float (*responses[nifos])[3];
    const double *locations[nifos];

    /* Gather C-aligned arrays from Numpy types */
    INPUT_VECTOR_DOUBLE_NIFOS(epochs)
    INPUT_LIST_OF_ARRAYS(snrs, NPY_CFLOAT, 1,
        npy_intp dim = PyArray_DIM(npy, 0);
        if (iifo == 0)
            nsamples = dim;
        else if ((unsigned long)dim != nsamples)
        {
            PyErr_SetString(PyExc_ValueError,
                "expected elements of snrs to be vectors of the same length");
            goto fail;
        }
    )
    INPUT_LIST_OF_ARRAYS(responses, NPY_FLOAT, 2,
        if (PyArray_DIM(npy, 0) != 3 || PyArray_DIM(npy, 1) != 3)
        {
            PyErr_SetString(PyExc_ValueError,
                "expected elements of responses to be 3x3 arrays");
            goto fail;
        }
    )
    INPUT_LIST_OF_ARRAYS(locations, NPY_DOUBLE, 1,
        if (PyArray_DIM(npy, 0) != 3)
        {
            PyErr_SetString(PyExc_ValueError,
                "expected elements of locations to be vectors of length 3");
            goto fail;
        }
    )
    INPUT_VECTOR_DOUBLE_NIFOS(horizons)

    /* Call function */
    gsl_error_handler_t *old_handler = gsl_set_error_handler_off();
    const double ret = bayestar_log_likelihood_toa_phoa_snr(ra, sin_dec,
        distance, u, twopsi, t, gmst, nifos, nsamples, sample_rate, epochs,
        snrs, responses, locations, horizons);
    gsl_set_error_handler(old_handler);

    /* Prepare output object */
    out = PyFloat_FromDouble(ret);

fail: /* Cleanup */
    Py_XDECREF(epochs_npy);
    FREE_INPUT_LIST_OF_ARRAYS(snrs)
    FREE_INPUT_LIST_OF_ARRAYS(responses)
    FREE_INPUT_LIST_OF_ARRAYS(locations)
    Py_XDECREF(horizons_npy);
    return out;
};


static PyObject *test(
    PyObject *NPY_UNUSED(module), PyObject *NPY_UNUSED(arg))
{
    gsl_error_handler_t *old_handler = gsl_set_error_handler_off();
    int ret = bayestar_test();
    gsl_set_error_handler(old_handler);
    return PyLong_FromLong(ret);
}


static PyMethodDef methods[] = {
    {"toa_phoa_snr", (PyCFunction)sky_map_toa_phoa_snr,
        METH_VARARGS | METH_KEYWORDS, "fill me in"},
    {"log_likelihood_toa_phoa_snr", (PyCFunction)log_likelihood_toa_phoa_snr,
        METH_VARARGS | METH_KEYWORDS, "fill me in"},
    {"test", (PyCFunction)test,
        METH_NOARGS, "fill me in"},
    {NULL, NULL, 0, NULL}
};


static PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "_sky_map", NULL, -1, methods
};


PyMODINIT_FUNC PyInit__sky_map(void); /* Silence -Wmissing-prototypes */
PyMODINIT_FUNC PyInit__sky_map(void)
{
    PyObject *module = NULL;

    import_array();

    module = PyModule_Create(&moduledef);
    if (!module)
        goto done;

done:
    return module;
}


SIX_COMPAT_MODULE(_sky_map)
