#import <Foundation/Foundation.h>
#import "RemoteCaptury.h"

NS_ASSUME_NONNULL_BEGIN

#pragma mark - Struct Wrappers & Enums

typedef NS_ENUM(NSInteger, CapturyActorStatusObjC) {
	CapturyActorStatusScaling = ACTOR_SCALING,
	CapturyActorStatusTracking = ACTOR_TRACKING,
	CapturyActorStatusStopped = ACTOR_STOPPED,
	CapturyActorStatusDeleted = ACTOR_DELETED,
	CapturyActorStatusUnknown = ACTOR_UNKNOWN
};

typedef NS_ENUM(NSInteger, CapturySnapMethodObjC) {
	CapturySnapMethodBackgroundLocal = SNAP_BACKGROUND_LOCAL,
	CapturySnapMethodBackgroundGlobal = SNAP_BACKGROUND_GLOBAL,
	CapturySnapMethodBodypartsLocal = SNAP_BODYPARTS_LOCAL,
	CapturySnapMethodBodypartsGlobal = SNAP_BODYPARTS_GLOBAL,
	CapturySnapMethodBodypartsJoints = SNAP_BODYPARTS_JOINTS,
	CapturySnapMethodDefault = SNAP_DEFAULT
};

typedef NS_OPTIONS(NSUInteger, CapturyStreamFlags) {
	CapturyStreamFlagsNothing = CAPTURY_STREAM_NOTHING,
	CapturyStreamFlagsPoses = CAPTURY_STREAM_POSES,
	CapturyStreamFlagsGlobalPoses = CAPTURY_STREAM_GLOBAL_POSES,
	CapturyStreamFlagsLocalPoses = CAPTURY_STREAM_LOCAL_POSES,
	CapturyStreamFlagsARTags = CAPTURY_STREAM_ARTAGS,
	CapturyStreamFlagsImages = CAPTURY_STREAM_IMAGES,
	CapturyStreamFlagsMetaData = CAPTURY_STREAM_META_DATA,
	CapturyStreamFlagsIMUData = CAPTURY_STREAM_IMU_DATA,
	CapturyStreamFlagsLatencyInfo = CAPTURY_STREAM_LATENCY_INFO,
	CapturyStreamFlagsFootContact = CAPTURY_STREAM_FOOT_CONTACT,
	CapturyStreamFlagsCompressed = CAPTURY_STREAM_COMPRESSED,
	CapturyStreamFlagsAngles = CAPTURY_STREAM_ANGLES,
	CapturyStreamFlagsScales = CAPTURY_STREAM_SCALES,
	CapturyStreamFlagsBlendShapes = CAPTURY_STREAM_BLENDSHAPES,
	CapturyStreamFlagsTCP = CAPTURY_STREAM_TCP,
	CapturyStreamFlagsOnlyRootTranslation = CAPTURY_STREAM_ONLY_ROOT_TRANSLATION
};

typedef NS_OPTIONS(NSUInteger, CapturyPoseFlagsObjC) {
	CapturyPoseFlagsLeftFootOnGround = CAPTURY_LEFT_FOOT_ON_GROUND,
	CapturyPoseFlagsRightFootOnGround = CAPTURY_RIGHT_FOOT_ON_GROUND
};

@interface CapturyTransformObjC : NSObject {
	float _translationX;
	float _translationY;
	float _translationZ;
	float _rotationX;
	float _rotationY;
	float _rotationZ;
}
@property (nonatomic, assign) float translationX;
@property (nonatomic, assign) float translationY;
@property (nonatomic, assign) float translationZ;
@property (nonatomic, assign) float rotationX; // Euler angles in degrees / radians as provided
@property (nonatomic, assign) float rotationY;
@property (nonatomic, assign) float rotationZ;

- (instancetype)initWithCapturyTransform:(CapturyTransform)transform;
- (CapturyTransform)toCapturyTransform;
@end

@interface CapturyJointObjC : NSObject {
	NSString *_name;
	int32_t _parent;
	float _offsetX;
	float _offsetY;
	float _offsetZ;
	float _orientationX;
	float _orientationY;
	float _orientationZ;
	float _scaleX;
	float _scaleY;
	float _scaleZ;
	int8_t _boneType;
}
@property (nonatomic, copy) NSString *name;
@property (nonatomic, assign) int32_t parent;
@property (nonatomic, assign) float offsetX;
@property (nonatomic, assign) float offsetY;
@property (nonatomic, assign) float offsetZ;
@property (nonatomic, assign) float orientationX;
@property (nonatomic, assign) float orientationY;
@property (nonatomic, assign) float orientationZ;
@property (nonatomic, assign) float scaleX;
@property (nonatomic, assign) float scaleY;
@property (nonatomic, assign) float scaleZ;
@property (nonatomic, assign) int8_t boneType;

