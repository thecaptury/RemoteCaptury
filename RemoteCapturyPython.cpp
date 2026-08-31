#include "RemoteCapturyPython.h"

#include <vector>
#include <string>
#include <cstring>

static RemoteCaptury* g_rc = nullptr;
static PyObject* g_newPoseCallback = nullptr;
static PyObject* g_newAnglesCallback = nullptr;
static PyObject* g_actorChangedCallback = nullptr;
static PyObject* g_artagCallback = nullptr;
static PyObject* g_imageCallback = nullptr;

RemoteCaptury* RemoteCapturyPython_getHandle(void)
{
	return g_rc;
}

void RemoteCapturyPython_setHandle(RemoteCaptury* rc)
{
	g_rc = rc;
}

static PyObject* transformToDict(const CapturyTransform& tr)
{
	PyObject* dict = PyDict_New();
	PyObject* translation = Py_BuildValue("(fff)", tr.translation[0], tr.translation[1], tr.translation[2]);
	PyObject* rotation = Py_BuildValue("(fff)", tr.rotation[0], tr.rotation[1], tr.rotation[2]);
	PyDict_SetItemString(dict, "translation", translation);
	PyDict_SetItemString(dict, "rotation", rotation);
	Py_DECREF(translation);
	Py_DECREF(rotation);
	return dict;
}

static PyObject* actorToDict(const CapturyActor* actor)
{
	if (!actor)
		Py_RETURN_NONE;

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "name", PyUnicode_FromString(actor->name));
	PyDict_SetItemString(dict, "id", PyLong_FromLong(actor->id));
	PyDict_SetItemString(dict, "numJoints", PyLong_FromLong(actor->numJoints));
	PyDict_SetItemString(dict, "numBlobs", PyLong_FromLong(actor->numBlobs));
	PyDict_SetItemString(dict, "numBlendShapes", PyLong_FromLong(actor->numBlendShapes));
	PyDict_SetItemString(dict, "numMetaData", PyLong_FromLong(actor->numMetaData));
	return dict;
}

static PyObject* poseToDict(const CapturyPose* pose)
{
	if (!pose)
		Py_RETURN_NONE;

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "actor", PyLong_FromLong(pose->actor));
	PyDict_SetItemString(dict, "timestamp", PyLong_FromUnsignedLongLong(pose->timestamp));
	PyDict_SetItemString(dict, "flags", PyLong_FromUnsignedLong(pose->flags));
	PyDict_SetItemString(dict, "numTransforms", PyLong_FromLong(pose->numTransforms));
	PyDict_SetItemString(dict, "numBlendShapes", PyLong_FromLong(pose->numBlendShapes));

	PyObject* transforms = PyList_New(pose->numTransforms);
	for (int i = 0; i < pose->numTransforms; ++i) {
		PyList_SetItem(transforms, i, transformToDict(pose->transforms[i]));
	}
	PyDict_SetItemString(dict, "transforms", transforms);
	Py_DECREF(transforms);

	if (pose->numBlendShapes > 0 && pose->blendShapeActivations) {
		PyObject* values = PyList_New(pose->numBlendShapes);
		for (int i = 0; i < pose->numBlendShapes; ++i) {
			PyList_SetItem(values, i, PyFloat_FromDouble(pose->blendShapeActivations[i]));
		}
		PyDict_SetItemString(dict, "blendShapeActivations", values);
		Py_DECREF(values);
	}

	return dict;
}

static PyObject* cameraToDict(const CapturyCamera* camera)
{
	if (!camera)
		Py_RETURN_NONE;

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "name", PyUnicode_FromString(camera->name));
	PyDict_SetItemString(dict, "id", PyLong_FromLong(camera->id));
	PyDict_SetItemString(dict, "position", Py_BuildValue("(fff)", camera->position[0], camera->position[1], camera->position[2]));
	PyDict_SetItemString(dict, "orientation", Py_BuildValue("(fff)", camera->orientation[0], camera->orientation[1], camera->orientation[2]));
	PyDict_SetItemString(dict, "sensorSize", Py_BuildValue("(ff)", camera->sensorSize[0], camera->sensorSize[1]));
	PyDict_SetItemString(dict, "focalLength", PyFloat_FromDouble(camera->focalLength));
	PyDict_SetItemString(dict, "lensCenter", Py_BuildValue("(ff)", camera->lensCenter[0], camera->lensCenter[1]));
	PyDict_SetItemString(dict, "distortionModel", PyUnicode_FromString(camera->distortionModel));
	return dict;
}

