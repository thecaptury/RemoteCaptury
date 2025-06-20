#include <Python.h>
#include "RemoteCaptury.h"

static RemoteCaptury* rc = nullptr;

static PyObject* connect(PyObject *self, PyObject *args)
{
	const char* host;
	int port = 2101;
	if (PyArg_ParseTuple(args, "s|i:connect", &host, &port)) {
		if (rc == nullptr)
			rc = Captury_create();
		if (Captury_connect(rc, host, port))
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

	if (Captury_startStreamingImages(rc, what, cameraNumber))
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

	// TODO: check cameraNumber is in range of available cameras.

	if (Captury_startStreaming(rc, what))
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

static PyObject* stopStreaming(PyObject *self, PyObject *args)
{
	if (Captury_stopStreaming(rc))
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

static PyObject* synchronizeTime(PyObject *self, PyObject *args)
{
	return PyLong_FromLong(Captury_synchronizeTime(rc));
}

static PyObject* startSyncLoop(PyObject *self, PyObject *args)
{
	Captury_startTimeSynchronizationLoop(rc);
	Py_RETURN_NONE;
}
static PyObject* getTime(PyObject *self, PyObject *args)
{
	return PyLong_FromLong(Captury_getTime(rc));
}

static PyObject* getTimeOffset(PyObject *self, PyObject *args)
{
	return PyLong_FromLong(Captury_getTimeOffset(rc));
}

static PyObject* snapActor(PyObject *self, PyObject *args)
{
	float x, y;
	float heading = 370;
	if (PyArg_ParseTuple(args, "ff|f:snapActor", &x, &y, &heading)) {
		if (Captury_snapActor(rc, x, y, heading))
			Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* startRecording(PyObject *self, PyObject *args)
{
	if (Captury_startRecording(rc))
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

static PyObject* stopRecording(PyObject *self, PyObject *args)
{
	if (Captury_stopRecording(rc))
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

static PyObject* setShotName(PyObject *self, PyObject *args)
{
	const char* name;
	if (PyArg_ParseTuple(args, "s:setShotName", &name)) {
		if (Captury_setShotName(rc, name))
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
	{"synchronizeTime", synchronizeTime, METH_NOARGS, "Synchronize time between remote and local machine once"},
	{"startSynchronizationLoop", startSyncLoop, METH_NOARGS, "Continuously synchronize time between remote and local machine once"},
	{"getTime", getTime, METH_NOARGS, "Get time on remote machine"},
	{"getTimeOffset", getTimeOffset, METH_NOARGS, "Get time difference between remote machine and local machine"},
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
