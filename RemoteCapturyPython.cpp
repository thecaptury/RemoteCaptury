#define PY_SSIZE_T_CLEAN
#include "numpy/arrayobject.h"
#include <Python.h>
#include "RemoteCaptury.h"

static PyObject* pythonCallBack; // global variable to store the python callback function

static void capturyImageCallback(const CapturyImage* image)
{

	// call pythonCallBack with thread safe
	PyGILState_STATE gstate;
	gstate = PyGILState_Ensure();
	assert(PyArray_API);
	if (pythonCallBack!=NULL) {

		PyObject* args = PyTuple_New(3);
		printf("capturyImageCallback: crating a 1d numpy array\n");
		// create a numpy array from of 1 dimension with shape : image->width * image->height * 3
		npy_intp dims[1] = { image->width * image->height * 3 };
		PyObject* array = PyArray_SimpleNewFromData(1, dims, NPY_UINT8, image->data);

		// PyObject* array = PyArray_ZEROS(1, dims, NPY_UINT8, 0);
		// method of memcpy from image->data to array
		// memcpy(PyArray_DATA((PyArrayObject*)array), image->data, image->width * image->height * 3);

		printf("capturyImageCallback: filling the tuple\n");
		PyTuple_SetItem(args, 0, array);
		PyTuple_SetItem(args, 1, PyLong_FromLong(image->width));
		PyTuple_SetItem(args, 2, PyLong_FromLong(image->height));

		PyObject* result = PyObject_CallObject(pythonCallBack, args);
	}
	else {
		printf("pythonCallBack is NULL. Please set a callback first");
	}
	PyGILState_Release(gstate);
}

static PyObject* connect(PyObject *self, PyObject *args, PyObject* kwargs)
{
	static char *kwlist[] = { (char*)"host", (char*)"port", NULL};
	const char* host;
	int port = 2101;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|i:connect", kwlist, &host, &port)) {
		return NULL;
	}

	if (Captury_connect(host, port) == 1) {
		Py_RETURN_TRUE; // success
	}
	Py_RETURN_FALSE;
}

static PyObject* startStreamingImages(PyObject *self, PyObject *args, PyObject* kwargs)
{
	static char *kwlist[] = {(char*)"cameraNumber", NULL};
	int cameraNumber;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i:startStreamingImages", kwlist, &cameraNumber)) {
		Py_RETURN_FALSE;
	}

	const CapturyCamera* camera;
	int numCameras = Captury_getCameras(&camera);
	printf("startStreamingImages: found numCameras: %d\n", numCameras);

	if (numCameras == 0)
		Py_RETURN_FALSE;

	if(cameraNumber >= numCameras) {
		PyErr_SetString(PyExc_TypeError, "startStreamingImages: cameraNumber is out of range. should be 0 to totalCameras-1");
		Py_RETURN_FALSE;
	}

	if (Captury_startStreamingImages(CAPTURY_STREAM_IMAGES, camera[cameraNumber].id)) {
		Captury_registerImageStreamingCallback(&capturyImageCallback);
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyDoc_STRVAR(setNewImageCallback_doc_, R"(
	This method allows to register a python callback that will be called when a new image is available.
	:param callback: a python function that will be called when a new image is available.
	:type callback: function
	)");
static PyObject* setNewImageCallback(PyObject *self, PyObject *args, PyObject* kwargs)
{
	static char *kwlist[] = {(char *)"callback", NULL};
	PyObject* callback;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:setNewImageCallback", kwlist, &callback)) {
		return NULL;
	}

	// check for callable
	if (PyCallable_Check(callback)) {
		Py_INCREF(callback);

		// if a callback was already set, decref it
		if(pythonCallBack != NULL) {
			Py_DECREF(pythonCallBack);
		}

		// set the new callback. make it thread safe to change it.
		PyGILState_STATE gstate;
		gstate = PyGILState_Ensure();
		pythonCallBack = callback;
		PyGILState_Release(gstate);
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* startStreaming(PyObject *self, PyObject *args, PyObject* kwargs)
{
	static char *kwlist[] = {(char *)"what", NULL};
	int cameraNumber;
	int what;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i:startStreaming", kwlist, &what, &cameraNumber)) {
		Py_RETURN_FALSE;
	}

	if (Captury_startStreaming(what))
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

static PyObject* stopStreaming(PyObject *self, PyObject *args)
{
	if (Captury_stopStreaming())
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

static PyObject* synchronizeTime(PyObject *self, PyObject *args)
{
	return PyLong_FromLong(Captury_synchronizeTime());
}

static PyObject* getTime(PyObject *self, PyObject *args)
{
	return PyLong_FromLong(Captury_getTime());
}

static PyObject* getTimeOffset(PyObject *self, PyObject *args)
{
	return PyLong_FromLong(Captury_getTimeOffset());
}

static PyObject* snapActor(PyObject *self, PyObject *args)
{
	float x, y;
	float heading = 370;
	if (PyArg_ParseTuple(args, "ff|f:snapActor", &x, &y, &heading)) {
		if (Captury_snapActor(x, y, heading))
			Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* startRecording(PyObject *self, PyObject *args)
{
	if (Captury_startRecording())
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

static PyObject* stopRecording(PyObject *self, PyObject *args)
{
	if (Captury_stopRecording())
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

static PyObject* setShotName(PyObject *self, PyObject *args)
{
	const char* name;
	if (PyArg_ParseTuple(args, "s:setShotName", &name)) {
		if (Captury_setShotName(name))
			Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyMethodDef pythonVisibleMethods[] = {
	{"connect", (PyCFunction)connect, METH_VARARGS | METH_KEYWORDS, "Connect to host[, port=2101]"},
	// {"getActors", getActors, METH_VARARGS, "Returns an array of actors"},
	{"startStreaming", (PyCFunction)startStreaming, METH_VARARGS | METH_KEYWORDS, "starts streaming "},
	{"startStreamingImages", (PyCFunction)startStreamingImages, METH_VARARGS | METH_KEYWORDS, "Starts streaming data and images"},
	{"setNewImageCallback", (PyCFunction)setNewImageCallback, METH_VARARGS | METH_KEYWORDS, setNewImageCallback_doc_},
	{"stopStreaming", stopStreaming, METH_NOARGS, "Stops streaming"},
	{"synchronizeTime", synchronizeTime, METH_NOARGS, "Stops streaming"},
	{"getTime", getTime, METH_NOARGS, "Stops streaming"},
	{"getTimeOffset", getTimeOffset, METH_NOARGS, "Stops streaming"},
	{"snapActor", snapActor, METH_VARARGS, "Tries to track a person at the given location."},
	{"setShotName", setShotName, METH_VARARGS, "Select the shot with the name (or create new one if it doesn't exist)."},
	{"startRecording", startRecording, METH_NOARGS, "Start recording."},
	{"stopRecording", stopRecording, METH_NOARGS, "Stop recording."},
	{NULL, NULL, 0, NULL}
};

static PyModuleDef rcModule = {
	PyModuleDef_HEAD_INIT, "remotecaptury", "communicate with CapturyLive", -1, pythonVisibleMethods
};

PyMODINIT_FUNC
PyInit_remotecaptury(void)
{
	import_array();
	if(PyErr_Occurred()) {
		Py_FatalError("can't initialize module remotecaptury");
	}
	return PyModule_Create(&rcModule);
}
