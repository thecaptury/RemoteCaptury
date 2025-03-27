#pragma once

#include <stdint.h>
#include "captury/PublicStructs.h"

#ifdef WIN32
#define CAPTURY_DLL_EXPORT __declspec(dllexport)
#else
#define CAPTURY_DLL_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct RemoteCaptury;


// returns 1 if successful, 0 otherwise
// the default port is 2101
CAPTURY_DLL_EXPORT RemoteCaptury* Captury_connect(const char* ip, unsigned short port);
// in case you need to set the local port because of firewalls, etc.
// use 0 for localPort and localStreamPort if you don't care
// if async != 0, the function will return immediately and perform the connection attempt asynchronously
CAPTURY_DLL_EXPORT RemoteCaptury* Captury_connect2(const char* ip, unsigned short port, unsigned short localPort, unsigned short localStreamPort, int async);

// returns 1 if successful, 0 otherwise
CAPTURY_DLL_EXPORT int Captury_disconnect(RemoteCaptury* rc);

#define CAPTURY_DISCONNECTED			0 // not connected
#define CAPTURY_CONNECTING			1 // trying to connect
#define CAPTURY_CONNECTED			2 // not connected
// returns one of the above
CAPTURY_DLL_EXPORT int Captury_getConnectionStatus(RemoteCaptury* rc);






// returns the number of actors
// on exit *actors points to an array of CapturyActor*
// the array is valid until the next call of Captury_getActors() or Captury_freeActors()
// free using Captury_freeActors()
CAPTURY_DLL_EXPORT int Captury_getActors(RemoteCaptury* rc, const CapturyActor** actors);

// returns the actor or NULL if it is not known
// free using Captury_freeActor()
CAPTURY_DLL_EXPORT const CapturyActor* Captury_getActor(RemoteCaptury* rc, int actorId);

// free an actor returned by Captury_getActor()
CAPTURY_DLL_EXPORT void Captury_freeActor(RemoteCaptury* rc, const CapturyActor* actor);

// free all actors returned by Captury_getActors()
CAPTURY_DLL_EXPORT void Captury_freeActors(RemoteCaptury* rc);

// returns the number of cameras
// on exit *cameras points to an array of CapturyCamera
// the array is owned by the library - do not free
CAPTURY_DLL_EXPORT int Captury_getCameras(RemoteCaptury* rc, const CapturyCamera** cameras);


#define CAPTURY_LEFT_KNEE_FLEXION_EXTENSION		1
#define CAPTURY_LEFT_KNEE_VARUS_VALGUS			2
#define CAPTURY_LEFT_KNEE_ROTATION			3 // both internal and external
#define CAPTURY_LEFT_HIP_FLEXION_EXTENSION		4
#define CAPTURY_LEFT_HIP_ABADDUCTION			5 // both ab- and adduction
#define CAPTURY_LEFT_HIP_ROTATION			6 // both internal and external
#define CAPTURY_LEFT_ANKLE_FLEXION_EXTENSION		7
#define CAPTURY_LEFT_ANKLE_PRONATION_SUPINATION		8
#define CAPTURY_LEFT_ANKLE_ROTATION			9
#define CAPTURY_LEFT_SHOULDER_FLEXION_EXTENSION		10
#define CAPTURY_LEFT_SHOULDER_TOTAL_FLEXION		11
#define CAPTURY_LEFT_SHOULDER_ABADDUCTION		12 // both ab- and adduction
#define CAPTURY_LEFT_SHOULDER_ROTATION			13
#define CAPTURY_LEFT_ELBOW_FLEXION_EXTENSION		14
#define CAPTURY_LEFT_FOREARM_PRONATION_SUPINATION	15
#define CAPTURY_LEFT_WRIST_FLEXION_EXTENSION		16
#define CAPTURY_LEFT_WRIST_RADIAL_ULNAR_DEVIATION	17
#define CAPTURY_RIGHT_KNEE_FLEXION_EXTENSION		18
#define CAPTURY_RIGHT_KNEE_VARUS_VALGUS			19
#define CAPTURY_RIGHT_KNEE_ROTATION			20 // both internal and external
#define CAPTURY_RIGHT_HIP_FLEXION_EXTENSION		21
#define CAPTURY_RIGHT_HIP_ABADDUCTION			22 // both ab- and adduction
#define CAPTURY_RIGHT_HIP_ROTATION			23 // both internal and external
#define CAPTURY_RIGHT_ANKLE_FLEXION_EXTENSION		24
#define CAPTURY_RIGHT_ANKLE_PRONATION_SUPINATION	25
#define CAPTURY_RIGHT_ANKLE_ROTATION			26
#define CAPTURY_RIGHT_SHOULDER_FLEXION_EXTENSION	27
#define CAPTURY_RIGHT_SHOULDER_TOTAL_FLEXION		28
#define CAPTURY_RIGHT_SHOULDER_ABADDUCTION		29 // both ab- and adduction
#define CAPTURY_RIGHT_SHOULDER_ROTATION			30
#define CAPTURY_RIGHT_ELBOW_FLEXION_EXTENSION		31
#define CAPTURY_RIGHT_FOREARM_PRONATION_SUPINATION	32
#define CAPTURY_RIGHT_WRIST_FLEXION_EXTENSION		33
#define CAPTURY_RIGHT_WRIST_RADIAL_ULNAR_DEVIATION	34
#define CAPTURY_NECK_FLEXION_EXTENSION			35
#define CAPTURY_NECK_ROTATION				36
#define CAPTURY_NECK_LATERAL_BENDING			37
#define CAPTURY_CENTER_OF_GRAVITY_X			38
#define CAPTURY_CENTER_OF_GRAVITY_Y			39
#define CAPTURY_CENTER_OF_GRAVITY_Z			40
#define CAPTURY_HEAD_ROTATION				41
#define CAPTURY_TORSO_ROTATION				42
#define CAPTURY_TORSO_INCLINATION			43
#define CAPTURY_HEAD_INCLINATION			44
#define CAPTURY_TORSO_FLEXION				45



