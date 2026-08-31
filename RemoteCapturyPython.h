#pragma once

#include <Python.h>
#include "RemoteCaptury.h"

#ifdef __cplusplus
extern "C" {
#endif

RemoteCaptury* RemoteCapturyPython_getHandle(void);
void RemoteCapturyPython_setHandle(RemoteCaptury* rc);

PyObject* RemoteCapturyPython_actorToDict(const CapturyActor* actor);
PyObject* RemoteCapturyPython_poseToDict(const CapturyPose* pose);
PyObject* RemoteCapturyPython_cameraToDict(const CapturyCamera* camera);
PyObject* RemoteCapturyPython_angleDataToList(const CapturyAngleData* values, int numAngles);
PyObject* RemoteCapturyPython_artagToList(const CapturyARTag* artags);
PyObject* RemoteCapturyPython_imageToDict(const CapturyImage* image);

#ifdef __cplusplus
}
#endif