- (instancetype)initWithCapturyJoint:(CapturyJoint)joint;
@end

@interface CapturyBlendShapeObjC : NSObject {
	NSString *_name;
}
@property (nonatomic, copy) NSString *name;

- (instancetype)initWithName:(NSString *)name;
@end

@interface CapturyBlobObjC : NSObject {
	int32_t _parent;
	float _offsetX;
	float _offsetY;
	float _offsetZ;
	float _size;
	float _colorR;
	float _colorG;
	float _colorB;
}
@property (nonatomic, assign) int32_t parent;
@property (nonatomic, assign) float offsetX;
@property (nonatomic, assign) float offsetY;
@property (nonatomic, assign) float offsetZ;
@property (nonatomic, assign) float size;
@property (nonatomic, assign) float colorR;
@property (nonatomic, assign) float colorG;
@property (nonatomic, assign) float colorB;

- (instancetype)initWithCapturyBlob:(CapturyBlob)blob;
@end

@interface CapturyActorObjC : NSObject {
	NSString *_name;
	int32_t _actorId;
	NSArray<CapturyJointObjC *> *_joints;
	NSArray<CapturyBlobObjC *> *_blobs;
	NSArray<CapturyBlendShapeObjC *> *_blendShapes;
	NSDictionary<NSString *, NSString *> *_metaData;
}
@property (nonatomic, copy) NSString *name;
@property (nonatomic, assign) int32_t actorId;
@property (nonatomic, copy) NSArray<CapturyJointObjC *> *joints;
@property (nonatomic, copy) NSArray<CapturyBlobObjC *> *blobs;
@property (nonatomic, copy) NSArray<CapturyBlendShapeObjC *> *blendShapes;
@property (nonatomic, copy) NSDictionary<NSString *, NSString *> *metaData;

- (instancetype)initWithCapturyActor:(const CapturyActor *)actor;
@end

@interface CapturyCameraObjC : NSObject {
	NSString *_name;
	int32_t _cameraId;
	float _positionX;
	float _positionY;
	float _positionZ;
	float _orientationX;
	float _orientationY;
	float _orientationZ;
	float _sensorSizeWidth;
	float _sensorSizeHeight;
	float _focalLength;
	float _lensCenterX;
	float _lensCenterY;
	NSString *_distortionModel;
}
@property (nonatomic, copy) NSString *name;
@property (nonatomic, assign) int32_t cameraId;
@property (nonatomic, assign) float positionX;
@property (nonatomic, assign) float positionY;
@property (nonatomic, assign) float positionZ;
@property (nonatomic, assign) float orientationX;
@property (nonatomic, assign) float orientationY;
@property (nonatomic, assign) float orientationZ;
@property (nonatomic, assign) float sensorSizeWidth;
@property (nonatomic, assign) float sensorSizeHeight;
@property (nonatomic, assign) float focalLength;
@property (nonatomic, assign) float lensCenterX;
@property (nonatomic, assign) float lensCenterY;
@property (nonatomic, copy) NSString *distortionModel;

- (instancetype)initWithCapturyCamera:(const CapturyCamera *)camera;
@end

@interface CapturyPoseObjC : NSObject {
	int32_t _actorId;
	uint64_t _timestamp;
	NSArray<CapturyTransformObjC *> *_transforms;
	CapturyPoseFlagsObjC _flags;
	NSArray<NSNumber *> *_blendShapeActivations;
}
@property (nonatomic, assign) int32_t actorId;
@property (nonatomic, assign) uint64_t timestamp;
@property (nonatomic, copy) NSArray<CapturyTransformObjC *> *transforms;
@property (nonatomic, assign) CapturyPoseFlagsObjC flags;
@property (nonatomic, copy) NSArray<NSNumber *> *blendShapeActivations;

- (instancetype)initWithCapturyPose:(const CapturyPose *)pose;
@end

@interface CapturyAngleDataObjC : NSObject {
	uint16_t _type;
	float _value;
}
@property (nonatomic, assign) uint16_t type;
@property (nonatomic, assign) float value;

- (instancetype)initWithAngleData:(CapturyAngleData)angleData;
@end

@interface CapturyARTagObjC : NSObject {
	int32_t _tagId;
	CapturyTransformObjC *_transform;
}
@property (nonatomic, assign) int32_t tagId;
@property (nonatomic, strong) CapturyTransformObjC *transform;