#define CAPTURY_STREAM_NOTHING		0x0000
#define CAPTURY_STREAM_POSES		0x0001
#define CAPTURY_STREAM_GLOBAL_POSES	0x0001
#define CAPTURY_STREAM_LOCAL_POSES	0x0003
#define CAPTURY_STREAM_ARTAGS		0x0004
#define CAPTURY_STREAM_IMAGES		0x0008
#define CAPTURY_STREAM_META_DATA	0x0010	// only valid when streaming poses
#define CAPTURY_STREAM_IMU_DATA		0x0020
#define CAPTURY_STREAM_LATENCY_INFO	0x0040
#define CAPTURY_STREAM_FOOT_CONTACT	0x0080
#define CAPTURY_STREAM_COMPRESSED	0x0100
#define CAPTURY_STREAM_ANGLES		0x0200
#define CAPTURY_STREAM_SCALES		0x0400
#define CAPTURY_STREAM_BLENDSHAPES	0x0800
#define CAPTURY_STREAM_TCP		0x1000

// returns 1 if successful, 0 otherwise
CAPTURY_DLL_EXPORT int Captury_startStreaming(RemoteCaptury* rc, int what);

// if you want to stream images use this function rather than Captury_startStreaming()
// returns 1 if successfull otherwise 0
CAPTURY_DLL_EXPORT int Captury_startStreamingImages(RemoteCaptury* rc, int what, int32_t cameraId);

// if you want to stream images use this function rather than Captury_startStreaming()
// returns 1 if successfull otherwise 0
CAPTURY_DLL_EXPORT int Captury_startStreamingImagesAndAngles(RemoteCaptury* rc, int what, int32_t cameraId, int numAngles, uint16_t* angles);


// equivalent to Captury_startStreaming(CAPTURY_STREAM_NOTHING)
// returns 1 if successful, 0 otherwise
CAPTURY_DLL_EXPORT int Captury_stopStreaming(RemoteCaptury* rc, int wait = 1);

#pragma pack(push, 1)
struct CapturyAngleData {
	uint16_t type;
	float value;
};
#pragma pack(pop)

// fills the pose with the current pose for the given actor
// returns the current pose. Captury_freePose() after use
CAPTURY_DLL_EXPORT CapturyPose* Captury_getCurrentPoseForActor(RemoteCaptury* rc, int actorId);
CAPTURY_DLL_EXPORT CapturyPose* Captury_getCurrentPoseAndTrackingConsistencyForActor(RemoteCaptury* rc, int actorId, int* tc);
CAPTURY_DLL_EXPORT CapturyPose* Captury_getCurrentPose(RemoteCaptury* rc, int actorId);
CAPTURY_DLL_EXPORT CapturyPose* Captury_getCurrentPoseAndTrackingConsistency(RemoteCaptury* rc, int actorId, int* tc);
// *numAngles = number of angles returned
CAPTURY_DLL_EXPORT CapturyAngleData* Captury_getCurrentAngles(RemoteCaptury* rc, int actorId, int* numAngles);

// return a copy of pose
CAPTURY_DLL_EXPORT CapturyPose* Captury_clonePose(const CapturyPose* pose);

