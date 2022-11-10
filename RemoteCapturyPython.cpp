#include <Python.h>
#include "RemoteCaptury.h"
#include "iostream"
/* #define NO_IMPORT_ARRAY
#define PY_ARRAY_UNIQUE_SYMBOL PyArrayHandle
#include "numpy/arrayobject.h" */

PyObject* pyCallBack = NULL;

static void capturyImageCallback(const CapturyImage* image)
{
	std::cout << "CapturyImageCallback: start" << std::endl;
	if (pyCallBack!=NULL) {
		// numpy image from CapturyImage data
		/* npy_intp dims[3] = { image->height, image->width, 3 };
		PyObject* pyImage = PyArray_SimpleNewFromData(3, dims, NPY_UINT8, image->data);
		PyObject* args = PyTuple_New(1);
		PyTuple_SetItem(args, 0, pyImage); */
		int height = image->height;
		PyObject* args = PyTuple_New(1);
		PyTuple_SetItem(args, 0, PyLong_FromLong(height));
		PyObject* result = PyObject_CallObject(pyCallBack, args);
		Py_DECREF(args);
	}
	else {
		std::cout << "capturyImageCallback: pyCallBack is NULL" << std::endl;
	}
}

static PyObject* connect(PyObject *self, PyObject *args, PyObject* kwargs)
{
	static char *kwlist[] = { (char*)"host", (char*)"port", NULL};
	const char* host;
	int port = 2101;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|i:connect", kwlist, &host, &port)) {
		PyErr_SetString(PyExc_TypeError, "connect: invalid arguments. it should be connect(host:str, port:int=2101)->bool");
		return NULL;
	}
	std::cout << "connect: connecting to captury at " << host << ":" << port << std::endl;

	if (Captury_connect(host, port) == 1) {
		std::cout << "connect: connected to." << std::endl;
		Py_RETURN_TRUE; // success
	}
	Py_RETURN_FALSE;
}

static PyObject* startStreamingImages(PyObject *self, PyObject *args, PyObject* kwargs)
{
	static char *kwlist[] = {(char *)"what", (char*)"cameraNumber", NULL};
	int cameraNumber;
	int what;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii:startStreamingImages", kwlist, &what, &cameraNumber)) {
		PyErr_SetString(PyExc_TypeError, "startStreamingImages expects an integer arguments. startStreamingImages(what: int, cameraNumber:int)->bool");
		Py_RETURN_FALSE;
	}

	const CapturyCamera* camera;
	int numCameras = Captury_getCameras(&camera);
	std::cout << numCameras << " cameras found." << std::endl;

	if (numCameras == 0)
		Py_RETURN_FALSE;

	if(cameraNumber >= numCameras) {
		PyErr_SetString(PyExc_TypeError, "startStreamingImages: cameraNumber is out of range. should be 0 to totalCameras-1");
		Py_RETURN_FALSE;
	}

	if (Captury_startStreamingImages(what, camera[cameraNumber].id)) {
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
		PyErr_SetString(PyExc_TypeError, "setNewImageCallback expects a function as argument. setNewImageCallback(callback: function)->bool");
		Py_RETURN_FALSE;
	}

	if (PyCallable_Check(callback)) {
		std::cout << "callback is callable" << std::endl;
		Py_INCREF(callback);
		if (pyCallBack == NULL) {
			pyCallBack = callback;
			Py_RETURN_TRUE;
		}
		else {
			std::cout << "deleting the old callback" << std::endl;
			Py_DECREF(pyCallBack);
			pyCallBack = callback;
			Py_RETURN_TRUE;
		}
	}
	Py_RETURN_FALSE;
}

static PyObject* startStreaming(PyObject *self, PyObject *args, PyObject* kwargs)
{
	static char *kwlist[] = {(char *)"what", NULL};
	int cameraNumber;
	int what;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i:startStreaming", kwlist, &what, &cameraNumber)) {
		PyErr_SetString(PyExc_TypeError, "startStreaming expects an integer arguments. startStreaming(what: int)->bool");
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
	return PyModule_Create(&rcModule);
}
