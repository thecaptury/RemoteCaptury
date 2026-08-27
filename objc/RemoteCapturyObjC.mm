#import "RemoteCapturyObjC.h"
#include <vector>

#if !defined(dispatch_async) && !defined(__APPLE__)
#define dispatch_async(queue, block) \
	@autoreleasepool {           \
		block();             \
	}
#define dispatch_get_main_queue() 0
#endif

#pragma mark - Model Implementations

@implementation CapturyTransformObjC
@synthesize translationX = _translationX;
@synthesize translationY = _translationY;
@synthesize translationZ = _translationZ;
@synthesize rotationX = _rotationX;
@synthesize rotationY = _rotationY;
@synthesize rotationZ = _rotationZ;

- (instancetype)initWithCapturyTransform:(CapturyTransform)transform {
	self = [super init];
	if (self) {
		_translationX = transform.translation[0];
		_translationY = transform.translation[1];
		_translationZ = transform.translation[2];
		_rotationX = transform.rotation[0];
		_rotationY = transform.rotation[1];
		_rotationZ = transform.rotation[2];
	}
	return self;
}

- (CapturyTransform)toCapturyTransform {
	CapturyTransform transform;
	transform.translation[0] = _translationX;
	transform.translation[1] = _translationY;
	transform.translation[2] = _translationZ;
	transform.rotation[0] = _rotationX;
	transform.rotation[1] = _rotationY;
	transform.rotation[2] = _rotationZ;
	return transform;
}

@end

@implementation CapturyJointObjC
@synthesize name = _name;
@synthesize parent = _parent;
@synthesize offsetX = _offsetX;
@synthesize offsetY = _offsetY;
@synthesize offsetZ = _offsetZ;
@synthesize orientationX = _orientationX;
@synthesize orientationY = _orientationY;
@synthesize orientationZ = _orientationZ;
@synthesize scaleX = _scaleX;
@synthesize scaleY = _scaleY;
@synthesize scaleZ = _scaleZ;
@synthesize boneType = _boneType;

- (instancetype)initWithCapturyJoint:(CapturyJoint)joint {
	self = [super init];
	if (self) {
		_name = [NSString stringWithUTF8String:joint.name] ?: @"";
		_parent = joint.parent;
		_offsetX = joint.offset[0];
		_offsetY = joint.offset[1];
		_offsetZ = joint.offset[2];
		_orientationX = joint.orientation[0];
		_orientationY = joint.orientation[1];
		_orientationZ = joint.orientation[2];
		_scaleX = joint.scale[0];
		_scaleY = joint.scale[1];
		_scaleZ = joint.scale[2];
		_boneType = joint.boneType;
	}
	return self;
}

@end

@implementation CapturyBlendShapeObjC
@synthesize name = _name;

- (instancetype)initWithName:(NSString *)name {
	self = [super init];
	if (self) {
		_name = [name copy];
	}
	return self;
}

@end

@implementation CapturyBlobObjC
@synthesize parent = _parent;
@synthesize offsetX = _offsetX;
@synthesize offsetY = _offsetY;
@synthesize offsetZ = _offsetZ;
@synthesize size = _size;
@synthesize colorR = _colorR;
@synthesize colorG = _colorG;
@synthesize colorB = _colorB;

- (instancetype)initWithCapturyBlob:(CapturyBlob)blob {
	self = [super init];
	if (self) {
		_parent = blob.parent;
		_offsetX = blob.offset[0];
		_offsetY = blob.offset[1];
		_offsetZ = blob.offset[2];
		_size = blob.size;
		_colorR = blob.color[0];
		_colorG = blob.color[1];
		_colorB = blob.color[2];
	}
	return self;
}

@end

@implementation CapturyActorObjC
@synthesize name = _name;
@synthesize actorId = _actorId;
@synthesize joints = _joints;
@synthesize blobs = _blobs;
@synthesize blendShapes = _blendShapes;
@synthesize metaData = _metaData;

