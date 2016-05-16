#ifndef __LARCV_PYUTILS_H__
#define __LARCV_PYUTILS_H__

struct _object;
typedef _object PyObject;

#ifndef __CLING__
#ifndef __CINT__
#include <Python.h>
#endif
#endif

#include "DataFormat/Image2D.h"
#include "DataFormat/ROI.h"

namespace larcv {
	/// Utility function: call one-time-only numpy module initialization (you don't have to call)
	void SetPyUtil();
	/// larcv::Image2D to numpy array converter
	PyObject* as_ndarray(const Image2D& img);
}

#endif
