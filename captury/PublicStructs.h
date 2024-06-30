#pragma once

#include <stdint.h>

#pragma pack(push, 1)

struct CapturyJoint {
	char		name[64];
	int32_t		parent;		// index of parent joint or -1 for root node
	float		offset[3];	// offset to parent joint
	float		orientation[3];	// XYZ quaternion - w needs to be reconstructed (w >= 0)
	float		scale[3];	// local scale
};

struct CapturyBlendShape {
	char		name[64];
};

struct CapturyBlob {
	int32_t		parent;		// index of parent joint or -1 for root node
	float		offset[3];	// offset to parent joint
	float		size;		// radius
	float		color[3];	// RGB color [0..1]
};

struct CapturyActor {
	char		name[32];
	int32_t		id;
	int32_t		numJoints;
	CapturyJoint*	joints;
	int32_t		numBlobs;
	CapturyBlob*	blobs;
	int32_t		numBlendShapes;
	CapturyBlendShape* blendShapes;
};

struct CapturyTransform {
	float		translation[3];	// translation
	float		rotation[3];	// XYZ Euler angles
};

enum CapturyPoseFlags {CAPTURY_LEFT_FOOT_ON_GROUND = 0x01, CAPTURY_RIGHT_FOOT_ON_GROUND = 0x02};

struct CapturyPose {
	int32_t			actor;
	uint64_t		timestamp;	// in microseconds - since the start of Captury Live
	int32_t			numTransforms;
	CapturyTransform*	transforms;	// one CapturyTransform per joint in global (world space) coordinates
						// the transforms are in the same order as the joints
						// in the corresponding CapturyActor.joints array
	uint32_t		flags;		// feet-on-ground
	int32_t			numBlendShapes;
	float*			blendShapeActivations;
};

struct CapturyIMUPose {
	uint32_t		imu;
	uint64_t		timestamp;	// in microseconds - since the start of Captury Live
	float			orientation[3];
	float			acceleration[3];// linear acceleration
	float			position[3];
};

struct CapturyConstraint {
	int32_t		actor;
	char		jointName[24];
	CapturyTransform transform;
	float		weight;
	char		constrainTranslation;	// if 0 ignore the translational part of the transform
	char		constrainRotation;	// if 0 ignore the rotational part of the transform
};

struct CapturyCamera {
	char		name[32];
	int32_t		id;
	float		position[3];
	float		orientation[3];
	float		sensorSize[2];	// in mm
	float		focalLength;	// in mm
	float		lensCenter[2];	// in mm
	char		distortionModel[16]; // name of distortion model or "none"
	float		distortion[30];

	// the following can be computed from the above values and are provided for convenience only
	// the matrices are stored column wise:
	// 0  3  6  9
	// 1  4  7 10
	// 2  5  8 11
	float		extrinsic[12];
	float		intrinsic[9];
};

struct CapturyImage {
	int32_t		width;
	int32_t		height;
	int32_t		camera;		// camera index
	uint64_t	timestamp;	// in microseconds
	uint8_t*	data;		// packed image data: stride = width*3 bytes
					// data is expected to be formatted as RGB
	uint8_t*	gpuData;
};

struct CapturyDepthImage {
	int32_t		width;
	int32_t		height;
	int32_t		camera;
	uint64_t	timestamp;	// in microseconds
	uint16_t*	data;		// packed image data: stride = width*2 bytes
};

struct CapturyARTag {
	int32_t		id;

	CapturyTransform transform;
};

struct CapturyCornerDetection {
	int32_t		camera;
	float		position[2];
	float		boardCoordinates[3];
};

struct CapturyLatencyInfo {
	uint64_t	firstImagePacketTime;
	uint64_t	optimizationStartTime;
	uint64_t	optimizationEndTime;
	uint64_t	poseSentTime;
	uint64_t	poseReceivedTime;

	uint64_t	timestampOfCorrespondingPose;
};

#pragma pack(pop)