- (instancetype)initWithCapturyActor:(const CapturyActor *)actor {
	self = [super init];
	if (self && actor != NULL) {
		_name = [NSString stringWithUTF8String:actor->name] ?: @"";
		_actorId = actor->id;

		NSMutableArray<CapturyJointObjC *> *jointsArray = [NSMutableArray arrayWithCapacity:actor->numJoints];
		for (int i = 0; i < actor->numJoints; ++i) {
			[jointsArray addObject:[[CapturyJointObjC alloc] initWithCapturyJoint:actor->joints[i]]];
		}
		_joints = [jointsArray copy];

		NSMutableArray<CapturyBlobObjC *> *blobsArray = [NSMutableArray arrayWithCapacity:actor->numBlobs];
		for (int i = 0; i < actor->numBlobs; ++i) {
			[blobsArray addObject:[[CapturyBlobObjC alloc] initWithCapturyBlob:actor->blobs[i]]];
		}
		_blobs = [blobsArray copy];

		NSMutableArray<CapturyBlendShapeObjC *> *blendShapesArray = [NSMutableArray arrayWithCapacity:actor->numBlendShapes];
		for (int i = 0; i < actor->numBlendShapes; ++i) {
			NSString *bsName = [NSString stringWithUTF8String:actor->blendShapes[i].name] ?: @"";
			[blendShapesArray addObject:[[CapturyBlendShapeObjC alloc] initWithName:bsName]];
		}
		_blendShapes = [blendShapesArray copy];

		NSMutableDictionary<NSString *, NSString *> *metaDict = [NSMutableDictionary dictionaryWithCapacity:actor->numMetaData];
		for (int i = 0; i < actor->numMetaData; ++i) {
			if (actor->metaDataKeys && actor->metaDataValues && actor->metaDataKeys[i] && actor->metaDataValues[i]) {
				NSString *key = [NSString stringWithUTF8String:actor->metaDataKeys[i]];
				NSString *val = [NSString stringWithUTF8String:actor->metaDataValues[i]];
				if (key && val) {
					[metaDict setObject:val forKey:key];
				}
			}
		}
		_metaData = [metaDict copy];
	}
	return self;
}

@end

@implementation CapturyCameraObjC
@synthesize name = _name;
@synthesize cameraId = _cameraId;
@synthesize positionX = _positionX;
@synthesize positionY = _positionY;
@synthesize positionZ = _positionZ;
@synthesize orientationX = _orientationX;
@synthesize orientationY = _orientationY;
@synthesize orientationZ = _orientationZ;
@synthesize sensorSizeWidth = _sensorSizeWidth;
@synthesize sensorSizeHeight = _sensorSizeHeight;
@synthesize focalLength = _focalLength;
@synthesize lensCenterX = _lensCenterX;
@synthesize lensCenterY = _lensCenterY;
@synthesize distortionModel = _distortionModel;

- (instancetype)initWithCapturyCamera:(const CapturyCamera *)camera {
	self = [super init];
	if (self && camera != NULL) {
		_name = [NSString stringWithUTF8String:camera->name] ?: @"";
		_cameraId = camera->id;
		_positionX = camera->position[0];
		_positionY = camera->position[1];
		_positionZ = camera->position[2];
		_orientationX = camera->orientation[0];
		_orientationY = camera->orientation[1];
		_orientationZ = camera->orientation[2];
		_sensorSizeWidth = camera->sensorSize[0];
		_sensorSizeHeight = camera->sensorSize[1];
		_focalLength = camera->focalLength;
		_lensCenterX = camera->lensCenter[0];
		_lensCenterY = camera->lensCenter[1];
		_distortionModel = [NSString stringWithUTF8String:camera->distortionModel] ?: @"none";
	}
	return self;
}

@end

@implementation CapturyPoseObjC
@synthesize actorId = _actorId;
@synthesize timestamp = _timestamp;
@synthesize transforms = _transforms;
@synthesize flags = _flags;
@synthesize blendShapeActivations = _blendShapeActivations;

- (instancetype)initWithCapturyPose:(const CapturyPose *)pose {
	self = [super init];
	if (self && pose != NULL) {
		_actorId = pose->actor;
		_timestamp = pose->timestamp;
		_flags = (CapturyPoseFlagsObjC)pose->flags;

		NSMutableArray<CapturyTransformObjC *> *trafoArray = [NSMutableArray arrayWithCapacity:pose->numTransforms];
		for (int i = 0; i < pose->numTransforms; ++i) {
			[trafoArray addObject:[[CapturyTransformObjC alloc] initWithCapturyTransform:pose->transforms[i]]];
		}
		_transforms = [trafoArray copy];

		NSMutableArray<NSNumber *> *activations = [NSMutableArray arrayWithCapacity:pose->numBlendShapes];
		for (int i = 0; i < pose->numBlendShapes; ++i) {
			[activations addObject:@(pose->blendShapeActivations[i])];
		}
		_blendShapeActivations = [activations copy];
	}
	return self;
}

@end

@implementation CapturyAngleDataObjC
@synthesize type = _type;
@synthesize value = _value;

