#include <Python.h>
#include "../../RemoteCaptury.h"
#include <iostream>
#include <vector>

static RemoteCaptury* rc = nullptr;
static PyObject* poseCallback = nullptr;
static PyObject* actorCallback = nullptr;

// Helper to convert CapturyPose to Python Dict
static PyObject* poseToDict(CapturyPose* pose) {
    PyObject* dict = PyDict_New();
    PyDict_SetItemString(dict, "actor", PyLong_FromLong(pose->actor));
    PyDict_SetItemString(dict, "timestamp", PyLong_FromUnsignedLongLong(pose->timestamp));
    
    // Convert transforms to list of dicts
    PyObject* transforms = PyList_New(pose->numTransforms);
    for (int i = 0; i < pose->numTransforms; ++i) {
        PyObject* transform = PyDict_New();
        
        PyObject* translation = PyList_New(3);
        for (int j = 0; j < 3; ++j) {
            PyList_SetItem(translation, j, PyFloat_FromDouble(pose->transforms[i].translation[j]));
        }
        PyDict_SetItemString(transform, "translation", translation);
        
        PyObject* rotation = PyList_New(3);
        for (int j = 0; j < 3; ++j) {
            PyList_SetItem(rotation, j, PyFloat_FromDouble(pose->transforms[i].rotation[j]));
        }
        PyDict_SetItemString(transform, "rotation", rotation);
        
        PyList_SetItem(transforms, i, transform);
    }
    PyDict_SetItemString(dict, "transforms", transforms);
    
    // Add blend shapes if present
    if (pose->numBlendShapes > 0 && pose->blendShapeActivations != nullptr) {
        PyObject* blendShapes = PyList_New(pose->numBlendShapes);
        for (int i = 0; i < pose->numBlendShapes; ++i) {
            PyList_SetItem(blendShapes, i, PyFloat_FromDouble(pose->blendShapeActivations[i]));
        }
        PyDict_SetItemString(dict, "blendShapes", blendShapes);
    }
    
    return dict;
}

// Trampoline for Pose Callback
static void onPose(RemoteCaptury* rc, CapturyActor* actor, CapturyPose* pose, int trackingQuality, void* userArg) {
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    if (poseCallback && PyCallable_Check(poseCallback)) {
        PyObject* pyPose = poseToDict(pose);
        PyObject* args = Py_BuildValue("(iO)", actor->id, pyPose);
        PyObject* result = PyObject_CallObject(poseCallback, args);
        
        if (result == NULL) {
            PyErr_Print(); // Print python exception if any
        } else {
            Py_DECREF(result);
        }
        
        Py_DECREF(args);
        Py_DECREF(pyPose);
    }

    PyGILState_Release(gstate);
}

// Trampoline for Actor Callback
static void onActor(RemoteCaptury* rc, int actorId, int mode, void* userArg) {
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    if (actorCallback && PyCallable_Check(actorCallback)) {
        PyObject* args = Py_BuildValue("(ii)", actorId, mode);
        PyObject* result = PyObject_CallObject(actorCallback, args);
        
        if (result == NULL) {
            PyErr_Print();
        } else {
            Py_DECREF(result);
        }
        Py_DECREF(args);
    }

    PyGILState_Release(gstate);
}

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

static PyObject* disconnect(PyObject *self, PyObject *args)
{
    if (rc) {
        Captury_disconnect(rc);
        // Captury_destroy(rc); // Maybe don't destroy, just disconnect?
        // rc = nullptr;
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* startStreaming(PyObject *self, PyObject *args)
{
    int what = CAPTURY_STREAM_POSES; // Default
    if (!PyArg_ParseTuple(args, "|i:startStreaming", &what)) {
        return NULL;
    }

    if (rc && Captury_startStreaming(rc, what))
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject* stopStreaming(PyObject *self, PyObject *args)
{
    if (rc && Captury_stopStreaming(rc))
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject* registerPoseCallback(PyObject *self, PyObject *args)
{
    PyObject *temp;
    if (PyArg_ParseTuple(args, "O:registerPoseCallback", &temp)) {
        if (!PyCallable_Check(temp)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            return NULL;
        }
        Py_XINCREF(temp);         /* Add a reference to new callback */
        Py_XDECREF(poseCallback); /* Dispose of previous callback */
        poseCallback = temp;      /* Remember new callback */
        
        if (rc) {
            Captury_registerNewPoseCallback(rc, onPose, NULL);
            Py_RETURN_TRUE;
        }
    }
    Py_RETURN_FALSE;
}

static PyObject* registerActorCallback(PyObject *self, PyObject *args)
{
    PyObject *temp;
    if (PyArg_ParseTuple(args, "O:registerActorCallback", &temp)) {
        if (!PyCallable_Check(temp)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            return NULL;
        }
        Py_XINCREF(temp);
        Py_XDECREF(actorCallback);
        actorCallback = temp;
        
        if (rc) {
            Captury_registerActorChangedCallback(rc, onActor, NULL);
            Py_RETURN_TRUE;
        }
    }
    Py_RETURN_FALSE;
}

static PyMethodDef pythonVisibleMethods[] = {
    {"connect", connect, METH_VARARGS, "Connect to host[, port=2101]"},
    {"disconnect", disconnect, METH_NOARGS, "Disconnect from host"},
    {"startStreaming", startStreaming, METH_VARARGS, "Start streaming data"},
    {"stopStreaming", stopStreaming, METH_NOARGS, "Stop streaming"},
    {"registerPoseCallback", registerPoseCallback, METH_VARARGS, "Register a callback for new poses"},
    {"registerActorCallback", registerActorCallback, METH_VARARGS, "Register a callback for actor changes"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef rcModule = {
    PyModuleDef_HEAD_INIT,
    "_remotecaptury",   /* name of module */
    "Internal C++ wrapper for RemoteCaptury", /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    pythonVisibleMethods
};

PyMODINIT_FUNC
PyInit__remotecaptury(void)
{
    return PyModule_Create(&rcModule);
}