// simple function for releasing memory of a pose
CAPTURY_DLL_EXPORT void Captury_freePose(CapturyPose* pose);

typedef void (*CapturyNewPoseCallback)(RemoteCaptury*, CapturyActor*, CapturyPose*, int trackingQuality, void* userArg);

// register callback that will be called when a new pose is received
// the callback will be run in a different thread than the main application
// try to be quick in the callback
// returns 1 if successful otherwise 0
CAPTURY_DLL_EXPORT int Captury_registerNewPoseCallback(RemoteCaptury* rc, CapturyNewPoseCallback callback, void* userArg);

typedef void (*CapturyNewAnglesCallback)(RemoteCaptury*, const CapturyActor*, int numAngles, struct CapturyAngleData* values, void* userArg);

// register callback that will be called when new physiological angle data is received
// the callback will be run in a different thread than the main application
// try to be quick in the callback
// returns 1 if successful otherwise 0
CAPTURY_DLL_EXPORT int Captury_registerNewAnglesCallback(RemoteCaptury* rc, CapturyNewAnglesCallback callback, void* userArg);

typedef enum { ACTOR_SCALING = 0, ACTOR_TRACKING = 1, ACTOR_STOPPED = 2, ACTOR_DELETED = 3, ACTOR_UNKNOWN = 4 } CapturyActorStatus;
extern const char* CapturyActorStatusString[];
typedef void (*CapturyActorChangedCallback)(RemoteCaptury* rc, int actorId, int mode, void* userArg);
// returns CapturyActorStatus if the actorId is not known returns ACTOR_UNKNOWN
// this retrieves the local status. it causes no network traffic and should be fast.
CAPTURY_DLL_EXPORT int Captury_getActorStatus(RemoteCaptury* rc, int actorId);

// register callback that will be called when a new actor is found or
// the status of an existing actor changes
// status can be one of CapturyActorStatus
// returns 1 if successful otherwise 0
CAPTURY_DLL_EXPORT int Captury_registerActorChangedCallback(RemoteCaptury* rc, CapturyActorChangedCallback callback, void* userArg);

typedef void (*CapturyARTagCallback)(RemoteCaptury*, int num, CapturyARTag*, void* userArg);

// register callback that will be called when an artag is detected
// pass NULL if you want to deregister the callback
// returns 1 if successful otherwise 0
CAPTURY_DLL_EXPORT int Captury_registerARTagCallback(RemoteCaptury* rc, CapturyARTagCallback callback, void* userArg);

// returns an array of artags followed by one where the id is -1
// Captury_freeARTags() after use
CAPTURY_DLL_EXPORT CapturyARTag* Captury_getCurrentARTags(RemoteCaptury* rc);

CAPTURY_DLL_EXPORT void Captury_freeARTags(CapturyARTag* artags);

// do NOT free the image
typedef void (*CapturyImageCallback)(RemoteCaptury* rc, const CapturyImage* img, void* userArg);

// register callback that will be called when a new frame was streamed from this particular camera
// pass NULL to deregister
// returns 1 if successfull otherwise 0
CAPTURY_DLL_EXPORT int Captury_registerImageStreamingCallback(RemoteCaptury* rc, CapturyImageCallback callback, void* userArg);

// may return NULL if no image has been received yet
// use Captury_freeImage to free after use
CAPTURY_DLL_EXPORT CapturyImage* Captury_getCurrentImage(RemoteCaptury* rc);

// requests an update of the texture for the given actor. non-blocking
// returns 1 if successful otherwise 0
CAPTURY_DLL_EXPORT int Captury_requestTexture(RemoteCaptury* rc, int actorId);

// returns the timestamp of the constraint or 0
CAPTURY_DLL_EXPORT uint64_t Captury_getMarkerTransform(RemoteCaptury* rc, int actorId, int joint, CapturyTransform* trafo);

// get the scaling status (0 - 100)
CAPTURY_DLL_EXPORT int Captury_getScalingProgress(RemoteCaptury* rc, int actorId);

// get the tracking quality (0 - 100)
CAPTURY_DLL_EXPORT int Captury_getTrackingQuality(RemoteCaptury* rc, int actorId);

// change the name of the actor
CAPTURY_DLL_EXPORT int Captury_setActorName(RemoteCaptury* rc, int actorId, const char* name);

// returns a texture image of the specified actor. free after use with Captury_freeImage().
CAPTURY_DLL_EXPORT CapturyImage* Captury_getTexture(RemoteCaptury* rc, int actorId);

// simple function for releasing memory of a pose
CAPTURY_DLL_EXPORT void Captury_freeImage(CapturyImage* image);