- (instancetype)initWithCapturyARTag:(CapturyARTag)artag;
@end

@interface CapturyImageObjC : NSObject {
	int32_t _width;
	int32_t _height;
	int32_t _cameraId;
	uint64_t _timestamp;
	NSData *_imageData;
}
@property (nonatomic, assign) int32_t width;
@property (nonatomic, assign) int32_t height;
@property (nonatomic, assign) int32_t cameraId;
@property (nonatomic, assign) uint64_t timestamp;
@property (nonatomic, strong) NSData *imageData;

- (instancetype)initWithCapturyImage:(const CapturyImage *)image;
@end

@interface CapturyLatencyInfoObjC : NSObject {
	uint64_t _firstImagePacketTime;
	uint64_t _optimizationStartTime;
	uint64_t _optimizationEndTime;
	uint64_t _poseSentTime;
	uint64_t _poseReceivedTime;
	uint64_t _timestampOfCorrespondingPose;
}
@property (nonatomic, assign) uint64_t firstImagePacketTime;
@property (nonatomic, assign) uint64_t optimizationStartTime;
@property (nonatomic, assign) uint64_t optimizationEndTime;
@property (nonatomic, assign) uint64_t poseSentTime;
@property (nonatomic, assign) uint64_t poseReceivedTime;
@property (nonatomic, assign) uint64_t timestampOfCorrespondingPose;

- (instancetype)initWithLatencyInfo:(CapturyLatencyInfo)info;
@end

#pragma mark - Callbacks & Delegate

@class CapturyRemote;

@protocol CapturyRemoteDelegate <NSObject>
@optional
- (void)capturyRemote:(CapturyRemote *)remote didReceivePose:(CapturyPoseObjC *)pose actor:(nullable CapturyActorObjC *)actor trackingQuality:(int)trackingQuality;
- (void)capturyRemote:(CapturyRemote *)remote didReceiveAngles:(NSArray<CapturyAngleDataObjC *> *)angles forActor:(nullable CapturyActorObjC *)actor;
- (void)capturyRemote:(CapturyRemote *)remote actorStatusChanged:(int32_t)actorId status:(CapturyActorStatusObjC)status;
- (void)capturyRemote:(CapturyRemote *)remote didReceiveARTags:(NSArray<CapturyARTagObjC *> *)artags;
- (void)capturyRemote:(CapturyRemote *)remote didReceiveStreamedImage:(CapturyImageObjC *)image;
- (void)capturyRemote:(CapturyRemote *)remote didReceiveLogMessage:(NSString *)message level:(int)logLevel;
@end

typedef void (^CapturyNewPoseBlock)(CapturyActorObjC *_Nullable actor, CapturyPoseObjC *pose, int trackingQuality);
typedef void (^CapturyNewAnglesBlock)(CapturyActorObjC *_Nullable actor, NSArray<CapturyAngleDataObjC *> *angles);
typedef void (^CapturyActorChangedBlock)(int32_t actorId, CapturyActorStatusObjC status);
typedef void (^CapturyARTagBlock)(NSArray<CapturyARTagObjC *> *artags);
typedef void (^CapturyImageBlock)(CapturyImageObjC *image);
typedef void (^CapturyLogBlock)(int logLevel, NSString *message);
typedef void (^CapturyBackgroundFinishedBlock)(void);

#pragma mark - Main CapturyRemote Interface

@interface CapturyRemote : NSObject {
	id<CapturyRemoteDelegate> _delegate;
	RemoteCaptury *_rcHandle;
@public
	CapturyNewPoseBlock _poseBlock;
	CapturyNewAnglesBlock _anglesBlock;
	CapturyActorChangedBlock _actorChangedBlock;
	CapturyARTagBlock _artagBlock;
	CapturyImageBlock _imageBlock;
	CapturyLogBlock _logBlock;
	CapturyBackgroundFinishedBlock _bgFinishedBlock;
}

@property (nonatomic, weak, nullable) id<CapturyRemoteDelegate> delegate;

// Lifecycle
- (instancetype)init;

// Connection
- (BOOL)connectToHost:(NSString *)ip port:(unsigned short)port;
- (BOOL)connectToHost:(NSString *)ip
		 port:(unsigned short)port
	    localPort:(unsigned short)localPort
      localStreamPort:(unsigned short)localStreamPort
		async:(BOOL)async
	 localAddress:(nullable NSString *)localAddress
     multicastAddress:(nullable NSString *)multicastAddress;