- (instancetype)initWithAngleData:(CapturyAngleData)angleData {
	self = [super init];
	if (self) {
		_type = angleData.type;
		_value = angleData.value;
	}
	return self;
}

@end

@implementation CapturyARTagObjC
@synthesize tagId = _tagId;
@synthesize transform = _transform;

- (instancetype)initWithCapturyARTag:(CapturyARTag)artag {
	self = [super init];
	if (self) {
		_tagId = artag.id;
		_transform = [[CapturyTransformObjC alloc] initWithCapturyTransform:artag.transform];
	}
	return self;
}

@end

@implementation CapturyImageObjC
@synthesize width = _width;
@synthesize height = _height;
@synthesize cameraId = _cameraId;
@synthesize timestamp = _timestamp;
@synthesize imageData = _imageData;

- (instancetype)initWithCapturyImage:(const CapturyImage *)image {
	self = [super init];
	if (self && image != NULL) {
		_width = image->width;
		_height = image->height;
		_cameraId = image->camera;
		_timestamp = image->timestamp;
		if (image->data != NULL) {
			_imageData = [NSData dataWithBytes:image->data length:_width * _height * 3];
		} else {
			_imageData = [NSData data];
		}
	}
	return self;
}

@end

@implementation CapturyLatencyInfoObjC
@synthesize firstImagePacketTime = _firstImagePacketTime;
@synthesize optimizationStartTime = _optimizationStartTime;
@synthesize optimizationEndTime = _optimizationEndTime;
@synthesize poseSentTime = _poseSentTime;
@synthesize poseReceivedTime = _poseReceivedTime;
@synthesize timestampOfCorrespondingPose = _timestampOfCorrespondingPose;

- (instancetype)initWithLatencyInfo:(CapturyLatencyInfo)info {
	self = [super init];
	if (self) {
		_firstImagePacketTime = info.firstImagePacketTime;
		_optimizationStartTime = info.optimizationStartTime;
		_optimizationEndTime = info.optimizationEndTime;
		_poseSentTime = info.poseSentTime;
		_poseReceivedTime = info.poseReceivedTime;
		_timestampOfCorrespondingPose = info.timestampOfCorrespondingPose;
	}
	return self;
}

@end

#pragma mark - CapturyRemote Implementation & Private Bridge

@interface CapturyRemote ()

@end

// C Callbacks bridging to Objective-C instances
static void objcPoseCallback(RemoteCaptury *rc, CapturyActor *actor, CapturyPose *pose, int trackingQuality, void *userArg) {
	CapturyRemote *remote = (__bridge CapturyRemote *)userArg;
	if (!remote)
		return;

	dispatch_async(dispatch_get_main_queue(), ^{
		CapturyPoseObjC *poseObjC = pose ? [[CapturyPoseObjC alloc] initWithCapturyPose:pose] : nil;
		CapturyActorObjC *actorObjC = actor ? [[CapturyActorObjC alloc] initWithCapturyActor:actor] : nil;

		if (remote->_poseBlock) {
			remote->_poseBlock(actorObjC, poseObjC, trackingQuality);
		}
		if ([remote.delegate respondsToSelector:@selector(capturyRemote:didReceivePose:actor:trackingQuality:)]) {
			[remote.delegate capturyRemote:remote didReceivePose:poseObjC actor:actorObjC trackingQuality:trackingQuality];
		}
	});
}

static void objcAnglesCallback(RemoteCaptury *rc, const CapturyActor *actor, int numAngles, struct CapturyAngleData *values, void *userArg) {
	CapturyRemote *remote = (__bridge CapturyRemote *)userArg;
	if (!remote)
		return;

	dispatch_async(dispatch_get_main_queue(), ^{
		CapturyActorObjC *actorObjC = actor ? [[CapturyActorObjC alloc] initWithCapturyActor:actor] : nil;
		NSMutableArray<CapturyAngleDataObjC *> *anglesArray = [NSMutableArray arrayWithCapacity:numAngles];
		for (int i = 0; i < numAngles; ++i) {
			[anglesArray addObject:[[CapturyAngleDataObjC alloc] initWithAngleData:values[i]]];
		}
		NSArray<CapturyAngleDataObjC *> *resultAngles = [anglesArray copy];

		if (remote->_anglesBlock) {
			remote->_anglesBlock(actorObjC, resultAngles);
		}
		if ([remote.delegate respondsToSelector:@selector(capturyRemote:didReceiveAngles:forActor:)]) {
			[remote.delegate capturyRemote:remote didReceiveAngles:resultAngles forActor:actorObjC];
		}
	});
}