// synchronizes time with Captury Live
// this function should be called once before calling Captury_getTime()
// returns the current time in microseconds
CAPTURY_DLL_EXPORT uint64_t Captury_synchronizeTime(RemoteCaptury* rc);

// start a thread that continuously synchronizes the time with Captury Live
// if this is running it is not necessary to call Captury_synchronizeTime()
CAPTURY_DLL_EXPORT void Captury_startTimeSynchronizationLoop(RemoteCaptury* rc);

// returns the current time as measured by Captury Live in microseconds
CAPTURY_DLL_EXPORT uint64_t Captury_getTime(RemoteCaptury* rc);

// returns the difference between the local and the remote time in microseconds
// offset = CapturyLive.time - local.time
CAPTURY_DLL_EXPORT int64_t Captury_getTimeOffset(RemoteCaptury* rc);

// returns the current tracking framerate
CAPTURY_DLL_EXPORT void Captury_getFramerate(RemoteCaptury* rc, int* numerator, int* denominator);

// get the last error message
CAPTURY_DLL_EXPORT char* Captury_getLastErrorMessage(RemoteCaptury* rc);
CAPTURY_DLL_EXPORT void Captury_freeErrorMessage(char* msg);



// tries to snap an actor at the specified location
// x and z are in mm
// heading is in degrees measured from the x-axis around the y-axis (270 is facing the z-axis)
// use a value larger than 360 to indicate that heading is not known
// poll Captury_getActors to get the new actor id
// returns 1 if the request was successfully received
CAPTURY_DLL_EXPORT int Captury_snapActor(RemoteCaptury* rc, float x, float z, float heading);

// tries to snap an actor at the specified location just like Captury_snapActor()
// the additional parameters allow specifying which skeleton should be snapped (the name should match the name in the drop down)
// snapMethod should be one of CapturySnapMethod
// if quickScaling != 0 snapping uses only a single frame
// returns 1 on success, 0 otherwise
typedef enum { SNAP_BACKGROUND_LOCAL, SNAP_BACKGROUND_GLOBAL, SNAP_BODYPARTS_LOCAL, SNAP_BODYPARTS_GLOBAL, SNAP_BODYPARTS_JOINTS, SNAP_DEFAULT } CapturySnapMethod;
CAPTURY_DLL_EXPORT int Captury_snapActorEx(RemoteCaptury* rc, float x, float z, float radius, float heading, const char* skeletonName, int snapMethod, int quickScaling);

// (re-)start tracking the actor at the given location
// x and z are in mm
// heading is in degrees measured from the x-axis around the y-axis (270 is facing the z-axis)
// use a value larger than 360 to indicate that heading is not known
// returns 1 on success, 0 otherwise
CAPTURY_DLL_EXPORT int Captury_startTracking(RemoteCaptury* rc, int actorId, float x, float z, float heading);

// stops tracking the specified actor
// returns 1 on success, 0 otherwise
CAPTURY_DLL_EXPORT int Captury_stopTracking(RemoteCaptury* rc, int actorId);

// stops tracking the actor and deletes the corresponding internal data in CapturyLive
// returns 1 on success, 0 otherwise
CAPTURY_DLL_EXPORT int Captury_deleteActor(RemoteCaptury* rc, int actorId);

// rescale actor
// returns 1 on success, 0 otherwise
CAPTURY_DLL_EXPORT int Captury_rescaleActor(RemoteCaptury* rc, int actorId);

// recolor actor
// returns 1 on success, 0 otherwise
CAPTURY_DLL_EXPORT int Captury_recolorActor(RemoteCaptury* rc, int actorId);

// recolor actor
// returns 1 on success, 0 otherwise
CAPTURY_DLL_EXPORT int Captury_updateActorColors(RemoteCaptury* rc, int actorId);



// sets the shot name for the next recording
// returns 1 if successful, 0 otherwise
CAPTURY_DLL_EXPORT int Captury_setShotName(RemoteCaptury* rc, const char* name);

// you have to set the shot name before starting to record - or make sure that it has been set using CapturyLive
// returns the timestamp when recording starts (on the CapturyLive machine) if successful, 0 otherwise
CAPTURY_DLL_EXPORT int64_t Captury_startRecording(RemoteCaptury* rc);

// returns 1 if successful, 0 otherwise
CAPTURY_DLL_EXPORT int Captury_stopRecording(RemoteCaptury* rc);

// returns 1 if successful, 0 otherwise
CAPTURY_DLL_EXPORT int Captury_getCurrentLatency(RemoteCaptury* rc, CapturyLatencyInfo* latencyInfo);


