#include <Python.h>
#include "RemoteCaptury.h"

static PyObject* connect(PyObject *self, PyObject *args)
{
	const char* host;
	int port = 2101;
	if (PyArg_ParseTuple(args, "s|i:connect", &host, &port)) {
		if (Captury_connect(host, port))
			Py_RETURN_TRUE;
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

	// todo : check numberNumber is in range of available cameras.
	// the method captury_startStreamingImages accepts int cameraNumber, that number is what ? 0 to totalCameras-1 ?

	if (Captury_startStreamingImages(what, cameraNumber))
		Py_RETURN_TRUE;
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

	// todo : check numberNumber is in range of available cameras.

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
	{"connect", connect, METH_VARARGS, "Connect to host[, port=2101]"},
	// {"getActors", getActors, METH_VARARGS, "Returns an array of actors"},
	{"startStreaming", (PyCFunction)startStreaming, METH_VARARGS | METH_KEYWORDS, "starts streaming "},
	{"startStreamingImages", (PyCFunction)startStreamingImages, METH_VARARGS | METH_KEYWORDS, "Starts streaming data and images"},
	{"stopStreaming", stopStreaming, METH_NOARGS, "Stops streaming"},
	{"synchronizeTime", synchronizeTime, METH_NOARGS, "Stops streaming"},
	{"getTime", getTime, METH_NOARGS, "Stops streaming"},
	{"getTimeOffset", getTimeOffset, METH_NOARGS, "Stops streaming"},
	{"snapActor", snapActor, METH_NOARGS, "Tries to track a person at the given location."},
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