static void objcActorChangedCallback(RemoteCaptury *rc, int actorId, int mode, void *userArg) {
	CapturyRemote *remote = (__bridge CapturyRemote *)userArg;
	if (!remote)
		return;

	CapturyActorStatusObjC status = (CapturyActorStatusObjC)mode;

	dispatch_async(dispatch_get_main_queue(), ^{
		if (remote->_actorChangedBlock) {
			remote->_actorChangedBlock(actorId, status);
		}
		if ([remote.delegate respondsToSelector:@selector(capturyRemote:actorStatusChanged:status:)]) {
			[remote.delegate capturyRemote:remote actorStatusChanged:actorId status:status];
		}
	});
}

static void objcARTagCallback(RemoteCaptury *rc, int num, CapturyARTag *tags, void *userArg) {
	CapturyRemote *remote = (__bridge CapturyRemote *)userArg;
	if (!remote)
		return;

	dispatch_async(dispatch_get_main_queue(), ^{
		NSMutableArray<CapturyARTagObjC *> *tagsArray = [NSMutableArray arrayWithCapacity:num];
		for (int i = 0; i < num; ++i) {
			[tagsArray addObject:[[CapturyARTagObjC alloc] initWithCapturyARTag:tags[i]]];
		}
		NSArray<CapturyARTagObjC *> *resultTags = [tagsArray copy];

		if (remote->_artagBlock) {
			remote->_artagBlock(resultTags);
		}
		if ([remote.delegate respondsToSelector:@selector(capturyRemote:didReceiveARTags:)]) {
			[remote.delegate capturyRemote:remote didReceiveARTags:resultTags];
		}
	});
}

static void objcImageCallback(RemoteCaptury *rc, const CapturyImage *img, void *userArg) {
	CapturyRemote *remote = (__bridge CapturyRemote *)userArg;
	if (!remote)
		return;

	dispatch_async(dispatch_get_main_queue(), ^{
		CapturyImageObjC *imgObjC = img ? [[CapturyImageObjC alloc] initWithCapturyImage:img] : nil;

		if (remote->_imageBlock) {
			remote->_imageBlock(imgObjC);
		}
		if ([remote.delegate respondsToSelector:@selector(capturyRemote:didReceiveStreamedImage:)]) {
			[remote.delegate capturyRemote:remote didReceiveStreamedImage:imgObjC];
		}
	});
}

static void objcLogCallback(int logLevel, const char *msg, void *userArg) {
	CapturyRemote *remote = (__bridge CapturyRemote *)userArg;
	if (!remote)
		return;

	dispatch_async(dispatch_get_main_queue(), ^{
		NSString *message = msg ? [NSString stringWithUTF8String:msg] : @"";
		if (remote->_logBlock) {
			remote->_logBlock(logLevel, message);
		}
		if ([remote.delegate respondsToSelector:@selector(capturyRemote:didReceiveLogMessage:level:)]) {
			[remote.delegate capturyRemote:remote didReceiveLogMessage:message level:logLevel];
		}
	});
}

static void objcBackgroundFinishedCallback(RemoteCaptury *rc, void *userData) {
	CapturyRemote *remote = (__bridge CapturyRemote *)userData;
	if (!remote)
		return;

	dispatch_async(dispatch_get_main_queue(), ^{
		if (remote->_bgFinishedBlock) {
			remote->_bgFinishedBlock();
		}
	});
}

@implementation CapturyRemote
@synthesize delegate = _delegate;

- (instancetype)init {
	self = [super init];
	if (self) {
		_rcHandle = Captury_create();
	}
	return self;
}

- (void)dealloc {
	if (_rcHandle) {
		Captury_destroy(_rcHandle);
		_rcHandle = NULL;
	}
}

#pragma mark - Connection

- (BOOL)connectToHost:(NSString *)ip port:(unsigned short)port {
	if (!_rcHandle)
		return NO;
	return Captury_connect(_rcHandle, [ip UTF8String], port) != 0;
}

- (BOOL)connectToHost:(NSString *)ip
		 port:(unsigned short)port
	    localPort:(unsigned short)localPort
      localStreamPort:(unsigned short)localStreamPort
		async:(BOOL)async
	 localAddress:(nullable NSString *)localAddress
     multicastAddress:(nullable NSString *)multicastAddress {
	if (!_rcHandle)
		return NO;
	return Captury_connect2(_rcHandle,
				[ip UTF8String],
				port,
				localPort,
				localStreamPort,
				async ? 1 : 0,
				localAddress ? [localAddress UTF8String] : NULL,
				multicastAddress ? [multicastAddress UTF8String] : NULL) != 0;
}