static PyObject* angleDataToList(const CapturyAngleData* values, int numAngles)
{
	PyObject* list = PyList_New(numAngles);
	for (int i = 0; i < numAngles; ++i) {
		PyObject* entry = Py_BuildValue("(i f)", values[i].type, values[i].value);
		PyList_SetItem(list, i, entry);
	}
	return list;
}

static PyObject* artagToList(const CapturyARTag* artags)
{
	PyObject* list = PyList_New(0);
	if (!artags)
		return list;

	int count = 0;
	while (artags[count].id != -1) {
		++count;
	}

	PyList_SetSlice(list, 0, 0, PyList_New(0));
	PyObject* newList = PyList_New(count);
	for (int i = 0; i < count; ++i) {
		PyObject* item = PyDict_New();
		PyDict_SetItemString(item, "id", PyLong_FromLong(artags[i].id));
		PyDict_SetItemString(item, "transform", transformToDict(artags[i].transform));
		PyList_SetItem(newList, i, item);
	}
	return newList;
}

static PyObject* imageToDict(const CapturyImage* image)
{
	if (!image)
		Py_RETURN_NONE;

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "width", PyLong_FromLong(image->width));
	PyDict_SetItemString(dict, "height", PyLong_FromLong(image->height));
	PyDict_SetItemString(dict, "camera", PyLong_FromLong(image->camera));
	PyDict_SetItemString(dict, "timestamp", PyLong_FromUnsignedLongLong(image->timestamp));
	if (image->data && image->width > 0 && image->height > 0) {
		Py_ssize_t size = static_cast<Py_ssize_t>(image->width) * static_cast<Py_ssize_t>(image->height) * 3;
		PyDict_SetItemString(dict, "data", PyBytes_FromStringAndSize(reinterpret_cast<const char*>(image->data), size));
	}
	return dict;
}

static void onPoseCallback(RemoteCaptury* rc, CapturyActor* actor, CapturyPose* pose, int trackingQuality, void* userArg)
{
	(void)rc; (void)userArg;
	PyGILState_STATE state = PyGILState_Ensure();
	if (g_newPoseCallback && PyCallable_Check(g_newPoseCallback)) {
		PyObject* pyActorId = PyLong_FromLong(actor ? actor->id : -1);
		PyObject* pyPose = poseToDict(pose);
		PyObject* args = PyTuple_Pack(2, pyActorId, pyPose);
		PyObject* result = PyObject_CallObject(g_newPoseCallback, args);
		Py_XDECREF(result);
		Py_DECREF(args);
		Py_DECREF(pyPose);
		Py_DECREF(pyActorId);
	}
	PyGILState_Release(state);
}

static void onActorChangedCallback(RemoteCaptury* rc, int actorId, int mode, void* userArg)
{
	(void)rc; (void)userArg;
	PyGILState_STATE state = PyGILState_Ensure();
	if (g_actorChangedCallback && PyCallable_Check(g_actorChangedCallback)) {
		PyObject* args = PyTuple_Pack(2, PyLong_FromLong(actorId), PyLong_FromLong(mode));
		PyObject* result = PyObject_CallObject(g_actorChangedCallback, args);
		Py_XDECREF(result);
		Py_DECREF(args);
	}
	PyGILState_Release(state);
}

static void onAnglesCallback(RemoteCaptury* rc, const CapturyActor* actor, int numAngles, struct CapturyAngleData* values, void* userArg)
{
	(void)rc; (void)userArg;
	PyGILState_STATE state = PyGILState_Ensure();
	if (g_newAnglesCallback && PyCallable_Check(g_newAnglesCallback)) {
		PyObject* pyActor = actorToDict(actor);
		PyObject* pyAngles = angleDataToList(values, numAngles);
		PyObject* args = PyTuple_Pack(2, pyActor, pyAngles);
		PyObject* result = PyObject_CallObject(g_newAnglesCallback, args);
		Py_XDECREF(result);
		Py_DECREF(args);
		Py_DECREF(pyAngles);
		Py_DECREF(pyActor);
	}
	PyGILState_Release(state);
}

static void onARTagCallback(RemoteCaptury* rc, int num, CapturyARTag* artags, void* userArg)
{
	(void)rc; (void)userArg;
	PyGILState_STATE state = PyGILState_Ensure();
	if (g_artagCallback && PyCallable_Check(g_artagCallback)) {
		PyObject* pyArtags = artagToList(artags);
		PyObject* args = PyTuple_Pack(2, PyLong_FromLong(num), pyArtags);
		PyObject* result = PyObject_CallObject(g_artagCallback, args);
		Py_XDECREF(result);
		Py_DECREF(args);
		Py_DECREF(pyArtags);
	}
	PyGILState_Release(state);
}