// convert the pose given in global coordinates into local coordinates
CAPTURY_DLL_EXPORT void Captury_convertPoseToLocal(RemoteCaptury* rc, CapturyPose* pose, int actorId);


typedef void (*CapturyBackgroundFinishedCallback)(RemoteCaptury*, void* userData);

CAPTURY_DLL_EXPORT int Captury_captureBackground(RemoteCaptury* rc, CapturyBackgroundFinishedCallback callback, void* userData);
CAPTURY_DLL_EXPORT int Captury_getBackgroundQuality(RemoteCaptury* rc);

CAPTURY_DLL_EXPORT const char* Captury_getStatus(RemoteCaptury* rc); // do not free.

CAPTURY_DLL_EXPORT void Captury_enablePrintf(RemoteCaptury* rc, int on); // 0 to turn off
CAPTURY_DLL_EXPORT void Captury_enableRemoteLogging(RemoteCaptury* rc, int on); // 0 to turn off
CAPTURY_DLL_EXPORT const char* Captury_getNextLogMessage(RemoteCaptury* rc); // do free.

#define CAPTURY_LOG_FATAL	0	// this is definitely causing a crash
#define CAPTURY_LOG_ERROR	1	// for things that went so wrong
// that the program will probably not work
#define CAPTURY_LOG_WARNING	2	// when things went wrong but the program
					// is probably going to work anyhow
#define CAPTURY_LOG_IMPORTANT	3	// the program is running normally but some
					// important messages needs to be passed to the user
#define CAPTURY_LOG_INFO	4	// the program is running normally but some
					// interesting points have been reached
#define CAPTURY_LOG_DEBUG	5	// debugging messages
#define CAPTURY_LOG_TRACE	6	// for tracing function calls

CAPTURY_DLL_EXPORT void Captury_log(RemoteCaptury* rc, int logLevel, const char* format, ...);

//
// it is safe to ignore everything below this line
//
typedef enum { capturyActors = 1, capturyActor = 2,
	       capturyCameras = 3, capturyCamera = 4,
	       capturyStream = 5, capturyStreamAck = 6, capturyPose = 7,
	       capturyDaySessionShot = 8, capturySetShot = 9, capturySetShotAck = 10,
	       capturyStartRecording = 11, capturyStartRecordingAck = 12,
	       capturyStopRecording = 13, capturyStopRecordingAck = 14,
	       capturyConstraint = 15,
	       capturyGetTime = 16, capturyTime = 17,
	       capturyCustom = 18, capturyCustomAck = 19,
	       capturyGetImage = 20, capturyImageHeader = 21, capturyImageData = 22,
	       capturyGetImageData = 23,
	       capturyActorContinued = 24,
	       capturyGetMarkerTransform = 25, capturyMarkerTransform = 26,
	       capturyGetScalingProgress = 27, capturyScalingProgress = 28,
	       capturyConstraintAck = 29,
	       capturySnapActor = 30, capturyStopTracking = 31, capturyDeleteActor = 32,
	       capturySnapActorAck = 33, capturyStopTrackingAck = 34, capturyDeleteActorAck = 35,
	       capturyActorModeChanged = 36, capturyARTag = 37,
	       capturyGetBackgroundQuality = 38, capturyBackgroundQuality = 39,
	       capturyCaptureBackground = 40, capturyCaptureBackgroundAck = 41, capturyBackgroundFinished = 42,
	       capturySetActorName = 43, capturySetActorNameAck = 44,
	       capturyStreamedImageHeader = 45, capturyStreamedImageData = 46,
	       capturyGetStreamedImageData = 47, capturyRescaleActor = 48, capturyRecolorActor = 49,
	       capturyRescaleActorAck = 50, capturyRecolorActorAck = 51,
	       capturyStartTracking = 52, capturyStartTrackingAck = 53,
	       capturyPose2 = 54,
	       capturyGetStatus = 55, capturyStatus = 56,
	       capturyUpdateActorColors = 57,
	       capturyPoseCont = 58,
	       capturyActor2 = 59, capturyActorContinued2 = 60,
	       capturyLatency = 61,
	       capturyActors2 = 62, capturyActor3 = 63, capturyActorContinued3 = 64,
	       capturyCompressedPose = 65, capturyCompressedPose2 = 66,
	       capturyCompressedPoseCont = 67,
	       capturyGetTime2 = 68, capturyTime2 = 69,
	       capturyAngles = 70,
	       capturyStartRecording2 = 71, capturyStartRecordingAck2 = 72,
	       capturyHello = 73, // handshake finished
	       capturyActorBlendShapes = 74,
	       capturyMessage = 75,
	       capturyEnableRemoteLogging = 76,
	       capturyDisableRemoteLogging = 77,
	       capturyGetFramerate = 78,
	       capturyFramerate = 79,
	       CapturyBoneTypes = 80,
	       capturyError = 0 } CapturyPacketTypes;