- (BOOL)disconnect {
	if (!_rcHandle)
		return NO;
	return Captury_disconnect(_rcHandle) != 0;
}

- (int)connectionStatus {
	if (!_rcHandle)
		return CAPTURY_DISCONNECTED;
	return Captury_getConnectionStatus(_rcHandle);
}

#pragma mark - Actors & Cameras

- (nullable NSArray<CapturyActorObjC *> *)fetchActors {
	if (!_rcHandle)
		return nil;
	const CapturyActor *actorsPtr = NULL;
	int count = Captury_getActors(_rcHandle, &actorsPtr);
	if (count <= 0 || actorsPtr == NULL)
		return @[];

	NSMutableArray<CapturyActorObjC *> *result = [NSMutableArray arrayWithCapacity:count];
	for (int i = 0; i < count; ++i) {
		[result addObject:[[CapturyActorObjC alloc] initWithCapturyActor:&actorsPtr[i]]];
	}
	Captury_freeActors(_rcHandle);
	return [result copy];
}

- (nullable CapturyActorObjC *)fetchActorWithId:(int32_t)actorId {
	if (!_rcHandle)
		return nil;
	const CapturyActor *actor = Captury_getActor(_rcHandle, actorId);
	if (!actor)
		return nil;
	CapturyActorObjC *result = [[CapturyActorObjC alloc] initWithCapturyActor:actor];
	Captury_freeActor(_rcHandle, actor);
	return result;
}

- (nullable NSArray<CapturyCameraObjC *> *)fetchCamerasWithWaitTimeMs:(int)waitTimeMs {
	if (!_rcHandle)
		return nil;
	const CapturyCamera *camerasPtr = NULL;
	int count = Captury_getCameras(_rcHandle, &camerasPtr, waitTimeMs);
	if (count <= 0 || camerasPtr == NULL)
		return @[];

	NSMutableArray<CapturyCameraObjC *> *result = [NSMutableArray arrayWithCapacity:count];
	for (int i = 0; i < count; ++i) {
		[result addObject:[[CapturyCameraObjC alloc] initWithCapturyCamera:&camerasPtr[i]]];
	}
	return [result copy];
}

#pragma mark - Streaming

- (BOOL)startStreaming:(CapturyStreamFlags)what {
	if (!_rcHandle)
		return NO;
	return Captury_startStreaming(_rcHandle, (int)what) != 0;
}

- (BOOL)startStreamingImages:(CapturyStreamFlags)what cameraId:(int32_t)cameraId {
	if (!_rcHandle)
		return NO;
	return Captury_startStreamingImages(_rcHandle, (int)what, cameraId) != 0;
}

- (BOOL)startStreamingImagesAndAngles:(CapturyStreamFlags)what cameraId:(int32_t)cameraId angles:(NSArray<NSNumber *> *)angles {
	if (!_rcHandle)
		return NO;
	NSUInteger numAngles = [angles count];
	std::vector<uint16_t> anglesVec(numAngles);
	for (NSUInteger i = 0; i < numAngles; ++i) {
		anglesVec[i] = [[angles objectAtIndex:i] unsignedShortValue];
	}
	return Captury_startStreamingImagesAndAngles(_rcHandle, (int)what, cameraId, (int)numAngles, anglesVec.data()) != 0;
}

- (BOOL)stopStreaming {
	return [self stopStreamingWait:YES];
}

- (BOOL)stopStreamingWait:(BOOL)wait {
	if (!_rcHandle)
		return NO;
	return Captury_stopStreaming(_rcHandle, wait ? 1 : 0) != 0;
}

#pragma mark - Poses & Polling Data

- (nullable CapturyPoseObjC *)currentPoseForActor:(int32_t)actorId {
	return [self currentPoseForActor:actorId trackingConsistency:NULL];
}

- (nullable CapturyPoseObjC *)currentPoseForActor:(int32_t)actorId trackingConsistency:(out int *_Nullable)tc {
	if (!_rcHandle)
		return nil;
	CapturyPose *pose = tc != NULL ? Captury_getCurrentPoseAndTrackingConsistencyForActor(_rcHandle, actorId, tc) : Captury_getCurrentPoseForActor(_rcHandle, actorId);
	if (!pose)
		return nil;
	CapturyPoseObjC *result = [[CapturyPoseObjC alloc] initWithCapturyPose:pose];
	Captury_freePose(pose);
	return result;
}