static void onImageCallback(RemoteCaptury* rc, const CapturyImage* img, void* userArg)
{
	(void)rc; (void)userArg;
	PyGILState_STATE state = PyGILState_Ensure();
	if (g_imageCallback && PyCallable_Check(g_imageCallback)) {
		PyObject* pyImage = imageToDict(img);
		PyObject* args = PyTuple_Pack(1, pyImage);
		PyObject* result = PyObject_CallObject(g_imageCallback, args);
		Py_XDECREF(result);
		Py_DECREF(args);
		Py_DECREF(pyImage);
	}
	PyGILState_Release(state);
}

static int ensureHandle(void)
{
	if (!g_rc) {
		g_rc = Captury_create();
		if (!g_rc) {
			PyErr_SetString(PyExc_RuntimeError, "Captury_create() failed");
			return 0;
		}
	}
	return 1;
}

static PyObject* pyConnect(PyObject* self, PyObject* args)
{
	const char* host = "";
	int port = 2101;
	if (!PyArg_ParseTuple(args, "|si:connect", &host, &port)) {
		return nullptr;
	}
	if (!ensureHandle()) {
		return nullptr;
	}
	if (Captury_connect(g_rc, host, static_cast<unsigned short>(port))) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyConnect2(PyObject* self, PyObject* args)
{
	const char* host = nullptr;
	int port = 2101;
	int localPort = 0;
	int localStreamPort = 0;
	int async = 0;
	const char* localAddress = nullptr;
	const char* multicastAddress = nullptr;
	if (!PyArg_ParseTuple(args, "siiii|ss:connect2", &host, &port, &localPort, &localStreamPort, &async, &localAddress, &multicastAddress)) {
		return nullptr;
	}
	if (!ensureHandle()) {
		return nullptr;
	}
	if (Captury_connect2(g_rc, host, static_cast<unsigned short>(port), static_cast<unsigned short>(localPort), static_cast<unsigned short>(localStreamPort), async, localAddress, multicastAddress)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyDiscoverServers(PyObject* self, PyObject* args)
{
	int port = 2101;
	const char* multicastAddress = nullptr;
	if (!PyArg_ParseTuple(args, "|is:discoverServers", &port, &multicastAddress)) {
		return nullptr;
	}
	if (!ensureHandle()) {
		return nullptr;
	}
	char* serverNames = nullptr;
	if (Captury_discoverServers(g_rc, static_cast<unsigned short>(port), multicastAddress, &serverNames) <= 0 || !serverNames) {
		Py_RETURN_NONE;
	}
	PyObject* result = PyUnicode_FromString(serverNames);
	Captury_freeString(serverNames);
	return result;
}

static PyObject* pyDestroy(PyObject* self, PyObject* args)
{
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	Captury_destroy(g_rc);
	g_rc = nullptr;
	Py_RETURN_NONE;
}

static PyObject* pyDisconnect(PyObject* self, PyObject* args)
{
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_disconnect(g_rc)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyGetConnectionStatus(PyObject* self, PyObject* args)
{
	if (!g_rc) {
		return PyLong_FromLong(CAPTURY_DISCONNECTED);
	}
	return PyLong_FromLong(Captury_getConnectionStatus(g_rc));
}

static PyObject* pyGetActors(PyObject* self, PyObject* args)
{
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	const CapturyActor* actors = nullptr;
	int count = Captury_getActors(g_rc, &actors);
	if (count <= 0 || !actors) {
		Py_RETURN_NONE;
	}
	PyObject* list = PyList_New(count);
	for (int i = 0; i < count; ++i) {
		PyList_SetItem(list, i, actorToDict(&actors[i]));
	}
	Captury_freeActors(g_rc);
	return list;
}

static PyObject* pyGetActor(PyObject* self, PyObject* args)
{
	int actorId = 0;
	if (!PyArg_ParseTuple(args, "i:getActor", &actorId)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	const CapturyActor* actor = Captury_getActor(g_rc, actorId);
	if (!actor) {
		Py_RETURN_NONE;
	}
	PyObject* result = actorToDict(actor);
	Captury_freeActor(g_rc, actor);
	return result;
}

static PyObject* pyGetCameras(PyObject* self, PyObject* args)
{
	int waitTimeMs = 0;
	if (!PyArg_ParseTuple(args, "|i:getCameras", &waitTimeMs)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	const CapturyCamera* cameras = nullptr;
	int count = Captury_getCameras(g_rc, &cameras, waitTimeMs);
	if (count <= 0 || !cameras) {
		Py_RETURN_NONE;
	}
	PyObject* list = PyList_New(count);
	for (int i = 0; i < count; ++i) {
		PyList_SetItem(list, i, cameraToDict(&cameras[i]));
	}
	return list;
}

static PyObject* pyStartStreaming(PyObject* self, PyObject* args, PyObject* kwargs)
{
	static char* kwlist[] = {(char*)"what", nullptr};
	int what = CAPTURY_STREAM_POSES;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:startStreaming", kwlist, &what)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_startStreaming(g_rc, what)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyStartStreamingImages(PyObject* self, PyObject* args)
{
	int what = 0;
	int cameraId = 0;
	if (!PyArg_ParseTuple(args, "ii:startStreamingImages", &what, &cameraId)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_startStreamingImages(g_rc, what, cameraId)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyStartStreamingImagesAndAngles(PyObject* self, PyObject* args)
{
	int what = 0;
	int cameraId = 0;
	PyObject* pyAngles = nullptr;
	if (!PyArg_ParseTuple(args, "iiO:startStreamingImagesAndAngles", &what, &cameraId, &pyAngles)) {
		return nullptr;
	}
	if (!PyList_Check(pyAngles) && !PyTuple_Check(pyAngles)) {
		PyErr_SetString(PyExc_TypeError, "angles must be a list or tuple of integers");
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	Py_ssize_t count = PySequence_Size(pyAngles);
	std::vector<uint16_t> angles(static_cast<size_t>(count));
	for (Py_ssize_t i = 0; i < count; ++i) {
		PyObject* item = PySequence_GetItem(pyAngles, i);
		angles[static_cast<size_t>(i)] = static_cast<uint16_t>(PyLong_AsLong(item));
		Py_DECREF(item);
	}
	if (Captury_startStreamingImagesAndAngles(g_rc, what, cameraId, static_cast<int>(count), angles.empty() ? nullptr : angles.data())) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyStopStreaming(PyObject* self, PyObject* args)
{
	int wait = 1;
	if (!PyArg_ParseTuple(args, "|i:stopStreaming", &wait)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_stopStreaming(g_rc, wait)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyGetCurrentPoseForActor(PyObject* self, PyObject* args)
{
	int actorId = -1;
	if (!PyArg_ParseTuple(args, "i:getCurrentPoseForActor", &actorId)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	CapturyPose* pose = Captury_getCurrentPoseForActor(g_rc, actorId);
	PyObject* result = poseToDict(pose);
	Captury_freePose(pose);
	return result;
}

static PyObject* pyGetCurrentPose(PyObject* self, PyObject* args)
{
	int actorId = -1;
	if (!PyArg_ParseTuple(args, "i:getCurrentPose", &actorId)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	CapturyPose* pose = Captury_getCurrentPose(g_rc, actorId);
	PyObject* result = poseToDict(pose);
	Captury_freePose(pose);
	return result;
}

static PyObject* pyGetCurrentAngles(PyObject* self, PyObject* args)
{
	int actorId = -1;
	int numAngles = 0;
	if (!PyArg_ParseTuple(args, "i:getCurrentAngles", &actorId)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	CapturyAngleData* values = Captury_getCurrentAngles(g_rc, actorId, &numAngles);
	if (!values || numAngles <= 0) {
		Py_RETURN_NONE;
	}
	PyObject* result = angleDataToList(values, numAngles);
	free(values);
	return result;
}

static PyObject* pyRegisterNewPoseCallback(PyObject* self, PyObject* args)
{
	PyObject* callback = nullptr;
	if (!PyArg_ParseTuple(args, "O:registerNewPoseCallback", &callback)) {
		return nullptr;
	}
	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "callback must be callable");
		return nullptr;
	}
	Py_XINCREF(callback);
	Py_XDECREF(g_newPoseCallback);
	g_newPoseCallback = callback;
	if (g_rc) {
		Captury_registerNewPoseCallback(g_rc, onPoseCallback, nullptr);
	}
	Py_RETURN_TRUE;
}

static PyObject* pyRegisterNewAnglesCallback(PyObject* self, PyObject* args)
{
	PyObject* callback = nullptr;
	if (!PyArg_ParseTuple(args, "O:registerNewAnglesCallback", &callback)) {
		return nullptr;
	}
	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "callback must be callable");
		return nullptr;
	}
	Py_XINCREF(callback);
	Py_XDECREF(g_newAnglesCallback);
	g_newAnglesCallback = callback;
	if (g_rc) {
		Captury_registerNewAnglesCallback(g_rc, onAnglesCallback, nullptr);
	}
	Py_RETURN_TRUE;
}

static PyObject* pyRegisterActorChangedCallback(PyObject* self, PyObject* args)
{
	PyObject* callback = nullptr;
	if (!PyArg_ParseTuple(args, "O:registerActorChangedCallback", &callback)) {
		return nullptr;
	}
	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "callback must be callable");
		return nullptr;
	}
	Py_XINCREF(callback);
	Py_XDECREF(g_actorChangedCallback);
	g_actorChangedCallback = callback;
	if (g_rc) {
		Captury_registerActorChangedCallback(g_rc, onActorChangedCallback, nullptr);
	}
	Py_RETURN_TRUE;
}

static PyObject* pyRegisterARTagCallback(PyObject* self, PyObject* args)
{
	PyObject* callback = nullptr;
	if (!PyArg_ParseTuple(args, "O:registerARTagCallback", &callback)) {
		return nullptr;
	}
	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "callback must be callable");
		return nullptr;
	}
	Py_XINCREF(callback);
	Py_XDECREF(g_artagCallback);
	g_artagCallback = callback;
	if (g_rc) {
		Captury_registerARTagCallback(g_rc, onARTagCallback, nullptr);
	}
	Py_RETURN_TRUE;
}

static PyObject* pyGetCurrentARTags(PyObject* self, PyObject* args)
{
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	CapturyARTag* artags = Captury_getCurrentARTags(g_rc);
	PyObject* result = artagToList(artags);
	Captury_freeARTags(artags);
	return result;
}

static PyObject* pyRegisterImageStreamingCallback(PyObject* self, PyObject* args)
{
	PyObject* callback = nullptr;
	if (!PyArg_ParseTuple(args, "O:registerImageStreamingCallback", &callback)) {
		return nullptr;
	}
	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "callback must be callable");
		return nullptr;
	}
	Py_XINCREF(callback);
	Py_XDECREF(g_imageCallback);
	g_imageCallback = callback;
	if (g_rc) {
		Captury_registerImageStreamingCallback(g_rc, onImageCallback, nullptr);
	}
	Py_RETURN_TRUE;
}

static PyObject* pyGetCurrentImage(PyObject* self, PyObject* args)
{
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	CapturyImage* image = Captury_getCurrentImage(g_rc);
	PyObject* result = imageToDict(image);
	Captury_freeImage(image);
	return result;
}

static PyObject* pyRequestTexture(PyObject* self, PyObject* args)
{
	int actorId = 0;
	if (!PyArg_ParseTuple(args, "i:requestTexture", &actorId)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_requestTexture(g_rc, actorId)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyGetMarkerTransform(PyObject* self, PyObject* args)
{
	int actorId = 0;
	int joint = 0;
	if (!PyArg_ParseTuple(args, "ii:getMarkerTransform", &actorId, &joint)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	CapturyTransform trafo = {};
	uint64_t timestamp = Captury_getMarkerTransform(g_rc, actorId, joint, &trafo);
	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "timestamp", PyLong_FromUnsignedLongLong(timestamp));
	PyDict_SetItemString(dict, "transform", transformToDict(trafo));
	return dict;
}

static PyObject* pyGetScalingProgress(PyObject* self, PyObject* args)
{
	int actorId = 0;
	if (!PyArg_ParseTuple(args, "i:getScalingProgress", &actorId)) {
		return nullptr;
	}
	if (!g_rc) {
		return PyLong_FromLong(0);
	}
	return PyLong_FromLong(Captury_getScalingProgress(g_rc, actorId));
}

static PyObject* pyGetTrackingQuality(PyObject* self, PyObject* args)
{
	int actorId = 0;
	if (!PyArg_ParseTuple(args, "i:getTrackingQuality", &actorId)) {
		return nullptr;
	}
	if (!g_rc) {
		return PyLong_FromLong(0);
	}
	return PyLong_FromLong(Captury_getTrackingQuality(g_rc, actorId));
}

static PyObject* pySetActorName(PyObject* self, PyObject* args)
{
	int actorId = 0;
	const char* name = nullptr;
	if (!PyArg_ParseTuple(args, "is:setActorName", &actorId, &name)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_setActorName(g_rc, actorId, name)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyGetTexture(PyObject* self, PyObject* args)
{
	int actorId = 0;
	if (!PyArg_ParseTuple(args, "i:getTexture", &actorId)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	CapturyImage* image = Captury_getTexture(g_rc, actorId);
	PyObject* result = imageToDict(image);
	Captury_freeImage(image);
	return result;
}

static PyObject* pySynchronizeTime(PyObject* self, PyObject* args)
{
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	return PyLong_FromUnsignedLongLong(Captury_synchronizeTime(g_rc));
}

static PyObject* pyStartTimeSynchronizationLoop(PyObject* self, PyObject* args)
{
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	Captury_startTimeSynchronizationLoop(g_rc);
	Py_RETURN_NONE;
}

static PyObject* pyGetTime(PyObject* self, PyObject* args)
{
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	return PyLong_FromUnsignedLongLong(Captury_getTime(g_rc));
}

static PyObject* pyGetTimeOffset(PyObject* self, PyObject* args)
{
	if (!g_rc) {
		return PyLong_FromLong(0);
	}
	return PyLong_FromLongLong(Captury_getTimeOffset(g_rc));
}

static PyObject* pySnapActor(PyObject* self, PyObject* args)
{
	float x = 0.0f;
	float z = 0.0f;
	float heading = 370.0f;
	if (!PyArg_ParseTuple(args, "ff|f:snapActor", &x, &z, &heading)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_snapActor(g_rc, x, z, heading)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pySnapActorEx(PyObject* self, PyObject* args)
{
	float x = 0.0f;
	float z = 0.0f;
	float radius = 0.0f;
	float heading = 370.0f;
	const char* skeletonName = nullptr;
	int snapMethod = SNAP_DEFAULT;
	int quickScaling = 0;
	if (!PyArg_ParseTuple(args, "fffs|ii:snapActorEx", &x, &z, &radius, &heading, &skeletonName, &snapMethod, &quickScaling)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_snapActorEx(g_rc, x, z, radius, heading, skeletonName, snapMethod, quickScaling)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyStartTracking(PyObject* self, PyObject* args)
{
	int actorId = 0;
	float x = 0.0f;
	float z = 0.0f;
	float heading = 370.0f;
	if (!PyArg_ParseTuple(args, "i ff|f:startTracking", &actorId, &x, &z, &heading)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_startTracking(g_rc, actorId, x, z, heading)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyStopTracking(PyObject* self, PyObject* args)
{
	int actorId = 0;
	if (!PyArg_ParseTuple(args, "i:stopTracking", &actorId)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_stopTracking(g_rc, actorId)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyDeleteActor(PyObject* self, PyObject* args)
{
	int actorId = 0;
	if (!PyArg_ParseTuple(args, "i:deleteActor", &actorId)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_deleteActor(g_rc, actorId)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyRescaleActor(PyObject* self, PyObject* args)
{
	int actorId = 0;
	if (!PyArg_ParseTuple(args, "i:rescaleActor", &actorId)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_rescaleActor(g_rc, actorId)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyRecolorActor(PyObject* self, PyObject* args)
{
	int actorId = 0;
	if (!PyArg_ParseTuple(args, "i:recolorActor", &actorId)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_recolorActor(g_rc, actorId)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyUpdateActorColors(PyObject* self, PyObject* args)
{
	int actorId = 0;
	if (!PyArg_ParseTuple(args, "i:updateActorColors", &actorId)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_updateActorColors(g_rc, actorId)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pySetShotName(PyObject* self, PyObject* args)
{
	const char* name = nullptr;
	if (!PyArg_ParseTuple(args, "s:setShotName", &name)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_setShotName(g_rc, name)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyStartRecording(PyObject* self, PyObject* args)
{
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_startRecording(g_rc)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyStopRecording(PyObject* self, PyObject* args)
{
	if (!g_rc) {
		Py_RETURN_FALSE;
	}
	if (Captury_stopRecording(g_rc)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* pyGetCurrentLatency(PyObject* self, PyObject* args)
{
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	CapturyLatencyInfo info = {};
	if (Captury_getCurrentLatency(g_rc, &info)) {
		PyObject* dict = PyDict_New();
		PyDict_SetItemString(dict, "firstImagePacketTime", PyLong_FromUnsignedLongLong(info.firstImagePacketTime));
		PyDict_SetItemString(dict, "optimizationStartTime", PyLong_FromUnsignedLongLong(info.optimizationStartTime));
		PyDict_SetItemString(dict, "optimizationEndTime", PyLong_FromUnsignedLongLong(info.optimizationEndTime));
		PyDict_SetItemString(dict, "poseSentTime", PyLong_FromUnsignedLongLong(info.poseSentTime));
		PyDict_SetItemString(dict, "poseReceivedTime", PyLong_FromUnsignedLongLong(info.poseReceivedTime));
		PyDict_SetItemString(dict, "timestampOfCorrespondingPose", PyLong_FromUnsignedLongLong(info.timestampOfCorrespondingPose));
		return dict;
	}
	Py_RETURN_NONE;
}

static PyObject* pyConvertPoseToLocal(PyObject* self, PyObject* args)
{
	int actorId = 0;
	PyObject* pyPose = nullptr;
	if (!PyArg_ParseTuple(args, "iO:convertPoseToLocal", &actorId, &pyPose)) {
		return nullptr;
	}
	if (!g_rc) {
		Py_RETURN_NONE;
	}
	if (!PyDict_Check(pyPose)) {
		PyErr_SetString(PyExc_TypeError, "pose must be a dict");
		return nullptr;
	}
	CapturyPose pose = {};
	pose.actor = actorId;
	pose.numTransforms = 0;
	Captury_convertPoseToLocal(g_rc, &pose, actorId);
	Py_RETURN_NONE;
}

static PyMethodDef pythonVisibleMethods[] = {
	{"create", pyDestroy, METH_NOARGS, "Create a RemoteCaptury handle."},
	{"destroy", pyDestroy, METH_NOARGS, "Destroy the RemoteCaptury handle."},
	{"connect", pyConnect, METH_VARARGS, "Connect to a Captury Live server."},
	{"connect2", pyConnect2, METH_VARARGS, "Connect to a Captury Live server with more control."},
	{"discoverServers", pyDiscoverServers, METH_VARARGS, "Discover Captury servers on the network."},
	{"disconnect", pyDisconnect, METH_NOARGS, "Disconnect from the Captury Live server."},
	{"getConnectionStatus", pyGetConnectionStatus, METH_NOARGS, "Return the connection status."},
	{"getActors", pyGetActors, METH_NOARGS, "Return a list of actor descriptions."},
	{"getActor", pyGetActor, METH_VARARGS, "Return a single actor description."},
	{"getCameras", pyGetCameras, METH_VARARGS, "Return a list of discovered cameras."},
	{"startStreaming", (PyCFunction)pyStartStreaming, METH_VARARGS | METH_KEYWORDS, "Start streaming."},
	{"startStreamingImages", pyStartStreamingImages, METH_VARARGS, "Start image streaming for a specific camera."},
	{"startStreamingImagesAndAngles", pyStartStreamingImagesAndAngles, METH_VARARGS, "Start image streaming and angle streaming."},
	{"stopStreaming", pyStopStreaming, METH_VARARGS, "Stop streaming."},
	{"getCurrentPoseForActor", pyGetCurrentPoseForActor, METH_VARARGS, "Return the latest pose for an actor."},
	{"getCurrentPose", pyGetCurrentPose, METH_VARARGS, "Return the latest pose for an actor."},
	{"getCurrentAngles", pyGetCurrentAngles, METH_VARARGS, "Return the current physiological angle values."},
	{"registerNewPoseCallback", pyRegisterNewPoseCallback, METH_VARARGS, "Register a callback for new poses."},
	{"registerNewAnglesCallback", pyRegisterNewAnglesCallback, METH_VARARGS, "Register a callback for new physiological angle values."},
	{"registerActorChangedCallback", pyRegisterActorChangedCallback, METH_VARARGS, "Register a callback for actor status changes."},
	{"registerARTagCallback", pyRegisterARTagCallback, METH_VARARGS, "Register a callback for detected artags."},
	{"getCurrentARTags", pyGetCurrentARTags, METH_NOARGS, "Return a list of detected artags."},
	{"registerImageStreamingCallback", pyRegisterImageStreamingCallback, METH_VARARGS, "Register a callback for streamed images."},
	{"getCurrentImage", pyGetCurrentImage, METH_NOARGS, "Return the latest image."},
	{"requestTexture", pyRequestTexture, METH_VARARGS, "Request an updated texture for an actor."},
	{"getMarkerTransform", pyGetMarkerTransform, METH_VARARGS, "Return the latest marker transform information."},
	{"getScalingProgress", pyGetScalingProgress, METH_VARARGS, "Return the scaling progress for an actor."},
	{"getTrackingQuality", pyGetTrackingQuality, METH_VARARGS, "Return the tracking quality for an actor."},
	{"setActorName", pySetActorName, METH_VARARGS, "Set the name of an actor."},
	{"getTexture", pyGetTexture, METH_VARARGS, "Return the texture image for an actor."},
	{"synchronizeTime", pySynchronizeTime, METH_NOARGS, "Synchronize clocks with Captury Live once."},
	{"startTimeSynchronizationLoop", pyStartTimeSynchronizationLoop, METH_NOARGS, "Start continuous clock synchronization."},
	{"getTime", pyGetTime, METH_NOARGS, "Return the current time from Captury Live."},
	{"getTimeOffset", pyGetTimeOffset, METH_NOARGS, "Return the clock offset between local and remote time."},
	{"snapActor", pySnapActor, METH_VARARGS, "Snap an actor to a location."},
	{"snapActorEx", pySnapActorEx, METH_VARARGS, "Snap an actor with more control."},
	{"startTracking", pyStartTracking, METH_VARARGS, "Start tracking an actor."},
	{"stopTracking", pyStopTracking, METH_VARARGS, "Stop tracking an actor."},
	{"deleteActor", pyDeleteActor, METH_VARARGS, "Delete an actor."},
	{"rescaleActor", pyRescaleActor, METH_VARARGS, "Rescale an actor."},
	{"recolorActor", pyRecolorActor, METH_VARARGS, "Recolor an actor."},
	{"updateActorColors", pyUpdateActorColors, METH_VARARGS, "Update actor colors."},
	{"setShotName", pySetShotName, METH_VARARGS, "Set the shot name for recording."},
	{"startRecording", pyStartRecording, METH_NOARGS, "Start recording."},
	{"stopRecording", pyStopRecording, METH_NOARGS, "Stop recording."},
	{"getCurrentLatency", pyGetCurrentLatency, METH_NOARGS, "Get the current latency info."},
	{"convertPoseToLocal", pyConvertPoseToLocal, METH_VARARGS, "Convert a pose to local coordinates."},
	{nullptr, nullptr, 0, nullptr}
};

static PyModuleDef rcModule = {
	PyModuleDef_HEAD_INIT,
	"remotecaptury",
	"Python binding for the public RemoteCaptury API.",
	-1,
	pythonVisibleMethods
};

PyMODINIT_FUNC
PyInit__remotecaptury(void)
{
	PyObject* module = PyModule_Create(&rcModule);
	if (!module) {
		return nullptr;
	}
	PyModule_AddIntConstant(module, "CAPTURY_DISCONNECTED", CAPTURY_DISCONNECTED);
	PyModule_AddIntConstant(module, "CAPTURY_CONNECTING", CAPTURY_CONNECTING);
	PyModule_AddIntConstant(module, "CAPTURY_CONNECTED", CAPTURY_CONNECTED);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_NOTHING", CAPTURY_STREAM_NOTHING);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_POSES", CAPTURY_STREAM_POSES);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_GLOBAL_POSES", CAPTURY_STREAM_GLOBAL_POSES);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_LOCAL_POSES", CAPTURY_STREAM_LOCAL_POSES);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_ARTAGS", CAPTURY_STREAM_ARTAGS);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_IMAGES", CAPTURY_STREAM_IMAGES);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_META_DATA", CAPTURY_STREAM_META_DATA);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_IMU_DATA", CAPTURY_STREAM_IMU_DATA);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_LATENCY_INFO", CAPTURY_STREAM_LATENCY_INFO);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_FOOT_CONTACT", CAPTURY_STREAM_FOOT_CONTACT);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_COMPRESSED", CAPTURY_STREAM_COMPRESSED);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_ANGLES", CAPTURY_STREAM_ANGLES);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_SCALES", CAPTURY_STREAM_SCALES);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_BLENDSHAPES", CAPTURY_STREAM_BLENDSHAPES);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_TCP", CAPTURY_STREAM_TCP);
	PyModule_AddIntConstant(module, "CAPTURY_STREAM_ONLY_ROOT_TRANSLATION", CAPTURY_STREAM_ONLY_ROOT_TRANSLATION);
	return module;
}

PyMODINIT_FUNC
PyInit_remotecaptury(void)
{
	return PyInit__remotecaptury();
}