// returns a string for nicer error messages
const char* Captury_getHumanReadableMessageType(CapturyPacketTypes type);

// make sure structures are laid out without padding
#pragma pack(push, 1)

// sent to server
struct CapturyRequestPacket {
	int32_t		type;		// from capturyActors, capturyCameras, capturyDaySessionShot, capturySetShot, capturyStartRecording, capturyStopRecording
	int32_t		size;		// size of full message including type and size
};

// sent to client
// as a reply to CapturyRequestPacket = capturyActors
struct CapturyActorsPacket {
	int32_t		type;	// capturyActors
	int32_t		size;	// size of full message including type and size

	int32_t		numActors;
};

// sent to client
// part of CapturyActorPacket
struct CapturyJointPacket {
	char		name[24];
	int32_t		parent;
	float		offset[3];
	float		orientation[3];
};

// sent to client
// part of CapturyActorPacket
struct CapturyJointPacket2 {
	int32_t		parent;
	float		offset[3];
	float		orientation[3];
	char		name[];		// zero terminated joint name
};

// sent to client
// part of CapturyActorPacket
struct CapturyJointPacket3 {
	int32_t		parent;
	float		offset[3];
	float		orientation[3];
	float		scale[3];	// if scale[0] == -1: this is a blend shape
	char		name[];		// zero terminated joint name
};

// sent to client
// as a reply to CapturyRequestPacket = capturyActors
struct CapturyActorPacket {
	int32_t		type;		// capturyActor or capturyActor2 or capturyActor3
	int32_t		size;		// size of full message including type and size

	char		name[32];
	int32_t		id;
	int32_t		numJoints;
	CapturyJointPacket	joints[];
};

// sent to client
// as a reply to CapturyRequestPacket = capturyActors
struct CapturyActorBlendShapesPacket {
	int32_t		type;		// capturyActorBlendShapes
	int32_t		size;		// size of full message including type and size

	int32_t		actorId;
	int32_t		numBlendShapes;
	char		blendShapeNames[];
};

// sent to client
// as a reply to CapturyRequestPacket = capturyActors
// if the CapturyActorPacket becomes too big send this one
struct CapturyActorContinuedPacket {
	int32_t		type;		// capturyActorContinued
	int32_t		size;		// size of full message including type and size

	int32_t		id;		// actor id
	int32_t		startJoint;
	CapturyJointPacket	joints[];
};

// sent to client
// as a reply to CapturyRequestPacket = capturyCameras
struct CapturyCamerasPacket {
	int32_t		type;	// capturyCameras
	int32_t		size;	// size of full message including type and size

	int32_t		numCameras;
};

// sent to client
// as a reply to CapturyRequestPacket = capturyCamera
struct CapturyCameraPacket {
	int32_t		type;	// capturyCamera
	int32_t		size;	// size of full message including type and size

	char		name[32];
	int32_t		id;
	float		position[3];
	float		orientation[3];
	float		sensorSize[2];	// in mm
	float		focalLength;	// in mm
	float		lensCenter[2];	// in mm
};

// sent to server - old version needs to stay around for old clients
struct CapturyStreamPacket0 {
	int32_t		type;		// capturyStream
	int32_t		size;		// size of full message including type and size

	int32_t		what;		// CAPTURY_STREAM_POSES or CAPTURY_STREAM_NOTHING
};

// sent to server
struct CapturyStreamPacket {
	int32_t		type;		// capturyStream
	int32_t		size;		// size of full message including type and size

	int32_t		what;		// CAPTURY_STREAM_POSES or CAPTURY_STREAM_NOTHING
	int32_t		cameraId;	// valid if what & CAPTURY_STREAM_IMAGES
};

// sent to server
struct CapturyStreamPacket1 {
	int32_t		type;		// capturyStream
	int32_t		size;		// size of full message including type and size

	int32_t		what;		// CAPTURY_STREAM_POSES or CAPTURY_STREAM_NOTHING
	int32_t		cameraId;	// valid if what & CAPTURY_STREAM_IMAGES
	uint16_t	numAngles;
	uint16_t	angles[];
};

// sent to server
struct CapturyStreamPacketTcp {
	int32_t		type;		// capturyStream
	int32_t		size;		// size of full message including type and size

	int32_t		what;		// CAPTURY_STREAM_POSES or CAPTURY_STREAM_NOTHING
	int32_t		cameraId;	// valid if what & CAPTURY_STREAM_IMAGES