- (nullable NSArray<CapturyAngleDataObjC *> *)currentAnglesForActor:(int32_t)actorId {
	if (!_rcHandle)
		return nil;
	int numAngles = 0;
	CapturyAngleData *angles = Captury_getCurrentAngles(_rcHandle, actorId, &numAngles);
	if (!angles || numAngles <= 0)
		return @[];

	NSMutableArray<CapturyAngleDataObjC *> *result = [NSMutableArray arrayWithCapacity:numAngles];
	for (int i = 0; i < numAngles; ++i) {
		[result addObject:[[CapturyAngleDataObjC alloc] initWithAngleData:angles[i]]];
	}
	return [result copy];
}

- (nullable NSArray<CapturyARTagObjC *> *)currentARTags {
	if (!_rcHandle)
		return nil;
	CapturyARTag *tags = Captury_getCurrentARTags(_rcHandle);
	if (!tags)
		return @[];

	NSMutableArray<CapturyARTagObjC *> *result = [NSMutableArray array];
	int idx = 0;
	while (tags[idx].id != -1) {
		[result addObject:[[CapturyARTagObjC alloc] initWithCapturyARTag:tags[idx]]];
		idx++;
	}
	Captury_freeARTags(tags);
	return [result copy];
}

- (nullable CapturyImageObjC *)currentImage {
	if (!_rcHandle)
		return nil;
	CapturyImage *img = Captury_getCurrentImage(_rcHandle);
	if (!img)
		return nil;
	CapturyImageObjC *result = [[CapturyImageObjC alloc] initWithCapturyImage:img];
	Captury_freeImage(img);
	return result;
}

#pragma mark - Block Callbacks Registration

- (BOOL)registerPoseBlock:(nullable CapturyNewPoseBlock)block {
	if (!_rcHandle)
		return NO;
	if (_poseBlock)
		Block_release(_poseBlock);
	_poseBlock = block ? (CapturyNewPoseBlock)Block_copy(block) : (CapturyNewPoseBlock)NULL;
	return Captury_registerNewPoseCallback(_rcHandle, block ? objcPoseCallback : NULL, (__bridge void *)self) != 0;
}

- (BOOL)registerAnglesBlock:(nullable CapturyNewAnglesBlock)block {
	if (!_rcHandle)
		return NO;
	if (_anglesBlock)
		Block_release(_anglesBlock);
	_anglesBlock = block ? (CapturyNewAnglesBlock)Block_copy(block) : (CapturyNewAnglesBlock)NULL;
	return Captury_registerNewAnglesCallback(_rcHandle, block ? objcAnglesCallback : NULL, (__bridge void *)self) != 0;
}

- (BOOL)registerActorChangedBlock:(nullable CapturyActorChangedBlock)block {
	if (!_rcHandle)
		return NO;
	if (_actorChangedBlock)
		Block_release(_actorChangedBlock);
	_actorChangedBlock = block ? (CapturyActorChangedBlock)Block_copy(block) : (CapturyActorChangedBlock)NULL;
	return Captury_registerActorChangedCallback(_rcHandle, block ? objcActorChangedCallback : NULL, (__bridge void *)self) != 0;
}

- (BOOL)registerARTagBlock:(nullable CapturyARTagBlock)block {
	if (!_rcHandle)
		return NO;
	if (_artagBlock)
		Block_release(_artagBlock);
	_artagBlock = block ? (CapturyARTagBlock)Block_copy(block) : (CapturyARTagBlock)NULL;
	return Captury_registerARTagCallback(_rcHandle, block ? objcARTagCallback : NULL, (__bridge void *)self) != 0;
}

- (BOOL)registerImageStreamingBlock:(nullable CapturyImageBlock)block {
	if (!_rcHandle)
		return NO;
	if (_imageBlock)
		Block_release(_imageBlock);
	_imageBlock = block ? (CapturyImageBlock)Block_copy(block) : (CapturyImageBlock)NULL;
	return Captury_registerImageStreamingCallback(_rcHandle, block ? objcImageCallback : NULL, (__bridge void *)self) != 0;
}

- (void)registerLogBlock:(nullable CapturyLogBlock)block {
	if (!_rcHandle)
		return;
	if (_logBlock)
		Block_release(_logBlock);
	_logBlock = block ? (CapturyLogBlock)Block_copy(block) : (CapturyLogBlock)NULL;
	Captury_registerLogCallback(_rcHandle, block ? objcLogCallback : NULL, (__bridge void *)self);
}

#pragma mark - Actor Operations