- (BOOL)disconnect;
- (int)connectionStatus;

// Actors & Cameras
- (nullable NSArray<CapturyActorObjC *> *)fetchActors;
- (nullable CapturyActorObjC *)fetchActorWithId:(int32_t)actorId;
- (nullable NSArray<CapturyCameraObjC *> *)fetchCamerasWithWaitTimeMs:(int)waitTimeMs;

// Streaming
- (BOOL)startStreaming:(CapturyStreamFlags)what;
- (BOOL)startStreamingImages:(CapturyStreamFlags)what cameraId:(int32_t)cameraId;
- (BOOL)startStreamingImagesAndAngles:(CapturyStreamFlags)what cameraId:(int32_t)cameraId angles:(NSArray<NSNumber *> *)angles;
- (BOOL)stopStreaming;
- (BOOL)stopStreamingWait:(BOOL)wait;

// Poses & Polling Data
- (nullable CapturyPoseObjC *)currentPoseForActor:(int32_t)actorId;
- (nullable CapturyPoseObjC *)currentPoseForActor:(int32_t)actorId trackingConsistency:(out int *_Nullable)tc;
- (nullable NSArray<CapturyAngleDataObjC *> *)currentAnglesForActor:(int32_t)actorId;
- (nullable NSArray<CapturyARTagObjC *> *)currentARTags;
- (nullable CapturyImageObjC *)currentImage;

// Callbacks Registration
- (BOOL)registerPoseBlock:(nullable CapturyNewPoseBlock)block;
- (BOOL)registerAnglesBlock:(nullable CapturyNewAnglesBlock)block;
- (BOOL)registerActorChangedBlock:(nullable CapturyActorChangedBlock)block;
- (BOOL)registerARTagBlock:(nullable CapturyARTagBlock)block;
- (BOOL)registerImageStreamingBlock:(nullable CapturyImageBlock)block;
- (void)registerLogBlock:(nullable CapturyLogBlock)block;

// Actor Operations
- (CapturyActorStatusObjC)actorStatusForId:(int32_t)actorId;
- (BOOL)requestTextureForActor:(int32_t)actorId;
- (nullable CapturyImageObjC *)textureForActor:(int32_t)actorId;
- (uint64_t)markerTransformForActor:(int32_t)actorId joint:(int32_t)joint transform:(out CapturyTransformObjC *_Nullable *_Nullable)outTrafo;
- (int)scalingProgressForActor:(int32_t)actorId;
- (int)trackingQualityForActor:(int32_t)actorId;
- (BOOL)setActorName:(NSString *)name forActor:(int32_t)actorId;

- (BOOL)snapActorAtX:(float)x z:(float)z heading:(float)heading;
- (BOOL)snapActorExAtX:(float)x z:(float)z radius:(float)radius heading:(float)heading skeletonName:(nullable NSString *)skeletonName snapMethod:(CapturySnapMethodObjC)snapMethod quickScaling:(BOOL)quickScaling;
- (BOOL)startTrackingActor:(int32_t)actorId atX:(float)x z:(float)z heading:(float)heading;
- (BOOL)stopTrackingActor:(int32_t)actorId;
- (BOOL)deleteActor:(int32_t)actorId;
- (BOOL)rescaleActor:(int32_t)actorId;
- (BOOL)recolorActor:(int32_t)actorId;
- (BOOL)updateActorColors:(int32_t)actorId;

// Time Synchronization
- (uint64_t)synchronizeTime;
- (void)startTimeSynchronizationLoop;
- (uint64_t)getTime;
- (int64_t)getTimeOffset;

// Recording
- (BOOL)setShotName:(NSString *)name;
- (int64_t)startRecording;
- (BOOL)stopRecording;
- (nullable CapturyLatencyInfoObjC *)currentLatency;

// Background Capture & Diagnostics
- (BOOL)captureBackgroundWithFinishedBlock:(nullable CapturyBackgroundFinishedBlock)block;
- (int)backgroundQuality;
- (nullable NSString *)status;
- (void)enablePrintf:(BOOL)enable;
- (void)enableRemoteLogging:(BOOL)enable;
- (nullable NSString *)nextLogMessage;
- (void)logWithLevel:(int)logLevel message:(NSString *)message;
- (nullable NSDictionary<NSString *, NSNumber *> *)framerate;

// Helper / Utilities
- (void)convertPoseToLocal:(CapturyPoseObjC *)pose forActor:(int32_t)actorId;

@end

NS_ASSUME_NONNULL_END