	uint32_t	ip;		// where to stream to
	uint16_t	port;
};

// sent to server
struct CapturyStreamPacket1Tcp {
	int32_t		type;		// capturyStream
	int32_t		size;		// size of full message including type and size

	int32_t		what;		// CAPTURY_STREAM_POSES or CAPTURY_STREAM_NOTHING
	int32_t		cameraId;	// valid if what & CAPTURY_STREAM_IMAGES

	uint32_t	ip;		// where to stream to
	uint16_t	port;

	uint16_t	numAngles;
	uint16_t	angles[];
};

// sent to client
// as a reply to CapturyRequestPacket = capturyDaySessionShot
struct CapturyDaySessionShotPacket {
	int32_t		type;	// capturyDaySessionShot
	int32_t		size;	// size of full message including type and size

	char		day[100];
	char		session[100];
	char		shot[100];
};

// sent to server
struct CapturySetShotPacket {
	int32_t		type;	// capturySetShot
	int32_t		size;	// size of full message including type and size

	char		shot[100];
};

// sent to client
// as a reply to CapturyStreamPacket
struct CapturyPosePacket {
	int32_t		type;	// capturyPose
	int32_t		size;	// size of full message including type and size

	int32_t		actor;
	uint64_t	timestamp;
	int32_t		numValues; // 6 * numJoints + numBlendShapes + (numJoints if scale is enabled)
	float		values[];
};

// sent to client
// as a reply to CapturyStreamPacket
struct CapturyPosePacket2 {
	int32_t		type;	// capturyPose2
	int32_t		size;	// size of full message including type and size

	int32_t		actor;
	uint64_t	timestamp;
	uint8_t		trackingQuality; // [0 .. 100]
	uint8_t		scalingProgress; // [0 .. 100]
	uint8_t		flags;     // CAPTURY_LEFT_FOOT_ON_GROUND | CAPTURY_RIGHT_FOOT_ON_GROUND
	uint8_t		reserved;  // 0 for now
	int32_t		numValues; // 6 * numJoints + numBlendShapes + (numJoints if scale is enabled)
	float		values[];
};

struct CapturyPoseCont {
	int32_t		type;	// capturyPoseCont
	int32_t		size;	// size of full message including type and size

	int32_t		actor;
	uint64_t	timestamp;
	float		values[];
};

// sent to client
struct CapturyAnglesPacket {
	int32_t		type;	// capturyAngles
	int32_t		size;

	int32_t		actor;
	uint64_t	timestamp;
	uint16_t	numAngles;
	CapturyAngleData angles[];
};

// sent to server
struct CapturyConstraintPacket {
	int32_t		type;	// capturyConstraint
	int32_t		size;

	int32_t		constrType;

	int32_t		originActor;
	int32_t		originJoint;
	float		originOffset[3];

	int32_t		targetActor;
	int32_t		targetJoint;
	float		targetOffset[3];

	float		targetVector[3];
	float		targetValue;
	float		targetRotation[4];

	float		weight;
};

// sent to client
// as a reply to capturyGetTime
struct CapturyTimePacket {
	int32_t		type;	// capturyTime, capturyStartRecordingAck2
	int32_t		size;

	uint64_t	timestamp;
};

// sent to client
// as a reply to capturyGetTime2
struct CapturyTimePacket2 {
	int32_t		type;	// capturyGetTime2, capturyTime2
	int32_t		size;

	uint64_t	timestamp;

	int32_t		timeId; // set by remote client and repeated by server
};

// sent to server
struct CapturyGetMarkerTransformPacket {
	int32_t		type;	// capturyGetMarkerTransform
	int32_t		size;

	int32_t		actor;
	int32_t		joint;
};

// sent to client
// as a reply to capturyGetMarkerTransform
struct CapturyMarkerTransformPacket {
	int32_t		type;	// capturyMarkerTransform
	int32_t		size;

	uint64_t	timestamp;

	int32_t		actor;
	int32_t		joint;

	float		rotation[3]; // XYZ Euler angles
	float		translation[3];
};

// sent to server
struct CapturyGetScalingProgressPacket {
	int32_t		type;	// capturyGetScalingProgress
	int32_t		size;

	int32_t		actor;
};

// sent to client
// as a reply to capturyGetScalingProgress
struct CapturyScalingProgressPacket {
	int32_t		type;		// capturyScalingProgress
	int32_t		size;

	int32_t		actor;
	int8_t		progress;	// value from 0 to 100
};

// sent in both directions
// a CapturyRequestPacket = capturyCustomAck is always sent in reply
struct CapturyCustomPacket {
	int32_t		type;	// capturyCustom
	int32_t		size;