- (CapturyActorStatusObjC)actorStatusForId:(int32_t)actorId {
	if (!_rcHandle)
		return CapturyActorStatusUnknown;
	return (CapturyActorStatusObjC)Captury_getActorStatus(_rcHandle, actorId);
}

- (BOOL)requestTextureForActor:(int32_t)actorId {
	if (!_rcHandle)
		return NO;
	return Captury_requestTexture(_rcHandle, actorId) != 0;
}

- (nullable CapturyImageObjC *)textureForActor:(int32_t)actorId {
	if (!_rcHandle)
		return nil;
	CapturyImage *img = Captury_getTexture(_rcHandle, actorId);
	if (!img)
		return nil;
	CapturyImageObjC *result = [[CapturyImageObjC alloc] initWithCapturyImage:img];
	Captury_freeImage(img);
	return result;
}

- (uint64_t)markerTransformForActor:(int32_t)actorId joint:(int32_t)joint transform:(out CapturyTransformObjC *_Nullable *_Nullable)outTrafo {
	if (!_rcHandle)
		return 0;
	CapturyTransform trafo;
	uint64_t ts = Captury_getMarkerTransform(_rcHandle, actorId, joint, &trafo);
	if (ts != 0 && outTrafo != NULL) {
		*outTrafo = [[CapturyTransformObjC alloc] initWithCapturyTransform:trafo];
	}
	return ts;
}

- (int)scalingProgressForActor:(int32_t)actorId {
	if (!_rcHandle)
		return 0;
	return Captury_getScalingProgress(_rcHandle, actorId);
}

- (int)trackingQualityForActor:(int32_t)actorId {
	if (!_rcHandle)
		return 0;
	return Captury_getTrackingQuality(_rcHandle, actorId);
}

- (BOOL)setActorName:(NSString *)name forActor:(int32_t)actorId {
	if (!_rcHandle)
		return NO;
	return Captury_setActorName(_rcHandle, actorId, [name UTF8String]) != 0;
}

- (BOOL)snapActorAtX:(float)x z:(float)z heading:(float)heading {
	if (!_rcHandle)
		return NO;
	return Captury_snapActor(_rcHandle, x, z, heading) != 0;
}

- (BOOL)snapActorExAtX:(float)x z:(float)z radius:(float)radius heading:(float)heading skeletonName:(nullable NSString *)skeletonName snapMethod:(CapturySnapMethodObjC)snapMethod quickScaling:(BOOL)quickScaling {
	if (!_rcHandle)
		return NO;
	return Captury_snapActorEx(_rcHandle, x, z, radius, heading, skeletonName ? [skeletonName UTF8String] : NULL, (int)snapMethod, quickScaling ? 1 : 0) != 0;
}

- (BOOL)startTrackingActor:(int32_t)actorId atX:(float)x z:(float)z heading:(float)heading {
	if (!_rcHandle)
		return NO;
	return Captury_startTracking(_rcHandle, actorId, x, z, heading) != 0;
}

- (BOOL)stopTrackingActor:(int32_t)actorId {
	if (!_rcHandle)
		return NO;
	return Captury_stopTracking(_rcHandle, actorId) != 0;
}

- (BOOL)deleteActor:(int32_t)actorId {
	if (!_rcHandle)
		return NO;
	return Captury_deleteActor(_rcHandle, actorId) != 0;
}

- (BOOL)rescaleActor:(int32_t)actorId {
	if (!_rcHandle)
		return NO;
	return Captury_rescaleActor(_rcHandle, actorId) != 0;
}

- (BOOL)recolorActor:(int32_t)actorId {
	if (!_rcHandle)
		return NO;
	return Captury_recolorActor(_rcHandle, actorId) != 0;
}

- (BOOL)updateActorColors:(int32_t)actorId {
	if (!_rcHandle)
		return NO;
	return Captury_updateActorColors(_rcHandle, actorId) != 0;
}

#pragma mark - Time Synchronization

- (uint64_t)synchronizeTime {
	if (!_rcHandle)
		return 0;
	return Captury_synchronizeTime(_rcHandle);
}

- (void)startTimeSynchronizationLoop {
	if (!_rcHandle)
		return;
	Captury_startTimeSynchronizationLoop(_rcHandle);
}

- (uint64_t)getTime {
	if (!_rcHandle)
		return 0;
	return Captury_getTime(_rcHandle);
}

- (int64_t)getTimeOffset {
	if (!_rcHandle)
		return 0;
	return Captury_getTimeOffset(_rcHandle);
}

#pragma mark - Recording

- (BOOL)setShotName:(NSString *)name {
	if (!_rcHandle)
		return NO;
	return Captury_setShotName(_rcHandle, [name UTF8String]) != 0;
}

- (int64_t)startRecording {
	if (!_rcHandle)
		return 0;
	return Captury_startRecording(_rcHandle);
}

- (BOOL)stopRecording {
	if (!_rcHandle)
		return NO;
	return Captury_stopRecording(_rcHandle) != 0;
}

- (nullable CapturyLatencyInfoObjC *)currentLatency {
	if (!_rcHandle)
		return nil;
	CapturyLatencyInfo info;
	if (Captury_getCurrentLatency(_rcHandle, &info) != 0) {
		return [[CapturyLatencyInfoObjC alloc] initWithLatencyInfo:info];
	}
	return nil;
}

#pragma mark - Background Capture & Diagnostics

- (BOOL)captureBackgroundWithFinishedBlock:(nullable CapturyBackgroundFinishedBlock)block {
	if (!_rcHandle)
		return NO;
	if (_bgFinishedBlock)
		Block_release(_bgFinishedBlock);
	_bgFinishedBlock = block ? (CapturyBackgroundFinishedBlock)Block_copy(block) : (CapturyBackgroundFinishedBlock)NULL;
	return Captury_captureBackground(_rcHandle, block ? objcBackgroundFinishedCallback : NULL, (__bridge void *)self) != 0;
}

- (int)backgroundQuality {
	if (!_rcHandle)
		return 0;
	return Captury_getBackgroundQuality(_rcHandle);
}

- (nullable NSString *)status {
	if (!_rcHandle)
		return nil;
	const char *st = Captury_getStatus(_rcHandle);
	return st ? [NSString stringWithUTF8String:st] : nil;
}

- (void)enablePrintf:(BOOL)enable {
	if (!_rcHandle)
		return;
	Captury_enablePrintf(_rcHandle, enable ? 1 : 0);
}

- (void)enableRemoteLogging:(BOOL)enable {
	if (!_rcHandle)
		return;
	Captury_enableRemoteLogging(_rcHandle, enable ? 1 : 0);
}

- (nullable NSString *)nextLogMessage {
	if (!_rcHandle)
		return nil;
	const char *msg = Captury_getNextLogMessage(_rcHandle);
	if (!msg)
		return nil;
	NSString *res = [NSString stringWithUTF8String:msg];
	Captury_freeErrorMessage((char *)msg);
	return res;
}

- (void)logWithLevel:(int)logLevel message:(NSString *)message {
	if (!_rcHandle)
		return;
	Captury_log(_rcHandle, logLevel, "%s", [message UTF8String]);
}

- (nullable NSDictionary<NSString *, NSNumber *> *)framerate {
	if (!_rcHandle)
		return nil;
	int num = 0, den = 0;
	Captury_getFramerate(_rcHandle, &num, &den);
	return @{@"numerator": @(num), @"denominator": @(den)};
}

#pragma mark - Helper / Utilities

- (void)convertPoseToLocal:(CapturyPoseObjC *)pose forActor:(int32_t)actorId {
	if (!_rcHandle || !pose)
		return;
	// Build temporary CapturyPose struct
	NSUInteger count = [pose.transforms count];
	std::vector<CapturyTransform> trafos(count);
	for (NSUInteger i = 0; i < count; ++i) {
		trafos[i] = [[pose.transforms objectAtIndex:i] toCapturyTransform];
	}
	CapturyPose cPose;
	cPose.actor = pose.actorId;
	cPose.timestamp = pose.timestamp;
	cPose.numTransforms = (int)count;
	cPose.transforms = trafos.data();
	cPose.flags = (uint32_t)pose.flags;

	std::vector<float> blendShapes([pose.blendShapeActivations count]);
	for (NSUInteger i = 0; i < [pose.blendShapeActivations count]; ++i) {
		blendShapes[i] = [[pose.blendShapeActivations objectAtIndex:i] floatValue];
	}
	cPose.numBlendShapes = (int)blendShapes.size();
	cPose.blendShapeActivations = blendShapes.data();

	Captury_convertPoseToLocal(_rcHandle, &cPose, actorId);

	// Copy back modified local transforms
	NSMutableArray<CapturyTransformObjC *> *localTrafos = [NSMutableArray arrayWithCapacity:count];
	for (NSUInteger i = 0; i < count; ++i) {
		[localTrafos addObject:[[CapturyTransformObjC alloc] initWithCapturyTransform:cPose.transforms[i]]];
	}
	pose.transforms = [localTrafos copy];
}

@end