	char		name[16];
	char		data[];
};

// sent to server
struct CapturyGetImagePacket {
	int32_t		type;	// capturyGetImage
	int32_t		size;

	int32_t		actor;
};

struct CapturyGetImageDataPacket {
	int32_t		type;	// capturyGetImageData
	int32_t		size;

	uint16_t	port;
	int32_t		actor;
};

// sent to client
// as a reply to capturyGetImage or for streamed cameras
//
struct CapturyImageHeaderPacket {
	int32_t		type;		// capturyImageHeader, capturyStreamedImageHeader
	int32_t		size;

	int32_t		actor;		// for capturyStreamedImageHeader this is the camera id
	uint64_t	timestamp;
	uint32_t	fourcc;		// image compression format
	int32_t		width;
	int32_t		height;
	int32_t		dataPacketSize;	// size of data packets
	int32_t		dataSize;
};

struct CapturyImageDataPacket {
	int32_t		type;	// capturyImageData, capturyStreamedImageData
	int32_t		size;

	int32_t		actor;	// for capturyStreamedImageData this is the camera id
	int32_t		offset;	// offset in bytes into the following data (0 for first packet)
	unsigned char	data[];	// width*height*3 bytes
};

struct CapturySnapActorPacket {
	int32_t		type; // capturySnapActor
	int32_t		size;
	float		x;
	float		z;
	float		heading;
};

struct CapturySnapActorPacket2 {
	int32_t		type; // capturySnapActor
	int32_t		size;
	float		x;
	float		z;
	float		radius;
	float		heading;
	uint8_t		snapMethod;
	uint8_t		quickScaling;
	char		skeletonName[32];
};

struct CapturyStartTrackingPacket {
	int32_t		type; // capturyStartTracking
	int32_t		size;

	int32_t		actor;
	float		x;
	float		z;
	float		heading;
};

struct CapturyStopTrackingPacket {
	int32_t		type; // capturyStopTracking or capturyDeleteActor or capturyRescaleActor or capturyRecolorActor
	int32_t		size;
	int32_t		actor;
};

struct CapturyActorModeChangedPacket {
	int32_t		type; // capturyActorModeChanged
	int32_t		size;
	int32_t		actor;
	int32_t		mode;
};

struct CapturyARTagPacket {
	int32_t		type; // capturyARTag
	int32_t		size;
	int32_t		numTags;

	CapturyARTag	tags[1];
};

struct CapturyBackgroundQualityPacket {
	int32_t		type; // capturyBackgroundQuality
	int32_t		size;
	int32_t		quality;
};

struct CapturySetActorNamePacket {
	int32_t		type; // capturySetActorName
	int32_t		size;
	int32_t		actor;
	char		name[32];
};

struct CapturyStatusPacket {
	int32_t		type; // capturyStatus
	int32_t		size;
	char		message[1]; // 0-terminated
};

// sent to client
// as a reply to CapturyStreamPacket
struct CapturyIMUData {
	int32_t		type;	// capturyIMU
	int32_t		size;	// size of full message including type and size

	uint8_t		numIMUs;
	float		eulerAngles[]; // 3x numIMUs floats
};

// sent to client
// as a reply to CapturyStreamPacket
struct CapturyLatencyPacket {
	int32_t		type;	// capturyLatency
	int32_t		size;	// size of full message including type and size

	uint64_t	firstImagePacket;
	uint64_t	optimizationStart;
	uint64_t	optimizationEnd;
	uint64_t	sendPacketTime; // right before packet is sent

	uint64_t	poseTimestamp;	// timestamp of corresponding pose
};


// sent to server
struct CapturyLogPacket {
	int32_t		type;	// capturyMessage
	int32_t		size;	// size of full message including type and size

	uint8_t		logLevel;
	char		message[];
};

// sent to client
struct CapturyFrameratePacket {
	int32_t		type;	// capturyFramerate
	int32_t		size;	// size of full message including type and size

	int		numerator;
	int		denominator;
};

// sent to client
struct CapturyBoneTypePacket {
	int32_t		type;	// capturyBoneTypes
	int32_t		size;	// size of full message including type and size

	int32_t		actorId;
	uint8_t		boneTypes[];
};

#pragma pack(pop)

#ifndef FOURCC
#define FOURCC(a,b,c,d)		(((d)<<24)|((c)<<16)|((b)<<8)|(a))
#endif

#define FOURCC_RGB		FOURCC('2','4',' ',' ') // uncompressed RGB

#ifdef __cplusplus
} // extern "C"
#endif
