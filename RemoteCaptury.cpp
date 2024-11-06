#if 1
#include "RemoteCaptury.h"

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <list>
#include <ctime>
#include <time.h>

#include <string.h>
#include <inttypes.h>
#include <cmath>

#include <stdlib.h>
#include <stdio.h>
// #define MAXIMUM(a, b) ((a) > (b) ? (a) : (b))
#ifdef WIN32
#undef max
#undef min
#pragma warning(disable : 4200)
#pragma warning(disable : 4996)
#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_WINNT_WINTHRESHOLD 0
#define _APISET_RTLSUPPORT_VER 0
#define _APISET_INTERLOCKED_VER 0
#define _APISET_SECURITYBASE_VER 0
#define NTDDI_WIN7SP1 0
#define snprintf _snprintf
#include <winsock2.h>
#include <sysinfoapi.h>
#pragma comment(lib, "ws2_32.lib")
#ifndef PRIu64
  #define PRIu64 "I64u"
#endif
#ifndef PRId64
  #define PRId64 "I64d"
#endif
#ifndef SYSTEM_INFO
  #include <chrono>
#endif
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#endif

#include <stdarg.h>

typedef uint32_t uint;

#if defined(__clang__) && (!defined(SWIG))
#define THREAD_ANNOTATION_ATTRIBUTE__(x)   __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x)   // no-op
#endif

#define CAPABILITY(x)		THREAD_ANNOTATION_ATTRIBUTE__(capability(x))
#define GUARDED_BY(x)		THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))
#define REQUIRES(...)		THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(__VA_ARGS__))
#define ACQUIRE(...)		THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(__VA_ARGS__))
#define RELEASE(...)		THREAD_ANNOTATION_ATTRIBUTE__(release_capability(__VA_ARGS__))
// #pragma clang optimize off
// #pragma GCC optimize("O0")

#ifdef WIN32

static HANDLE		streamThread;
static HANDLE		receiveThread;
static HANDLE		syncThread;
static CRITICAL_SECTION	mutex;
static CRITICAL_SECTION	partialActorMutex;
static CRITICAL_SECTION syncMutex;
static CRITICAL_SECTION logMutex;
static bool mutexesInited = false;
#define socklen_t int
#define sleepMicroSeconds(us) Sleep(us / 1000)

static inline int sockerror()		{ return WSAGetLastError(); }
static inline const char* sockstrerror(int err) { static char msg[200]; FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), msg, sizeof(msg), nullptr); return msg; }
static inline const char* sockstrerror() { static char msg[200]; FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), msg, sizeof(msg), nullptr); return msg; }

static inline int setSocketTimeout(SOCKET sock, int timeout_ms)
{
	DWORD timeout = timeout_ms; // timeout in ms
	return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
}

static bool wsaInited = false;

#else

#define SOCKET		int
#define closesocket	close
static pthread_t	streamThread;
static pthread_t	receiveThread;
static pthread_t	syncThread;
struct CAPABILITY("mutex") MutexStruct {
	pthread_mutex_t	m;
	MutexStruct()			{ pthread_mutex_init(&m, nullptr); }
	void lock() ACQUIRE()		{ pthread_mutex_lock(&m); }
	void unlock() RELEASE()		{ pthread_mutex_unlock(&m); }
};
static MutexStruct mutex;
static MutexStruct partialActorMutex;
static MutexStruct syncMutex;
static MutexStruct logMutex;
// static pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;
#define sleepMicroSeconds(us) usleep(us)

static inline int sockerror()		{ return errno; }
static inline const char* sockstrerror(int err) { return strerror(err); }
static inline const char* sockstrerror() { return strerror(errno); }

static inline int setSocketTimeout(SOCKET sock, int timeout_ms)
{
	struct timeval tv;
	tv.tv_sec = timeout_ms / 1000;// seconds timeout
	tv.tv_usec = (timeout_ms % 1000) * 1000;
	return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
}

#endif

static bool syncLoopIsRunning = false;

static int streamWhat = CAPTURY_STREAM_NOTHING;
static int32_t streamCamera;
static std::vector<uint16_t> streamAngles;

static std::string currentDay;
static std::string currentSession;
static std::string currentShot;

typedef std::shared_ptr<CapturyActor> CapturyActor_p;

// actor id -> pointer to actor
static std::unordered_map<int, CapturyActor_p> actorsById GUARDED_BY(mutex);
static std::unordered_map<int, CapturyActor_p> partialActors GUARDED_BY(partialActorMutex); // actors that have been received in part
static std::vector<CapturyActor> actorPointers; // used by Captury_getActors()

static std::unordered_map<int, std::vector<CapturyAngleData>> currentAngles;

static int numCameras = -1;
static std::vector<CapturyCamera> cameras;

static CapturyLatencyPacket currentLatency;
static uint64_t receivedPoseTime; // time pose packet was received
static uint64_t receivedPoseTimestamp; // timestamp of pose that corresponds to the receivedPoseTime
static uint64_t dataAvailableTime;
static uint64_t dataReceivedTime;
static uint64_t mostRecentPoseReceivedTime; // time pose was received
static uint64_t mostRecentPoseReceivedTimestamp; // timestamp of that pose

const char* CapturyActorStatusString[] = {"scaling", "tracking", "stopped", "deleted", "unknown"};

struct ActorData {
	// actor id -> scaling progress (0 to 100)
	int			scalingProgress;
	// actor id -> tracking quality (0 to 100)
	int			trackingQuality;
	// actor id -> pose
	CapturyPose		currentPose;
	struct InProgress {
		float*			pose;
		int			bytesDone;
		uint64_t		timestamp;
	};
	InProgress		inProgress[4];
	// actor id -> timestamp
	uint64_t		lastPoseTimestamp;

	// actor id -> texture
	CapturyImage		currentTextures;
	std::vector<int>	receivedPackets;

	CapturyActorStatus	status;

	int			flags;

	ActorData() : scalingProgress(0), trackingQuality(100), lastPoseTimestamp(0), status(ACTOR_STOPPED), flags(0)
	{
		currentPose.timestamp = 0;
		currentPose.numTransforms = 0;
		currentPose.numBlendShapes = 0;
		currentPose.flags = 0;
		currentTextures.width = 0;
		currentTextures.height = 0;
		currentTextures.data = NULL;
		for (int i = 0; i < 4; ++i) {
			inProgress[i].pose = NULL;
			inProgress[i].timestamp = 0;
		}
	}
};

static std::unordered_map<int, ActorData> actorData GUARDED_BY(mutex);

static std::map<int32_t, CapturyImage> currentImages;
static std::map<int32_t, std::vector<int>> currentImagesReceivedPackets;
static std::map<int32_t, CapturyImage> currentImagesDone;

// custom type name -> callback
static std::map<std::string, CapturyCustomPacketCallback> callbacks;

static uint64_t arTagsTime;
static std::vector<CapturyARTag> arTags;

// helper structs
struct ActorAndJoint {
	int			actorId;
	int			jointIndex;

	bool operator<(const ActorAndJoint& aj) const
	{
		if (actorId < aj.actorId)
			return true;
		if (actorId > aj.actorId)
			return false;
		return (jointIndex < aj.jointIndex);
	}
	ActorAndJoint() : actorId(0), jointIndex(-1) {}
	ActorAndJoint(int actor, int joint) : actorId(actor), jointIndex(joint) {}
};

struct MarkerTransform {
	CapturyTransform	trafo;
	uint64_t		timestamp;
};
// actor id + joint index -> marker transformation + timestamp
static std::map<ActorAndJoint, MarkerTransform> markerTransforms;

// error message
static std::string lastErrorMessage;
static std::string lastStatusMessage = "disconnected";

static bool getLocalPoses = false;

static CapturyNewPoseCallback newPoseCallback = NULL;
static CapturyNewAnglesCallback newAnglesCallback = NULL;
static CapturyActorChangedCallback actorChangedCallback = NULL;
static CapturyARTagCallback arTagCallback = NULL;
static CapturyImageCallback imageCallback = NULL;

static bool		handshakeFinished = false;
static bool		isStreamThreadRunning = false;
static SOCKET		sock = -1;

static volatile int	stopStreamThread = 0; // stop streaming thread
static volatile int	stopReceiving = 0; // stop receiving thread

static sockaddr_in	localAddress; // local address
static sockaddr_in	localStreamAddress; // local address for streaming socket
static sockaddr_in	remoteAddress; // address of server
static uint16_t		streamSocketPort = 0;

static uint64_t		pingTime;
static int32_t		nextTimeId = 213;

static int					backgroundQuality = -1;
static CapturyBackgroundFinishedCallback	backgroundFinishedCallback = NULL;
static void*					backgroundFinishedCallbackUserData = NULL;

static int64_t				startRecordingTime = 0;

static bool				doPrintf = true;
static bool				doRemoteLogging = false;
static std::list<std::string>		logs;

#ifndef WIN32
static void log(const char *format, ...) __attribute__((format(printf,1,2)));
#endif

struct Sync {
	double offset;
	double factor;

	Sync(double o, double f) : offset(o), factor(f) {}

	uint64_t getRemoteTime(uint64_t localT)	{ return uint64_t((localT) * factor + offset); }
};

struct SyncSample {
	int64_t		localT;
	int64_t		remoteT;
	uint32_t	pingPongT;

	SyncSample(uint64_t l, uint64_t r, uint32_t pp) : localT(l), remoteT(r), pingPongT(pp) {}
};

std::vector<SyncSample> syncSamples;

#ifdef WIN32
static inline void lockMutex(CRITICAL_SECTION* critsec)
{
	EnterCriticalSection(critsec);
}
static inline void unlockMutex(CRITICAL_SECTION* critsec)
{
	LeaveCriticalSection(critsec);
}
#else
static inline void lockMutex(MutexStruct* mtx) ACQUIRE(mtx)
{
	mtx->lock();
}
static inline void unlockMutex(MutexStruct* mtx) RELEASE(mtx)
{
	mtx->unlock();
}
#endif

static void actualLog(int logLevel, const char* format, va_list args)
{
	#ifdef WIN32
	if (!mutexesInited) {
		InitializeCriticalSection(&mutex);
		InitializeCriticalSection(&partialActorMutex);
		InitializeCriticalSection(&syncMutex);
		InitializeCriticalSection(&logMutex);
		mutexesInited = true;
	}
	#endif

	char buffer[509];
	vsnprintf(buffer + 9, 500, format, args);

	if (doPrintf)
		printf("%s", buffer + 9);

	lockMutex(&logMutex);
	logs.emplace_back(buffer + 9);

	if (logs.size() > 100000)
		logs.pop_front();
	unlockMutex(&logMutex);

	if (doRemoteLogging && sock != -1) {
		CapturyLogPacket* lp = (CapturyLogPacket*)buffer;
		lp->type = capturyMessage;
		lp->size = 9 + strlen(buffer + 9) + 1;
		lp->logLevel = logLevel;
		send(sock, (const char*)lp, lp->size, 0);
	}
}

static void log(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	actualLog(CAPTURY_LOG_INFO, format, args);
	va_end(args);
}

void Captury_log(int logLevel, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	actualLog(logLevel, format, args);
	va_end(args);
}

void Captury_enablePrintf(int on)
{
	doPrintf = (on != 0);
}

void Captury_enableRemoteLogging(int on)
{
	doRemoteLogging = (on != 0);
}

const char* Captury_getNextLogMessage()
{
	lockMutex(&logMutex);
	if (logs.empty()) {
		unlockMutex(&logMutex);
		return nullptr;
	}

	const char* str = strdup(logs.front().c_str());
	logs.pop_front();
	unlockMutex(&logMutex);

	return str;
}

const char* Captury_getHumanReadableMessageType(CapturyPacketTypes type)
{
	switch (type) {
	case capturyActors:
		return "<actors>";
	case capturyActor:
		return "<actor>";
	case capturyCameras:
		return "<cameras>";
	case capturyCamera:
		return "<camera>";
	case capturyStream:
		return "<stream>";
	case capturyStreamAck:
		return "<stream ack>";
	case capturyPose:
		return "<pose>";
	case capturyDaySessionShot:
		return "<day/session/shot>";
	case capturySetShot:
		return "<set shot>";
	case capturySetShotAck:
		return "<set shot ack>";
	case capturyStartRecording:
		return "<start recording>";
	case capturyStartRecordingAck:
		return "<start recording ack>";
	case capturyStopRecording:
		return "<stop recording>";
	case capturyStopRecordingAck:
		return "<stop recording ack>";
	case capturyConstraint:
		return "<constraint>";
	case capturyConstraintAck:
		return "<constraint ack>";
	case capturyGetTime:
		return "<get time>";
	case capturyTime:
		return "<time>";
	case capturyCustom:
		return "<custom>";
	case capturyCustomAck:
		return "<custom ack>";
	case capturyGetImage:
		return "<get image>";
	case capturyImageHeader:
		return "<texture header>";
	case capturyImageData:
		return "<texture data>";
	case capturyGetImageData:
		return "<get image data>";
	case capturyActorContinued:
		return "<actor continued>";
	case capturyGetMarkerTransform:
		return "<get marker transform>";
	case capturyMarkerTransform:
		return "<marker transform>";
	case capturyGetScalingProgress:
		return "<get scaling progress>";
	case capturyScalingProgress:
		return "<scaling progress>";
	case capturySnapActor:
		return "<snap actor>";
	case capturySnapActorAck:
		return "<snap actor ack>";
	case capturyStopTracking:
		return "<stop tracking>";
	case capturyStopTrackingAck:
		return "<stop tracking ack>";
	case capturyDeleteActor:
		return "<delete actor>";
	case capturyDeleteActorAck:
		return "<delete actor ack>";
	case capturyActorModeChanged:
		return "<actor mode changed>";
	case capturyARTag:
		return "<ar tag>";
	case capturyGetBackgroundQuality:
		return "<get background quality>";
	case capturyBackgroundQuality:
		return "<background quality>";
	case capturyCaptureBackground:
		return "<capture background>";
	case capturyCaptureBackgroundAck:
		return "<capture background ack>";
	case capturyBackgroundFinished:
		return "<capture background finished>";
	case capturySetActorName:
		return "<set actor name>";
	case capturySetActorNameAck:
		return "<set actor name ack>";
	case capturyStreamedImageHeader:
		return "<streamed image header>";
	case capturyStreamedImageData:
		return "<streamed image data>";
	case capturyGetStreamedImageData:
		return "<get streamed image data>";
	case capturyRescaleActor:
		return "<rescale actor>";
	case capturyRecolorActor:
		return "<recolor actor>";
	case capturyUpdateActorColors:
		return "<update actor colors>";
	case capturyRescaleActorAck:
		return "<rescale actor ack>";
	case capturyRecolorActorAck:
		return "<recolor actor ack>";
	case capturyStartTracking:
		return "<start tracking>";
	case capturyStartTrackingAck:
		return "<start tracking ack>";
	case capturyPoseCont:
		return "<pose continued>";
	case capturyPose2:
		return "<pose2>";
	case capturyGetStatus:
		return "<get status>";
	case capturyStatus:
		return "<status>";
	case capturyActor2:
		return "<actor2>";
	case capturyActorContinued2:
		return "<actor2 continued>";
	case capturyLatency:
		return "<latency measurements>";
	case capturyActors2:
		return "<actors2>";
	case capturyActor3:
		return "<actor3>";
	case capturyActorContinued3:
		return "<actor3 continued>";
	case capturyCompressedPose:
		return "<compressed pose>";
	case capturyCompressedPose2:
		return "<compressed pose2>";
	case capturyCompressedPoseCont:
		return "<compressed pose continued>";
	case capturyGetTime2:
		return "<get time2>";
	case capturyTime2:
		return "<time2>";
	case capturyAngles:
		return "<angles>";
	case capturyStartRecording2:
		return "<start recording 2>";
	case capturyStartRecordingAck2:
		return "<start recording ack 2>";
	case capturyHello:
		return "<hello>";
	case capturyActorBlendShapes:
		return "<actor blend shapes>";
	case capturyMessage:
		return "<message>";
	case capturyEnableRemoteLogging:
		return "<enable remote logging>";
	case capturyDisableRemoteLogging:
		return "<enable remote logging>";
	case capturyError:
		return "<error>";
	}
	return "<unknown message type>";
}

#ifdef SYSTEM_INFO
// thanks https://www.frenk.com/2009/12/convert-filetime-to-unix-timestamp/
// A UNIX timestamp contains the number of seconds from Jan 1, 1970, while the FILETIME documentation says:
// Contains a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC).
//
// Between Jan 1, 1601 and Jan 1, 1970 there are 11644473600 seconds, so we will just subtract that value:
static uint64_t convertFileTimeToTimestamp(FILETIME& ft)
{
	// takes the last modified date
	LARGE_INTEGER date, adjust;
	date.HighPart = ft.dwHighDateTime;
	date.LowPart = ft.dwLowDateTime;

	// 100-nanoseconds = milliseconds * 10000
	adjust.QuadPart = 11644473600000 * 10000;

	// removes the diff between 1970 and 1601
	date.QuadPart -= adjust.QuadPart;

	// converts back from 100-nanoseconds to microseconds
	return date.QuadPart / 10;
}
#endif

//
// returns current time in us
//
static uint64_t getTime()
{
#ifdef WIN32
	#ifdef SYSTEM_INFO
	FILETIME ft;
	GetSystemTimePreciseAsFileTime(&ft);
	return convertFileTimeToTimestamp(ft);
	#else
	std::chrono::time_point<std::chrono::system_clock> tp = std::chrono::system_clock::now();
	std::chrono::duration<double, std::micro> duration = tp.time_since_epoch();
	return (uint64_t)duration.count();
	#endif
#else
	timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	return (uint64_t)t.tv_sec * 1000000 + t.tv_nsec / 1000;
#endif
}

//
// the approach this function takes may not be obvious.
//
// generally the goal is to estimate remote time r as a function of the local time l.
// we assume that r = l * f + o
// i.e. there is an offset between the clocks and a linear drift
//
// given some measurement samples the goal is to compute the two parameters f and o
//
// the naive approach of estimating the parameters directly with a linear system fails
// because of numerical instability. that's why we first subtract mean/median and then
// solve the following:
// note that the median(r-l) is necessary because the remote time measurement is noisy
// local time measurement is relatively smooth. so using the mean is sufficient.
//
// b = r - l - median(r-l)
// a = l - mean(l)
// a * f = b
// f = a \ b                                    <=>
// f = dot(a, b) / dot(a, a)      <- least squares solution
// (l - mean(l)) * f = b                        <=>
// (l - mean(l)) * f = r - l - median(r-l)      <=>
// (l - mean(l)) * f + l + median(r-l) = r      <=>
// (l - mean(l)) * f + l + median(r-l) = r      <=>
// l * (f + 1) + median(r-l) - mean(l) * f = r
//     \_____/   \_______________________/
//      factor             offset
//
// note that for extra efficiency all operations except for the division in the
// least squares solution and the multiplication in the last line can be performed as
// integer operations.
//
// if there are only a few samples we cannot estimate the slope f accurately. so we
// just estimate an offset (the median of the time differences between remote and local)
//
void computeSync(Sync& s)
{
	double meanLocalT = 0;
	double medianOffset = 0.0;

	int64_t offsets[50];

	int num = (int)syncSamples.size();
	for (int i = 0; i < num; ++i) {
		SyncSample& ss = syncSamples[i];
		offsets[i] = ss.remoteT - ss.localT; // alternatively use median here
		meanLocalT += ss.localT;
	}

	std::sort(&offsets[0], &offsets[num]);
	medianOffset = (num % 2 == 0) ? (offsets[num/2] + offsets[num/2+1]) * 0.5 : offsets[num/2];

	if (num < 10 || (syncSamples.back().localT - syncSamples.front().localT) < 1000000) { // not enough samples
		s.offset = medianOffset;
		s.factor = 1.0;
		log("sync based on %d samples: offset %g\n", num, s.offset);
	} else {
		s.offset = medianOffset;
		s.factor = 0.0;
		meanLocalT /= num;
		int64_t sumAsqr = 0;
		int64_t sumAB = 0;
		for (SyncSample& ss : syncSamples) {
			int64_t a = ss.localT - meanLocalT;
			sumAsqr += a*a;
			int64_t b = ss.remoteT - ss.localT - medianOffset;
			sumAB += a*b;
		}
		s.factor = sumAB / (double)sumAsqr;
		s.offset -= meanLocalT * s.factor;
		s.factor += 1.0;
		log("sync based on %d samples: offset %15f, factor %.15f (mlt %15f, moff %15f)\n", num, s.offset, s.factor, meanLocalT, medianOffset);
	}
}

static Sync oldSync GUARDED_BY(syncMutex) = Sync(0.0, 1.0);
static Sync currentSync GUARDED_BY(syncMutex) = Sync(0.0, 1.0);
static uint64_t transitionStartLocalT GUARDED_BY(syncMutex) = 0;
static uint64_t transitionEndLocalT GUARDED_BY(syncMutex) = 0;
static void updateSync(uint64_t localT)
{
	constexpr uint64_t defaultTransitionTime = 100000; // 0.1 second
	Sync tempSync(0.0, 1.0);
	computeSync(tempSync);

	lockMutex(&syncMutex);
	transitionStartLocalT = localT;

	// transitionFactor = std::abs(transitionStartRemoteT - remoteT) / defaultTransitionTime;
	// TODO limit skew speed
	transitionEndLocalT = transitionStartLocalT + defaultTransitionTime;
	double transitionStartRemoteT = (double)currentSync.getRemoteTime(localT);

	oldSync = currentSync;
	currentSync = tempSync;

	double delta = transitionStartRemoteT - currentSync.getRemoteTime(localT);
	double factor = currentSync.factor;
	unlockMutex(&syncMutex);

	log("sync: old - new estimate %g (factor %g)\n", delta, factor);
}

static uint64_t getRemoteTime(uint64_t localT)
{
	lockMutex(&syncMutex);
	if (localT >= transitionEndLocalT) {
		uint64_t t = currentSync.getRemoteTime(localT);
		unlockMutex(&syncMutex);
		return t;
	}

	uint64_t oldEstimate = oldSync.getRemoteTime(localT);
	uint64_t newEstimate = currentSync.getRemoteTime(localT);
	double at = double(localT - transitionStartLocalT) / (transitionEndLocalT - transitionStartLocalT);
	unlockMutex(&syncMutex);
	// log("sync: local %" PRIu64 " old %" PRIu64 " new %" PRIu64 " -> %" PRIu64 "\n", localT, oldEstimate, newEstimate, (uint64_t)(oldEstimate * (1.0 - at) + newEstimate * at));
	return (uint64_t)(oldEstimate * (1.0 - at) + newEstimate * at);
}

extern "C" uint64_t Captury_getTime()
{
	return getRemoteTime(getTime());
}

static SOCKET openTcpSocket()
{
	log("opening TCP socket\n");

	SOCKET sok = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sok == -1)
		return -1;

	if (localAddress.sin_port != 0 && bind(sok, (sockaddr*) &localAddress, sizeof(localAddress)) != 0) {
		closesocket(sok);
		return -1;
	}

	if (connect(sok, (sockaddr*) &remoteAddress, sizeof(remoteAddress)) != 0) {
		closesocket(sok);
		return -1;
	}

	// set read timeout
	setSocketTimeout(sok, 500);

#ifndef WIN32
	char buf[100];
	log("connected to %s:%d\n", inet_ntop(AF_INET, &remoteAddress.sin_addr, buf, 100), ntohs(remoteAddress.sin_port));
#endif

	return sok;
}

static bool isSocketErrorFatal(int err)
{
#ifdef WIN32
	return (err == WSAENOTSOCK || err == WSAESHUTDOWN || err == WSAECONNABORTED || err == WSAECONNRESET || err == WSAENETDOWN || err == WSAENETUNREACH || err == WSAENETRESET || err == WSAEBADF);
#else
	return (err == EPIPE || err == ECONNRESET || err == ENETRESET || err == EBADF);
#endif
}

static bool isSocketErrorTryAgain(int err)
{
#ifdef WIN32
	return (err == WSAEINTR || err == WSAETIMEDOUT || err == WSAEWOULDBLOCK || err == WSAEINPROGRESS || err == 0);
#else
	return (err == EAGAIN || err == EBUSY || err == EINTR);
#endif
}

// waits for at most 500ms before it fails
// returns false if the expected packet is not received
static bool receive(SOCKET& sok)
{
	static std::vector<char> buffer(9000);
	CapturyRequestPacket* p = (CapturyRequestPacket*)buffer.data();

	{
		fd_set reader;
		FD_ZERO(&reader);
		FD_SET(sok, &reader);

		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 500000; // 500ms should be enough
		int ret = select((int)(sok+1), &reader, NULL, NULL, &tv);
		if (ret == -1) { // error
			int err = sockerror();
			if (isSocketErrorFatal(err)) {
				// connection closed by peer or network down
				closesocket(sok);
				sok = -1;
			}
			log("error waiting for socket: %s\n", sockstrerror(err));
			return false;
		}
		if (ret == 0) { // timeout
			return true;
		}

		{
			struct sockaddr_in thisEnd;
			socklen_t len = sizeof(thisEnd);
 			getsockname(sok, (sockaddr*) &thisEnd, &len);
		}

		// first peek to find out which packet type this is
		int size = recv(sok, buffer.data(), sizeof(CapturyRequestPacket), 0);
		if (size == 0) { // the other end shut down the socket...
			log("socket shut down by other end %s\n", sockstrerror());
			closesocket(sok);
			sok = -1;
			return false;
		}
		if (size == -1) { // error
			int err = sockerror();
			log("socket error %s\n", sockstrerror(err));
			if (isSocketErrorFatal(err)) {
				closesocket(sok);
				sok = -1;
			}
			return false;
		}

		if (p->size > (int)sizeof(CapturyRequestPacket)) {
			if (p->size > 10000000) {
				log("invalid packet size: %d. closing connection.", p->size);
				closesocket(sok);
				sok = -1;
				return false;
			}

			int at = sizeof(CapturyRequestPacket);
			if (p->size > (int)buffer.size()) {
				buffer.resize(p->size);
				p = (CapturyRequestPacket*)buffer.data();
			}
			int toGet = p->size - at;
			while (toGet > 0) {
				size = recv(sok, &buffer[at], toGet, 0);
				if (size == 0) { // the other end shut down the socket...
					log("socket shut down by other end: %s\n", sockstrerror());
					closesocket(sok);
					sok = -1;
					return false;
				}
				if (size == -1) { // error
					int err = sockerror();
					log("socket error: %s\n", sockstrerror(err));
					if (isSocketErrorFatal(err)) {
						closesocket(sok);
						sok = -1;
					}
					return false;
				}
				at += size;
				toGet = std::min<int>(p->size, buffer.size()) - at;
			}
			size = std::min<int>(p->size, buffer.size());
		}

		// log("received packet size %d type %d (expected %d)\n", size, p->type, expect);

		switch (p->type) {
		case capturyHello:
			handshakeFinished = true;
			break;
		case capturyActors: {
			CapturyActorsPacket* cap = (CapturyActorsPacket*)p;
			log("expecting %d actor packets\n", cap->numActors);
			// if (expect == capturyActors) {
			// 	if (cap->numActors != 0) {
			// 		packetsMissing = cap->numActors;
			// 		expect = capturyActor;
			// 	}
			// }
			// numRetries += packetsMissing;
			break; }
		case capturyCameras: {
			CapturyCamerasPacket* ccp = (CapturyCamerasPacket*)p;
			numCameras = ccp->numCameras;
			// if (expect == capturyCameras) {
			// 	packetsMissing = numCameras;
			// 	expect = capturyCamera;
			// }
			// numRetries += packetsMissing;
			break; }
		case capturyActor:
		case capturyActor2:
		case capturyActor3: {
			CapturyActor_p actor(new CapturyActor);
			CapturyActorPacket* cap = (CapturyActorPacket*)p;
			strncpy(actor->name, cap->name, sizeof(actor->name));
			actor->id = cap->id;
			actor->numJoints = cap->numJoints;
			actor->joints = new CapturyJoint[actor->numJoints];
			char* at = (char*)cap->joints;
			char* end = &buffer[size];
			int version = (p->type == capturyActor) ? 1 : (p->type == capturyActor2) ? 2 : 3;

			int numTransmittedJoints = 0;
			for (int j = 0; at < end; ++j) {
				switch (version) {
				case 1: {
					CapturyJointPacket* jp = (CapturyJointPacket*)at;
					actor->joints[j].parent = jp->parent;
					for (int x = 0; x < 3; ++x) {
						actor->joints[j].offset[x] = jp->offset[x];
						actor->joints[j].orientation[x] = jp->orientation[x];
						actor->joints[j].scale[x] = 1.0f;
					}
					strncpy(actor->joints[j].name, jp->name, sizeof(actor->joints[j].name));
					at += sizeof(CapturyJointPacket);
					break; }
				case 2: {
					CapturyJointPacket2* jp = (CapturyJointPacket2*)at;
					actor->joints[j].parent = jp->parent;
					for (int x = 0; x < 3; ++x) {
						actor->joints[j].offset[x] = jp->offset[x];
						actor->joints[j].orientation[x] = jp->orientation[x];
						actor->joints[j].scale[x] = 1.0f;
					}
					strncpy(actor->joints[j].name, jp->name, sizeof(actor->joints[j].name)-1);
					at += sizeof(CapturyJointPacket2) + strlen(jp->name) + 1;
					break; }
				case 3: {
					CapturyJointPacket3* jp = (CapturyJointPacket3*)at;
					actor->joints[j].parent = jp->parent;
					for (int x = 0; x < 3; ++x) {
						actor->joints[j].offset[x] = jp->offset[x];
						actor->joints[j].orientation[x] = jp->orientation[x];
						actor->joints[j].scale[x] = jp->scale[x];
					}
					strncpy(actor->joints[j].name, jp->name, sizeof(actor->joints[j].name)-1);
					at += sizeof(CapturyJointPacket3) + strlen(jp->name) + 1;
					break; }
				}
				numTransmittedJoints = j + 1;
			}
			actor->numBlendShapes = 0;
			/*int numTransmittedJoints = std::min<int>((cap->size - sizeof(CapturyActorPacket)) / sizeof(CapturyJointPacket), actor->numJoints);
			for (int j = 0; j < numTransmittedJoints; ++j) {
				strcpy(actor->joints[j].name, cap->joints[j].name);
				actor->joints[j].parent = cap->joints[j].parent;
				for (int x = 0; x < 3; ++x) {
					actor->joints[j].offset[x] = cap->joints[j].offset[x];
					actor->joints[j].orientation[x] = cap->joints[j].orientation[x];
				}
			}*/
			for (int j = numTransmittedJoints; j < actor->numJoints; ++j) { // initialize to default values
				strncpy(actor->joints[j].name, "uninitialized", sizeof(actor->joints[j].name));
				actor->joints[j].parent = 0;
				for (int x = 0; x < 3; ++x) {
					actor->joints[j].offset[x] = 0;
					actor->joints[j].orientation[x] = 0;
				}
			}
			if (numTransmittedJoints < actor->numJoints) {
				// expect = (version == 1) ? capturyActorContinued : (version == 2) ? capturyActorContinued2 : capturyActorContinued3;
				// numRetries += 1;
			}
			log("received actor %x (%d/%d)\n", actor->id, numTransmittedJoints, actor->numJoints);
			p->type = capturyActor;
			if (numTransmittedJoints == actor->numJoints) {
				//log("received fulll actor %d\n", actor->id);
				lockMutex(&mutex);
				actorsById[actor->id] = actor;
				unlockMutex(&mutex);
				if (actorChangedCallback)
					actorChangedCallback(actor->id, actorData[actor->id].status);
			} else {
				lockMutex(&partialActorMutex);
				partialActors[actor->id] = actor;
				unlockMutex(&partialActorMutex);
			}
			break; }
		case capturyActorContinued:
		case capturyActorContinued2:
		case capturyActorContinued3: {
			int version = (p->type == capturyActor) ? 1 : (p->type == capturyActor2) ? 2 : 3;
			CapturyActorContinuedPacket* cacp = (CapturyActorContinuedPacket*)p;
			lockMutex(&partialActorMutex);
			if (partialActors.count(cacp->id) == 0) {
				unlockMutex(&partialActorMutex);
				break;
			}

			CapturyActor_p actor = partialActors[cacp->id];
			unlockMutex(&partialActorMutex);

			int j = cacp->startJoint;
			switch (version) {
			case 1: {
				CapturyJointPacket* end = (CapturyJointPacket*)&buffer[size];
				for (int k = 0; j < actor->numJoints && &cacp->joints[k] < end; ++j, ++k) {
					strncpy(actor->joints[j].name, cacp->joints[k].name, sizeof(actor->joints[j].name)-1);
					actor->joints[j].parent = cacp->joints[k].parent;
					for (int x = 0; x < 3; ++x) {
						actor->joints[j].offset[x] = cacp->joints[k].offset[x];
						actor->joints[j].orientation[x] = cacp->joints[k].orientation[x];
						actor->joints[j].scale[x] = 1.0f;
					}
				}
				break; }
			case 2: {
				char* at = (char*)cacp->joints;
				char* end = (char*)&buffer[size];
				for ( ; j < actor->numJoints && at < end; ++j) {
					CapturyJointPacket2* jp = (CapturyJointPacket2*)at;
					actor->joints[j].parent = jp->parent;
					for (int x = 0; x < 3; ++x) {
						actor->joints[j].offset[x] = jp->offset[x];
						actor->joints[j].orientation[x] = jp->orientation[x];
						actor->joints[j].scale[x] = 1.0f;
					}
					strncpy(actor->joints[j].name, jp->name, sizeof(actor->joints[j].name)-1);
					at += sizeof(CapturyJointPacket2) + strlen(jp->name) + 1;
				}
				break; }
			case 3: {
				char* at = (char*)cacp->joints;
				char* end = (char*)&buffer[size];
				for ( ; j < actor->numJoints && at < end; ++j) {
					CapturyJointPacket3* jp = (CapturyJointPacket3*)at;
					actor->joints[j].parent = jp->parent;
					for (int x = 0; x < 3; ++x) {
						actor->joints[j].offset[x] = jp->offset[x];
						actor->joints[j].orientation[x] = jp->orientation[x];
						actor->joints[j].scale[x] = jp->scale[x];
					}
					strncpy(actor->joints[j].name, jp->name, sizeof(actor->joints[j].name)-1);
					at += sizeof(CapturyJointPacket3) + strlen(jp->name) + 1;
				}
				break; }
			}
			if (j == actor->numJoints) {
				// log("received fulll actor %d\n", actor->id);
				lockMutex(&mutex);
				actorsById[actor->id] = actor;
				unlockMutex(&mutex);
				if (actorChangedCallback)
					actorChangedCallback(actor->id, actorData[actor->id].status);
				lockMutex(&partialActorMutex);
				partialActors.erase(actor->id);
				unlockMutex(&partialActorMutex);
			} else {
				// expect is already set correctly
				// numRetries += 1;
				// packetsMissing += 1;
			}
			log("received actor cont %d (%d/%d)\n", actor->id, j, actor->numJoints);
			break; }
		case capturyActorBlendShapes: {
			CapturyActorBlendShapesPacket* cabs = (CapturyActorBlendShapesPacket*)p;
			lockMutex(&mutex);
			std::shared_ptr<CapturyActor> actor = actorsById[cabs->actorId];
			actor->numBlendShapes = cabs->numBlendShapes;
			actor->blendShapes = new CapturyBlendShape[actor->numBlendShapes];
			char* at = cabs->blendShapeNames;
			for (int i = 0; i < actor->numBlendShapes; ++i) {
				strncpy(actor->blendShapes[i].name, at, 63);
				actor->blendShapes[i].name[63] = '\0';
				at += std::min<int>(strlen(actor->blendShapes[i].name) + 1, 64);
			}
			unlockMutex(&mutex);
			break; }
		case capturyCamera: {
			CapturyCamera camera;
			CapturyCameraPacket* ccp = (CapturyCameraPacket*)p;
			strncpy(camera.name, ccp->name, sizeof(camera.name));
			camera.id = ccp->id;
			for (int x = 0; x < 3; ++x) {
				camera.position[x] = ccp->position[x];
				camera.orientation[x] = ccp->orientation[x];
			}
			camera.sensorSize[0] = ccp->sensorSize[0];
			camera.sensorSize[1] = ccp->sensorSize[1];
			camera.focalLength = ccp->focalLength;
			camera.lensCenter[0] = ccp->lensCenter[0];
			camera.lensCenter[1] = ccp->lensCenter[1];
			strncpy(camera.distortionModel, "none", sizeof(camera.distortionModel));
			memset(&camera.distortion[0], 0, sizeof(camera.distortion));

			// TODO compute extrinsic and intrinsic matrix

			lockMutex(&mutex);
			cameras.push_back(camera);
			unlockMutex(&mutex);
			break; }
		case capturyPose:
		case capturyPose2:
			log("received pose on control socket\n");
			break;
		case capturyDaySessionShot: {
			CapturyDaySessionShotPacket* dss = (CapturyDaySessionShotPacket*)p;
			currentDay = dss->day;
			currentSession = dss->session;
			currentShot = dss->shot;
			break; }
		case capturyTime2: {
			CapturyTimePacket2* tp = (CapturyTimePacket2*)p;
			if (tp->timeId != nextTimeId) {
				log("time id doesn't match, expected %d got %d", nextTimeId, tp->timeId);
				p->type = capturyError;
				break;
			}
			} // fall through
		case capturyTime: {
			CapturyTimePacket* tp = (CapturyTimePacket*)p;
			uint64_t pongTime = getTime();
			// we assume that the network transfer time is symmetric
			// so the timestamp given in the packet was captured at (pingTime + pongTime) / 2
			uint64_t t = (pongTime - pingTime) / 2 + pingTime;
			syncSamples.emplace_back(t, tp->timestamp, (uint32_t)(pongTime - pingTime));
			if (syncSamples.size() > 50)
				syncSamples.erase(syncSamples.begin());
			updateSync(t);
			log("local: %" PRIu64 " remote: %" PRIu64 " => offset %" PRId64 ", roundtrip %" PRId64 "\n", t, tp->timestamp, tp->timestamp - t, pongTime - pingTime);
			break; }
		case capturyCustom: {
			CapturyCustomPacket* ccp = (CapturyCustomPacket*)p;
			ccp->name[15] = 0;
			std::map<std::string, CapturyCustomPacketCallback>::iterator it = callbacks.find(ccp->name);
			if (it == callbacks.end()) // no callback for this string
				break;
			it->second(ccp->size - sizeof(CapturyCustomPacketCallback), ccp->data);
			break; }
		case capturyEnableRemoteLogging:
			doRemoteLogging = true;
			break;
		case capturyDisableRemoteLogging:
			doRemoteLogging = false;
			break;
		case capturyImageHeader: {
			CapturyImageHeaderPacket* tp = (CapturyImageHeaderPacket*)p;

			// update the image structures
			lockMutex(&mutex);
			if (actorData.count(tp->actor) > 0) {
				free(actorData[tp->actor].currentTextures.data);
				actorData[tp->actor].currentTextures.data = NULL;
			}
			actorData[tp->actor].currentTextures.camera = -1;
			actorData[tp->actor].currentTextures.width = tp->width;
			actorData[tp->actor].currentTextures.height = tp->height;
			actorData[tp->actor].currentTextures.timestamp = 0;
//			log("got image header %dx%d for actor %x\n", currentTextures[tp->actor].width, currentTextures[tp->actor].height, tp->actor);
			actorData[tp->actor].currentTextures.data = (unsigned char*)malloc(tp->width*tp->height*3);
			actorData[tp->actor].receivedPackets = std::vector<int>( ((tp->width*tp->height*3 + tp->dataPacketSize-16-1) / (tp->dataPacketSize-16)), 0);
			unlockMutex(&mutex);

			// and request the data to go with it
			if (sock == -1 || streamSocketPort == 0)
				break;

			CapturyGetImageDataPacket packet;
			packet.type = capturyGetImageData;
			packet.size = sizeof(packet);
			packet.actor = tp->actor;
			packet.port = streamSocketPort;
//			log("requesting image to port %d\n", ntohs(packet.port));

			if (send(sock, (const char*)&packet, packet.size, 0) != packet.size)
				break;

			break; }
		case capturyMarkerTransform: {
			CapturyMarkerTransformPacket* cmt = (CapturyMarkerTransformPacket*)p;
			ActorAndJoint aj(cmt->actor, cmt->joint);
			MarkerTransform& mt = markerTransforms[aj];
			mt.timestamp = cmt->timestamp;
			mt.trafo.translation[0] = cmt->translation[0];
			mt.trafo.translation[1] = cmt->translation[1];
			mt.trafo.translation[2] = cmt->translation[2];
			mt.trafo.rotation[0] = cmt->rotation[0];
			mt.trafo.rotation[1] = cmt->rotation[1];
			mt.trafo.rotation[2] = cmt->rotation[2];
			break; }
		case capturyScalingProgress: {
			CapturyScalingProgressPacket* spp = (CapturyScalingProgressPacket*)p;
			lockMutex(&mutex);
			if (actorData.count(spp->actor))
				actorData[spp->actor].scalingProgress = spp->progress;
			unlockMutex(&mutex);
			break; }
		case capturyBackgroundQuality: {
			CapturyBackgroundQualityPacket* bqp = (CapturyBackgroundQualityPacket*)p;
			backgroundQuality = bqp->quality;
			break; }
		case capturyStatus: {
			CapturyStatusPacket* sp = (CapturyStatusPacket*)p;
			lastStatusMessage = sp->message; // FIXME this is unsafe. assumes that message is 0 terminated.
			break; }
		case capturyStartRecordingAck2: {
			CapturyTimePacket* srp = (CapturyTimePacket*)p;
			startRecordingTime = srp->timestamp;
			break; }
		case capturyActorModeChanged: {
			CapturyActorModeChangedPacket* amc = (CapturyActorModeChangedPacket*)p;
			if (actorChangedCallback != NULL)
				actorChangedCallback(amc->actor, amc->mode);
			lockMutex(&mutex);
			if (actorData.count(amc->actor))
				actorData[amc->actor].status = (CapturyActorStatus)amc->mode;
			unlockMutex(&mutex);
			break; }
		case capturyStreamAck:
		case capturySetShotAck:
		case capturyStartRecordingAck:
		case capturyStopRecordingAck:
		case capturyCustomAck:
			break; // all good
		default:
			log("unrecognized packet: %d bytes, type %d, size %d", size, p->type, p->size);
			break;
		}

		// if (p->type == expect) {
		// 	--packetsMissing;

		// 	if (packetsMissing == 0)
		// 		break;
		// }
	}

	return true;
}

static void deleteActors()
{
	log("deleting all actors\n");
	lockMutex(&mutex);
	for (auto it : actorsById)
		delete[] it.second->joints;
	actorsById.clear();

	std::vector<int> deletedActorIds;
	std::unordered_map<int, ActorData>::iterator it;
	for (it = actorData.begin(); it != actorData.end(); ++it) {
		if (it->second.currentPose.numTransforms != 0) {
			if (it->second.currentPose.numTransforms)
				delete[] it->second.currentPose.transforms;
			it->second.currentPose.numTransforms = 0; // should not be necessary but weird things do happen
			if (it->second.currentPose.numBlendShapes)
				delete[] it->second.currentPose.blendShapeActivations;
			it->second.currentPose.numBlendShapes = 0; // should not be necessary but weird things do happen
			it->second.currentPose.transforms = NULL;
		}
		if (it->second.currentTextures.data != NULL)
			free(it->second.currentTextures.data);

		if (actorChangedCallback)
			deletedActorIds.push_back(it->first);
	}
	actorData.clear();
	unlockMutex(&mutex);

	for (int id : deletedActorIds)
		actorChangedCallback(id, ACTOR_DELETED);
}

#ifdef WIN32
static DWORD WINAPI receiveLoop(void* /*arg*/)
#else
static void* receiveLoop(void* arg)
#endif
{
	bool handshaking = !handshakeFinished;
	log("starting receive loop\n");
	while (!stopReceiving && (!handshaking || !handshakeFinished)) {
		if (!receive(sock)) {
			if (sock == -1) {
				deleteActors();

				if (isStreamThreadRunning) {
					stopStreamThread = 1;

					#ifdef WIN32
					WaitForSingleObject(streamThread, 1000);
					#else
					void* retVal;
					pthread_join(streamThread, &retVal);
					#endif
				}

				while (!stopReceiving) {
					sock = openTcpSocket();
					if (sock != -1)
						break;

					sleepMicroSeconds(100000);
				}

				if (streamWhat != CAPTURY_STREAM_NOTHING)
					Captury_startStreamingImagesAndAngles(streamWhat, streamCamera, (int)streamAngles.size(), streamAngles.data());

				handshaking = false; // this is a lie but makes it go into the normal loop
			}
		}
	}
	log("stopping receive loop\n");

	return 0;
}

static bool sendPacket(CapturyRequestPacket* packet, CapturyPacketTypes expectedReplyType)
{
	if (send(sock, (const char*)packet, packet->size, 0) != packet->size)
		return false;

	return true;
}

static void receivedPose(CapturyPose* pose, int actorId, ActorData* aData, uint64_t timestamp) REQUIRES(mutex)
{
	if (aData->status == ACTOR_DELETED)
		return;

	if (getLocalPoses)
		Captury_convertPoseToLocal(pose, actorId);

	pose->timestamp = timestamp;

	uint64_t now = getTime();
	// log("received pose %ld at %ld, diff %ld\n", pose->timestamp, now, now - aData->lastPoseTimestamp);
	aData->lastPoseTimestamp = now;

	mostRecentPoseReceivedTime = getRemoteTime(now);
	mostRecentPoseReceivedTimestamp = timestamp;

	if (aData->status != ACTOR_SCALING && aData->status != ACTOR_TRACKING) {
		aData->status = ACTOR_TRACKING;
		if (actorChangedCallback) {
			unlockMutex(&mutex);
			actorChangedCallback(actorId, ACTOR_TRACKING);
			lockMutex(&mutex);
		}
	}

	if (newPoseCallback != NULL && actorsById.count(actorId)) {
		CapturyActor* actor = actorsById[actorId].get();
		unlockMutex(&mutex);
		newPoseCallback(actor, pose, aData->trackingQuality);
		lockMutex(&mutex);
	}

	// mark actors as stopped if no data was received for a while
	now -= 500000; // half a second ago
	std::vector<int> stoppedActorIds;
	for (std::unordered_map<int, ActorData>::iterator it = actorData.begin(); it != actorData.end(); ++it) {
		if (it->second.lastPoseTimestamp > now) // still current
			continue;

		if (it->second.status == ACTOR_SCALING || it->second.status == ACTOR_TRACKING) {
			if (actorChangedCallback)
				stoppedActorIds.push_back(it->first);
			it->second.status = ACTOR_STOPPED;
		}
	}

	unlockMutex(&mutex);
	for (int id : stoppedActorIds)
		actorChangedCallback(id, ACTOR_STOPPED);
}

static void decompressPose(CapturyPose* pose, uint8_t* v, CapturyActor* actor)
{
	float* copyTo = (float*)pose->transforms;
	float* values = (float*)pose->transforms;
	int numJoints = (actor->numJoints < pose->numTransforms) ? actor->numJoints : pose->numTransforms;
	for (int i = 0; i < numJoints; ++i) {
		if (i == 0) {
			int32_t t = v[0] | (v[1] << 8) | (v[2] << 16);
			if ((t & 0x800000) != 0)
				t |= 0xFF000000;
			copyTo[0] = t * 0.0625f;
			v += 3;
			t = v[0] | (v[1] << 8) | (v[2] << 16);
			if ((t & 0x800000) != 0)
				t |= 0xFF000000;
			copyTo[1] = t * 0.0625f;
			v += 3;
			t = v[0] | (v[1] << 8) | (v[2] << 16);
			if ((t & 0x800000) != 0)
				t |= 0xFF000000;
			copyTo[2] = t * 0.0625f;
			v += 3;
		} else {
			int32_t t = v[0] | (v[1] << 8);
			if ((t & 0x8000) != 0)
				t |= 0xFFFF0000;
			copyTo[0] = t * 0.0625f + values[actor->joints[i].parent*6];
			v += 2;
			t = v[0] | (v[1] << 8);
			if ((t & 0x8000) != 0)
				t |= 0xFFFF0000;
			copyTo[1] = t * 0.0625f + values[actor->joints[i].parent*6+1];
			v += 2;
			t = v[0] | (v[1] << 8);
			if ((t & 0x8000) != 0)
				t |= 0xFFFF0000;
			copyTo[2] = t * 0.0625f + values[actor->joints[i].parent*6+2];
			v += 2;
		}

		// decompress rotation
		uint32_t rall = *(uint32_t*)v;
		v += 4;
		copyTo[3] = ((rall & 0x000007FF))       * (360.0f / 2047) - 180.0f;
		copyTo[4] = ((rall & 0x003FF800) >> 11) * (360.0f / 2047) - 180.0f;
		copyTo[5] = ((rall & 0xFFC00000) >> 22) * (180.0f / 1023);
		copyTo += 6;
	}

	for (int i = 0; i < pose->numBlendShapes; ++i, v += 2)
		pose->blendShapeActivations[i] = (*(uint16_t*)v) / 32768.0f;
}

#ifdef WIN32
static DWORD WINAPI streamLoop(void* arg)
#else
static void* streamLoop(void* arg)
#endif
{
	isStreamThreadRunning = true;

	CapturyStreamPacketTcp* packet = (CapturyStreamPacketTcp*)arg;

	SOCKET streamSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (streamSock == -1) {
		log("failed to create stream socket\n");
		free(packet);
		return 0;
	}

	if (bind(streamSock, (sockaddr*) &localStreamAddress, sizeof(localStreamAddress)) != 0) {
		closesocket(streamSock);
		log("failed to bind stream socket\n");
		free(packet);
		return 0;
	}

	if (connect(streamSock, (sockaddr*) &remoteAddress, sizeof(remoteAddress)) != 0) {
		closesocket(streamSock);
		log("failed to connect stream socket\n");
		free(packet);
		return 0;
	}

	// send dummy packet to trick some firewalls into letting us through
	CapturyTimePacket timePacket = {capturyTime, sizeof(CapturyTimePacket), getTime()};
	send(streamSock, (const char*)&timePacket, sizeof(timePacket), 0);

	uint64_t lastKeepAliveTime = getTime();

	int sockBufSize = 500000;
	socklen_t optSize = sizeof(sockBufSize);
	getsockopt(streamSock, SOL_SOCKET, SO_RCVBUF, (char*)&sockBufSize, &optSize);

	// set read timeout
	setSocketTimeout(streamSock, 100);

	struct sockaddr_in thisEnd;
	{
		socklen_t len = sizeof(thisEnd);
		getsockname(streamSock, (sockaddr*) &thisEnd, &len);
		streamSocketPort = thisEnd.sin_port;

		#ifdef WIN32
		 	log("Stream receiving on %d.%d.%d.%d:%d\n", thisEnd.sin_addr.S_un.S_un_b.s_b1, thisEnd.sin_addr.S_un.S_un_b.s_b2, thisEnd.sin_addr.S_un.S_un_b.s_b3, thisEnd.sin_addr.S_un.S_un_b.s_b4, ntohs(thisEnd.sin_port));
		#else
			char buf[100];
			log("stream receiving on %s:%d\n", inet_ntop(AF_INET, &thisEnd.sin_addr, buf, 100), ntohs(thisEnd.sin_port));
		#endif
	}

	packet->ip = thisEnd.sin_addr.s_addr;
	packet->port = thisEnd.sin_port;

	//	log("stream packet has size %d\n", packet->size);

	// send request on TCP socket
	if (send(sock, (const char*)packet, packet->size, 0) != packet->size) {
		lastErrorMessage = "Failed to start streaming";
		return 0;
	}

	while (!stopStreamThread) {
/*		fd_set reader;
		FD_ZERO(&reader);
		FD_SET(streamSock, &reader);

		// keep the streaming alive

		tv.tv_sec = 0;
		tv.tv_usec = 500000; // 0.5s = 2Hz
		int ret = select((int)(MAXIMUM(sock, streamSock)+1), &reader, NULL, NULL, &tv);
		if (ret == -1) { // error
			log("error waiting for stream socket\n");
			lastErrorMessage = "Error waiting for stream socket";
			return 0;
		}
		if (ret == 0) {
			lastErrorMessage = "Stream timed out";
			continue;
		}*/

		dataAvailableTime = getRemoteTime(getTime());

		std::vector<char> buffer(10000);
		CapturyPosePacket* cpp = (CapturyPosePacket*)buffer.data();

		// first peek to find out which packet type this is
		int size = recv(streamSock, buffer.data(), (int)buffer.size(), 0);
//		log("received stream packet size %d (%d %d)\n", (int) size, cpp->type, cpp->size);
		if (size == 0) { // the other end shut down the socket...
			lastErrorMessage = "Stream socket closed unexpectedly";
			continue;
		}
		if (size == -1) { // error
			int err = sockerror();
			if (isSocketErrorTryAgain(err)) {
				#ifdef WIN32
				if (err == 0) {
					u_long read;
					ioctlsocket(streamSock, FIONREAD, &read); // returns fill status of socket buffer...
					log("socket filled %.1f%%", (read * 100.0f) / sockBufSize);
				}
				#endif

				// renew the keep-alive firewall hole punching
				uint64_t now = getTime();
				if (now > lastKeepAliveTime + 1000000) {
					timePacket.timestamp = now;
					send(streamSock, (const char*)&timePacket, sizeof(timePacket), 0);
					lastKeepAliveTime = now;
				}

				continue;
			}
			char buff[200];
			sprintf(buff, "Stream socket error: %s", sockstrerror(err));
			lastErrorMessage = buff;
			log("streaming error: %s\n", buff);
			break;
		}

		dataReceivedTime = Captury_getTime(); // get remote time

		if (cpp->type == capturyImageData) {
			// received data for the image
			CapturyImageDataPacket* cip = (CapturyImageDataPacket*)buffer.data();
			//log("received image data for actor %x (payload %d bytes)\n", cip->actor, cip->size-16);

			// check if we have a texture already
			lockMutex(&mutex);
			std::unordered_map<int, ActorData>::iterator it = actorData.find(cip->actor);
			if (it == actorData.end()) {
				unlockMutex(&mutex);
				log("received image data for actor %x without having received image header\n", cip->actor);
				continue;
			}

			// copy data from packet into the buffer
			const int imgSize = it->second.currentTextures.width * it->second.currentTextures.height * 3;

			// check if packet fits
			if (cip->offset >= imgSize || cip->offset + cip->size-16 > imgSize) {
				unlockMutex(&mutex);
				log("received image data for actor %x (%d-%d) that is larger than header (%dx%d*3 = %d)\n", cip->actor, cip->offset, cip->offset+cip->size-16, it->second.currentTextures.width, it->second.currentTextures.height, imgSize);
				continue;
			}

			// mark paket as received
			const int packetIndex = cip->offset / (cip->size-16);
			actorData[cip->actor].receivedPackets[packetIndex] = 1;

			// copy data
			memcpy(it->second.currentTextures.data + cip->offset, cip->data, cip->size-16);
			unlockMutex(&mutex);

			continue;
		}

		if (cpp->type == capturyStreamedImageHeader) {
			CapturyImageHeaderPacket* tp = (CapturyImageHeaderPacket*)buffer.data();

			// update the image structures
			lockMutex(&mutex);
			if (currentImages.count(tp->actor) == 0) {
				currentImages[tp->actor].camera = tp->actor;
				currentImages[tp->actor].width = tp->width;
				currentImages[tp->actor].height = tp->height;
				currentImages[tp->actor].timestamp = 0;
				currentImages[tp->actor].data = (unsigned char*)malloc(tp->width*tp->height*3);
			} else if (currentImages[tp->actor].width != tp->width || currentImages[tp->actor].height != tp->height)
				currentImages[tp->actor].data = (unsigned char*)realloc(currentImages[tp->actor].data, tp->width*tp->height*3);

			currentImagesReceivedPackets[tp->actor] = std::vector<int>( ((tp->width*tp->height*3 + tp->dataPacketSize-16-1) / (tp->dataPacketSize-16)) + 1, 0);
			unlockMutex(&mutex);

			// and request the data to go with it
			if (sock != -1 && streamSocketPort != 0) {
				CapturyGetImageDataPacket imPacket;
				imPacket.type = capturyGetStreamedImageData;
				imPacket.size = sizeof(imPacket);
				imPacket.actor = tp->actor;
				imPacket.port = streamSocketPort;
				if (send(sock, (const char*)&imPacket, imPacket.size, 0) != imPacket.size)
					log("cannot request streamed image data\n");
			}
			continue;
		}

		if (cpp->type == capturyStreamedImageData) {
			// received data for the image
			CapturyImageDataPacket* cip = (CapturyImageDataPacket*)buffer.data();
//			log("received image data for camera %d (payload %d bytes)\n", cip->actor, cip->size-16);

			// check if we have a texture already
			std::map<int, CapturyImage>::iterator it = currentImages.find(cip->actor);
			if (it == currentImages.end()) {
				log("received image data for camera %d without having received image header\n", cip->actor);
				continue;
			}

			// copy data from packet into the buffer
			lockMutex(&mutex);
			const int imgSize = it->second.width * it->second.height * 3;

			// check if packet fits
			if (cip->offset >= imgSize || cip->offset + cip->size-16 > imgSize) {
				unlockMutex(&mutex);
				log("received image data for camera %d (%d-%d) that is larger than header (%dx%d*3 = %d)\n", cip->actor, cip->offset, cip->offset+cip->size-16, it->second.width, it->second.height, imgSize);
				continue;
			}

			bool finished = false;

			// mark paket as received
			const int packetIndex = cip->offset / (cip->size-16);
			std::vector<int>& recvd = currentImagesReceivedPackets[cip->actor];
			if (recvd[packetIndex] == 1) { // copying image to done although it is not quite finished
				auto done = currentImagesDone.find(cip->actor);
				if (done == currentImagesDone.end()) {
					currentImagesDone[cip->actor].camera = it->second.camera;
					currentImagesDone[cip->actor].data = (unsigned char*)malloc(it->second.width*it->second.height*3);
					done = currentImagesDone.find(cip->actor);
				}
				done->second.width = it->second.width;
				done->second.height = it->second.height;
				done->second.timestamp = it->second.timestamp;
				std::swap(done->second.data, it->second.data);
				std::fill(recvd.begin(), recvd.end(), 0);
				finished = true;
			}
			recvd[packetIndex] = 1;
			++recvd[recvd.size()-1];

			// copy data
			memcpy(it->second.data + cip->offset, cip->data, cip->size-16);

			if (recvd[recvd.size()-1] == (int)recvd.size()-2) { // done
				auto done = currentImagesDone.find(cip->actor);
				if (done == currentImagesDone.end()) {
					currentImagesDone[cip->actor].camera = it->second.camera;
					currentImagesDone[cip->actor].data = (unsigned char*)malloc(it->second.width*it->second.height*3);
					done = currentImagesDone.find(cip->actor);
				}
				done->second.width = it->second.width;
				done->second.height = it->second.height;
				done->second.timestamp = it->second.timestamp;
				std::swap(done->second.data, it->second.data);
				std::fill(recvd.begin(), recvd.end(), 0);
				finished = true;
			}
			unlockMutex(&mutex);

			if (finished && imageCallback)
				imageCallback(&currentImagesDone[cip->actor]);

			continue;
		}

		if (cpp->type == capturyARTag) {
			//log("received ARTag message\n");
			CapturyARTagPacket* art = (CapturyARTagPacket*)buffer.data();
			lockMutex(&mutex);
			arTagsTime = getTime();
			arTags.resize(art->numTags);
			memcpy(&arTags[0], &art->tags[0], sizeof(CapturyARTag) * art->numTags);
			//for (int i = 0; i < art->numTags; ++i)
			//	log("  id %d: orient % 4.1f,% 4.1f,% 4.1f\n", art->tags[i].id, art->tags[i].transform.rotation[0], art->tags[i].transform.rotation[1], art->tags[i].transform.rotation[2]);
			unlockMutex(&mutex);
			if (arTagCallback != NULL)
				arTagCallback(art->numTags, &art->tags[0]);
			continue;
		}

		if (cpp->type == capturyAngles) {
			CapturyAnglesPacket* ang = (CapturyAnglesPacket*)buffer.data();
			if (newAnglesCallback != NULL)
				newAnglesCallback(Captury_getActor(ang->actor), ang->numAngles, ang->angles);
			lockMutex(&mutex);
			currentAngles[ang->actor].resize(ang->numAngles);
			for (int i = 0; i < ang->numAngles; ++i)
				currentAngles[ang->actor][i] = *(CapturyAngleData*)((char*)ang->angles + sizeof(CapturyAngleData) * i);
			unlockMutex(&mutex);
			continue;
		}

		if (cpp->type == capturyActorModeChanged) {
			CapturyActorModeChangedPacket* amc = (CapturyActorModeChangedPacket*)buffer.data();
			log("received actorModeChanged packet %x %d\n", amc->actor, amc->mode);
			if (actorChangedCallback != NULL)
				actorChangedCallback(amc->actor, amc->mode);
			lockMutex(&mutex);
			if (actorData.count(amc->actor))
				actorData[amc->actor].status = (CapturyActorStatus)amc->mode;
			unlockMutex(&mutex);
			continue;
		}
		if (cpp->type == capturyPoseCont || cpp->type == capturyCompressedPoseCont) {
			lockMutex(&mutex);
			if (actorsById.count(cpp->actor) == 0) {
				char buff[400];
				snprintf(buff, 400, "pose continuation: Actor %d does not exist", cpp->actor);
				lastErrorMessage = buff;
				unlockMutex(&mutex);
				continue;
			}

			std::unordered_map<int, ActorData>::iterator it = actorData.find(cpp->actor);
			ActorData& aData = it->second;
			int inProgressIndex = -1;
			for (int x = 0; x < 4; ++x) {
				if (cpp->timestamp == aData.inProgress[x].timestamp) {
					inProgressIndex = x;
					break;
				}
			}
			if (inProgressIndex == -1) {
				lastErrorMessage = "pose continuation packet for wrong timestamp";
				unlockMutex(&mutex);
				continue;
			}

			CapturyPoseCont* cpc = (CapturyPoseCont*)cpp;

			int numBytesToCopy = size - (int)((char*)cpc->values - (char*)cpc);
			int numJoints = actorsById[cpp->actor]->numJoints;
			int numBlendShapes = actorsById[cpp->actor]->numBlendShapes;
			int totalBytes = (numJoints * 6 + numBlendShapes) * sizeof(float);
			if (aData.inProgress[inProgressIndex].bytesDone + numBytesToCopy > totalBytes) {
				lastErrorMessage = "pose continuation too large";
				unlockMutex(&mutex);
				continue;
			}

			char* at = ((char*)aData.inProgress[inProgressIndex].pose) + aData.inProgress[inProgressIndex].bytesDone;
			memcpy(at, cpc->values, numBytesToCopy);
			aData.inProgress[inProgressIndex].bytesDone += numBytesToCopy;

			if (aData.inProgress[inProgressIndex].bytesDone == totalBytes) {
				if (cpp->type == capturyCompressedPoseCont)
					decompressPose(&aData.currentPose, (uint8_t*)aData.inProgress[inProgressIndex].pose, actorsById[cpp->actor].get());
				else {
					memcpy(aData.currentPose.transforms, aData.inProgress[inProgressIndex].pose, numJoints * 6 * sizeof(float));
					memcpy(aData.currentPose.blendShapeActivations, aData.inProgress[inProgressIndex].pose + numJoints * 6 * sizeof(float), numBlendShapes * sizeof(float));
				}
				receivedPose(&aData.currentPose, cpc->actor, &actorData[cpc->actor], aData.inProgress[inProgressIndex].timestamp);
			}
			unlockMutex(&mutex);
			continue;
		}

		if (cpp->type == capturyLatency) {
			CapturyLatencyPacket* lp = (CapturyLatencyPacket*)buffer.data();
			lockMutex(&mutex);
			currentLatency = *lp;
			if (mostRecentPoseReceivedTimestamp == currentLatency.poseTimestamp) {
				receivedPoseTime = mostRecentPoseReceivedTime;
				receivedPoseTimestamp = mostRecentPoseReceivedTimestamp;
			} else {
				receivedPoseTime = 0; // most recent one doesn't match
				receivedPoseTimestamp = 0;
			}
			unlockMutex(&mutex);
			log("latency received %" PRIu64 ", %" PRIu64 " - %" PRIu64 ", %" PRIu64 " - %" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n", lp->firstImagePacket, lp->optimizationStart, lp->optimizationEnd, lp->sendPacketTime, dataAvailableTime, dataReceivedTime, receivedPoseTime);
			continue;
		}

		if (cpp->type != capturyPose && cpp->type != capturyPose2 && cpp->type != capturyCompressedPose && cpp->type != capturyCompressedPose2) {
			log("stream socket received unrecognized packet %d\n", cpp->type);
			continue;
		}

		lockMutex(&mutex);
		if (actorsById.count(cpp->actor) == 0) {
			char buff[400];
			snprintf(buff, 400, "Actor %x does not exist", cpp->actor);
			lastErrorMessage = buff;
			unlockMutex(&mutex);
			continue;
		}

		int numValues;
		float* values;
		int at;
		if (cpp->type == capturyPose || cpp->type == capturyCompressedPose) {
			// log("actor %x: %d values: %g\n", cpp->actor, cpp->numValues, cpp->values[0]);
			numValues = cpp->numValues;
			values = cpp->values;
			at = (int)((char*)(cpp->values) - (char*)cpp);
		} else { // capturyPose2 || capturyCompressedPose2
			numValues = ((CapturyPosePacket2*)cpp)->numValues;
			values = ((CapturyPosePacket2*)cpp)->values;
			at = (int)((char*)(((CapturyPosePacket2*)cpp)->values) - (char*)cpp);
		}

		int numTransforms = std::min<int>(numValues / 6, actorsById[cpp->actor]->numJoints);
		int numBlendShapes = std::min<int>(numValues - numTransforms*6, actorsById[cpp->actor]->numBlendShapes);

		if (actorsById.count(cpp->actor) != 0 && actorsById[cpp->actor]->numJoints * 6 + actorsById[cpp->actor]->numBlendShapes != numValues) {
			if (actorsById[cpp->actor]->numJoints * 6 == numValues)
				numBlendShapes = 0;
			else {
				log("expected %d+%d dofs, got %d\n", actorsById[cpp->actor]->numJoints * 6, actorsById[cpp->actor]->numBlendShapes, numValues);
				unlockMutex(&mutex);
				continue;
			}
		}

		std::unordered_map<int, ActorData>::iterator it = actorData.find(cpp->actor);
		if (it == actorData.end() || (it->second.currentPose.numTransforms == 0 && it->second.currentPose.numBlendShapes == 0)) {
			it = actorData.insert(std::make_pair(cpp->actor, ActorData())).first;
			it->second.currentPose.actor = cpp->actor;
			it->second.currentPose.numTransforms = numTransforms;
			it->second.currentPose.transforms = (numTransforms != 0) ? new CapturyTransform[numTransforms] : nullptr;
			it->second.currentPose.numBlendShapes = numBlendShapes;
			it->second.currentPose.blendShapeActivations = (numBlendShapes != 0) ? new float[numBlendShapes] : nullptr;
		}

		if (cpp->type == capturyPose2 || cpp->type == capturyCompressedPose2) {
			it->second.scalingProgress = ((CapturyPosePacket2*)cpp)->scalingProgress;
			it->second.trackingQuality = ((CapturyPosePacket2*)cpp)->trackingQuality;
			it->second.flags = ((CapturyPosePacket2*)cpp)->flags;
		}

		// select oldest in-progress item
		int inProgressIndex = 0;
		uint64_t oldest = 0xFFFFFFFFFFFFFFFF;
		for (int x = 0; x < 4; ++x) {
			if (it->second.inProgress[x].timestamp < oldest) {
				oldest = it->second.inProgress[x].timestamp;
				inProgressIndex = x;
			}
		}

		// either copy to currentPose or inProgress pose
		int numBytesToCopy = size - at;
		bool done = false;
		if ((cpp->type == capturyPose || cpp->type == capturyPose2) && numBytesToCopy == (int)((numTransforms*6 + numBlendShapes)*sizeof(float))) {
			if (numTransforms != 0)
				memcpy(it->second.currentPose.transforms, values, numTransforms*6*sizeof(float));
			if (numBlendShapes != 0)
				memcpy(it->second.currentPose.blendShapeActivations, values + numTransforms*6, numBlendShapes*sizeof(float));
			done = true;
		} else if ((cpp->type == capturyCompressedPose || cpp->type == capturyCompressedPose2) && numBytesToCopy == (numTransforms-1)*10 + 13 + numBlendShapes * 2) {
			decompressPose(&it->second.currentPose, (uint8_t*)values, actorsById[cpp->actor].get());
			done = true;
		} else {// partial
			if (it->second.inProgress[inProgressIndex].pose == NULL)
				it->second.inProgress[inProgressIndex].pose = new float[numValues];
			memcpy(it->second.inProgress[inProgressIndex].pose, values, numBytesToCopy);
			it->second.inProgress[inProgressIndex].bytesDone = numBytesToCopy;
			it->second.inProgress[inProgressIndex].timestamp = cpp->timestamp;
		}

		if (done)
			receivedPose(&it->second.currentPose, cpp->actor, &it->second, cpp->timestamp);

		unlockMutex(&mutex);

	}

	isStreamThreadRunning = false;

	closesocket(streamSock);

	streamSocketPort = 0;

	log("closing streaming thread\n");

	free(packet);

	return 0;
}

extern "C" int Captury_connect(const char* ip, unsigned short port)
{
	return Captury_connect2(ip, port, 0, 0, 0);
}

// returns 1 if successful, 0 otherwise
extern "C" int Captury_connect2(const char* ip, unsigned short port, unsigned short localPort, unsigned short localStreamPort, int async)
{
#ifdef WIN32
	if (!mutexesInited) {
		InitializeCriticalSection(&mutex);
		InitializeCriticalSection(&partialActorMutex);
		InitializeCriticalSection(&syncMutex);
		InitializeCriticalSection(&logMutex);
		mutexesInited = true;
	}
#endif

	struct in_addr addr;
#ifdef WIN32
	addr.S_un.S_addr = inet_addr(ip);
#else
	if (!inet_pton(AF_INET, ip, &addr))
		return 0;
#endif

	localAddress.sin_family = AF_INET;
	localAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddress.sin_port = htons(localPort);

	localStreamAddress.sin_family = AF_INET;
	localStreamAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	localStreamAddress.sin_port = htons(localStreamPort);

	remoteAddress.sin_family = AF_INET;
	remoteAddress.sin_addr = addr;
	remoteAddress.sin_port = htons(port);

	handshakeFinished = false;
	stopReceiving = 0;

	if (async == 0) {
		if (sock == -1) {
#ifdef WIN32
			if (!wsaInited) {
				WSADATA init;
				const int ret = WSAStartup(WINSOCK_VERSION, &init);
				wsaInited = true;
			}
#endif

			if ((sock = openTcpSocket()) == -1)
				return 0;
		}

		receiveLoop(nullptr); // block until handshake is finished
	}

#ifdef WIN32
	receiveThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveLoop, nullptr, 0, NULL);
#else
	pthread_create(&receiveThread, NULL, receiveLoop, nullptr);
#endif

	return 1;
}

// returns 1 if successful, 0 otherwise
extern "C" int Captury_disconnect()
{
	bool closedOrStopped = false;
	if (sock != -1) {
		closesocket(sock);
		sock = (SOCKET)-1;
		closedOrStopped = true;
	}

	if (!stopReceiving) {
		handshakeFinished = false;
		stopReceiving = 1;
		stopStreamThread = 1;

		#ifdef WIN32
		if (wsaInited) {
			WSACleanup();
			wsaInited = false;
		}

		WaitForSingleObject(receiveThread, 1000);
		WaitForSingleObject(streamThread, 1000);
		#else
		void* retVal;
		pthread_join(receiveThread, &retVal);
		pthread_join(streamThread, &retVal);
		#endif

		closedOrStopped = true;
	}

	deleteActors();

	return closedOrStopped ? 1 : 0;
}

// returns 1 if successful, 0 otherwise
extern "C" int Captury_getConnectionStatus()
{
	if (sock == -1)
		return CAPTURY_DISCONNECTED;
	return (handshakeFinished && stopReceiving == 0) ? CAPTURY_CONNECTED : CAPTURY_CONNECTING;
}

// returns the current number of actors
// the array is owned by the library - do not free
extern "C" int Captury_getActors(const CapturyActor** actrs)
{
	lockMutex(&mutex);

	actorPointers.clear();
	actorPointers.reserve(actorsById.size());

	int numActors = 0;
	for (auto& it : actorsById) {
		if (actorData[it.first].status != ACTOR_DELETED) {
			actorPointers.push_back(*it.second.get());
			++numActors;
		}
	}

	*actrs = (numActors == 0) ? NULL : const_cast<const CapturyActor*>(actorPointers.data());
	unlockMutex(&mutex);

	return numActors;
}

// returns the actor if found or NULL if not
extern "C" const CapturyActor* Captury_getActor(int id)
{
	if (sock == -1)
		return NULL;

	if (id == 0) // invalid id
		return NULL;

	lockMutex(&mutex);
	if (actorsById.count(id) == 0) {
		unlockMutex(&mutex);
		return NULL;
	}

	CapturyActor* ret = actorsById[id].get();
	unlockMutex(&mutex);

	return ret;
}

// returns the number of cameras
// the array is owned by the library - do not free
extern "C" int Captury_getCameras(const CapturyCamera** cams)
{
	if (sock == -1 || cams == NULL)
		return 0;

	CapturyRequestPacket packet;
	packet.type = capturyCameras;
	packet.size = sizeof(packet);

	numCameras = -1;
	cameras.clear();

	if (!sendPacket(&packet, capturyCameras))
		return 0;

	if (cameras.size() > 0)
		*cams = &cameras[0];
	else
		*cams = NULL;
	return (int)cameras.size();
}

// get the last error message
char* Captury_getLastErrorMessage()
{
	lockMutex(&mutex);
	char* msg = new char[lastErrorMessage.size()+1];
	memcpy(msg, &lastErrorMessage[0], lastErrorMessage.size());
	msg[lastErrorMessage.size()] = 0;
	unlockMutex(&mutex);

	return msg;
}

void Captury_freeErrorMessage(char* msg)
{
	free(msg);
}


// returns 1 if successful, 0 otherwise
extern "C" int Captury_startStreaming(int what)
{
	if ((what & CAPTURY_STREAM_IMAGES) != 0)
		return 0;

	return Captury_startStreamingImagesAndAngles(what, -1, 0, nullptr);
}

extern "C" int Captury_startStreamingImages(int what, int32_t camId)
{
	return Captury_startStreamingImagesAndAngles(what, camId, 0, nullptr);
}

extern "C" int Captury_startStreamingImagesAndAngles(int what, int32_t camId, int numAngles, uint16_t* angles)
{
	streamWhat = what;
	streamCamera = camId;
	streamAngles.resize(numAngles);
	memcpy(streamAngles.data(), angles, sizeof(uint16_t) * numAngles);

	if (sock == -1)
		return 0;

	log("start streaming %x, cam %d, %d angles\n", what, camId, numAngles);

	if (what == CAPTURY_STREAM_NOTHING)
		return Captury_stopStreaming();

	if (isStreamThreadRunning)
		Captury_stopStreaming();

	CapturyStreamPacket1Tcp* packet = (CapturyStreamPacket1Tcp*)malloc(sizeof(CapturyStreamPacketTcp) + (numAngles ? (2 + numAngles * 2) : 0));
	packet->type = capturyStream;
	packet->size = sizeof(CapturyStreamPacketTcp) + (numAngles ? (2 + numAngles * 2) : 0);
	if (camId != -1) // only stream images if a camera is specified
		what |= CAPTURY_STREAM_IMAGES;
	else
		what &= ~CAPTURY_STREAM_IMAGES;

	if (numAngles != 0) { // only stream angles if a camera is specified
		what |= CAPTURY_STREAM_ANGLES;
		packet->numAngles = numAngles;
		memcpy(packet->angles, angles, numAngles*2);
	} else
		what &= ~CAPTURY_STREAM_ANGLES; // disable angle streaming if no angles are specified

	if ((what & CAPTURY_STREAM_LOCAL_POSES) == CAPTURY_STREAM_LOCAL_POSES) {
		getLocalPoses = true;
		what &= ~(CAPTURY_STREAM_LOCAL_POSES ^ CAPTURY_STREAM_GLOBAL_POSES);
	} else
		getLocalPoses = false;

	packet->what = what;
	packet->cameraId = camId;

	stopStreamThread = 0;
#ifdef WIN32
	streamThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)streamLoop, packet, 0, NULL);
#else
	pthread_create(&streamThread, NULL, streamLoop, packet);
#endif

	return 1;
}

// returns 1 if successful, 0 otherwise
extern "C" int Captury_stopStreaming(int wait)
{
	if (sock == -1)
		return 0;

	stopStreamThread = 1;

	if (wait) {
#ifdef WIN32
		WaitForSingleObject(streamThread, 1000);
#else
		void* retVal;
		pthread_join(streamThread, &retVal);
#endif
	}

	return 1;
}

// fills the pose with the current pose for the given actor
// the client is responsible for providing sufficient space (actor->numJoints*6) in pose->values
// returns 1 if successful, 0 otherwise
extern "C" CapturyPose* Captury_getCurrentPoseForActor(int actorId)
{
	return Captury_getCurrentPoseAndTrackingConsistencyForActor(actorId, nullptr);
}

extern "C" CapturyPose* Captury_getCurrentPoseAndTrackingConsistencyForActor(int actorId, int* tc)
{
	// check whether any actor changed status
	uint64_t now = getTime() - 500000; // half a second ago
	bool stillCurrent = true;
	lockMutex(&mutex);
	std::vector<int> stoppedActorIds;
	for (std::unordered_map<int, ActorData>::iterator it = actorData.begin(); it != actorData.end(); ++it) {
		if (it->second.lastPoseTimestamp > now) // still current
			continue;

		if (it->second.status == ACTOR_SCALING || it->second.status == ACTOR_TRACKING) {
			it->second.status = ACTOR_STOPPED;
			if (actorChangedCallback)
				stoppedActorIds.push_back(it->first);
			if (it->first == actorId)
				stillCurrent = false;
		}
	}

	unlockMutex(&mutex);
	for (int id : stoppedActorIds)
		actorChangedCallback(id, ACTOR_STOPPED);
	lockMutex(&mutex);

	if (!stillCurrent) {
		lastErrorMessage = "actor has disappeared";
		unlockMutex(&mutex);
		return NULL;
	}

	// uint64_t now = getTime();
	std::unordered_map<int, ActorData>::iterator it = actorData.find(actorId);
	if (it == actorData.end()) {
		char buf[400];
		snprintf(buf, 400, "Requested pose for unknown actor %d, have poses ", actorId);
		lastErrorMessage = buf;
		for (it = actorData.begin(); it != actorData.end(); ++it) {
			snprintf(buf, 400, "%d ", it->first);
			lastErrorMessage += buf;
		}
		unlockMutex(&mutex);
		return NULL;
	}

	if (it->second.currentPose.numTransforms == 0 && it->second.currentPose.numBlendShapes == 0) {
		unlockMutex(&mutex);
		lastErrorMessage = "most recent pose is empty";
		return NULL;
	}

	CapturyPose* pose = (CapturyPose*)malloc(sizeof(CapturyPose) + it->second.currentPose.numTransforms * sizeof(CapturyTransform) + it->second.currentPose.numBlendShapes*sizeof(float));
	pose->actor = actorId;
	pose->timestamp = it->second.currentPose.timestamp;
	pose->numTransforms = it->second.currentPose.numTransforms;
	pose->transforms = (CapturyTransform*)&pose[1];
	pose->numBlendShapes = it->second.currentPose.numBlendShapes;
	pose->blendShapeActivations = (float*)(((CapturyTransform*)&pose[1]) + pose->numTransforms);

	memcpy(pose->transforms, it->second.currentPose.transforms, sizeof(CapturyTransform) * pose->numTransforms);
	memcpy(pose->blendShapeActivations, it->second.currentPose.blendShapeActivations, sizeof(float) * pose->numBlendShapes);
	unlockMutex(&mutex);

	return pose;
}

extern "C" CapturyPose* Captury_getCurrentPose(int actorId)
{
	int tc;
	return Captury_getCurrentPoseAndTrackingConsistencyForActor(actorId, &tc);
}

extern "C" CapturyAngleData* Captury_getCurrentAngles(int actorId, int* numAngles)
{
	if (currentAngles.count(actorId)) {
		if (numAngles != nullptr)
			*numAngles = (int)currentAngles[actorId].size();
		return currentAngles[actorId].data();
	} else {
		if (numAngles != nullptr)
			*numAngles = 0;
		return nullptr;
	}
}

extern "C" CapturyPose* Captury_getCurrentPoseAndTrackingConsistency(int actorId, int* tc)
{
	return Captury_getCurrentPoseAndTrackingConsistencyForActor(actorId, tc);
}

// simple function for releasing memory of a pose
extern "C" void Captury_freePose(CapturyPose* pose)
{
	if (pose != NULL)
		free(pose);
}

extern "C" int Captury_getActorStatus(int actorId)
{
	lockMutex(&mutex);
	std::unordered_map<int, ActorData>::iterator it = actorData.find(actorId);
	if (it == actorData.end()) {
		unlockMutex(&mutex);
		return ACTOR_UNKNOWN;
	}

	CapturyActorStatus status = it->second.status;
	unlockMutex(&mutex);

	return status;
}

extern "C" CapturyARTag* Captury_getCurrentARTags()
{
	uint64_t now = getTime();
	lockMutex(&mutex);
	if (now > arTagsTime + 100000) { // 100ms
		unlockMutex(&mutex);
		return NULL;
	}
	int numARTags = (int)arTags.size();
	CapturyARTag* artags = (CapturyARTag*)malloc(sizeof(CapturyARTag) * (numARTags+1));
	memcpy(artags, &arTags[0], sizeof(CapturyARTag) * numARTags);
	unlockMutex(&mutex);

	artags[numARTags].id = -1;
	return artags;
}


extern "C" void Captury_freeARTags(CapturyARTag* artags)
{
	if (artags != NULL)
		free(artags);
}


// requests an update of the texture for the given actor. non-blocking
// returns 1 if successful otherwise 0
extern "C" int Captury_requestTexture(int actorId)
{
	if (sock == -1)
		return 0;

	CapturyGetImagePacket packet;
	packet.type = capturyGetImage;
	packet.size = sizeof(packet);
	packet.actor = actorId;

//	log("requesting texture for actor %x\n", actor->id);

	if (!sendPacket((CapturyRequestPacket*)&packet, capturyImageHeader))
		return 0;

	return 1;
}

// returns a texture image of the specified actor
extern "C" CapturyImage* Captury_getTexture(int actorId)
{
	// check if we don't have a texture yet
	lockMutex(&mutex);
	std::unordered_map<int, ActorData>::iterator it = actorData.find(actorId);
	if (it == actorData.end()) {
		unlockMutex(&mutex);
		return 0;
	}

	// create a copy of all data
	const int size = it->second.currentTextures.width * it->second.currentTextures.height * 3;
	CapturyImage* image = (CapturyImage*) malloc(sizeof(CapturyImage) + size);
	image->width = it->second.currentTextures.width;
	image->height = it->second.currentTextures.height;
	image->camera = -1;
	image->timestamp = 0;
	image->data = (unsigned char*)&image[1];
	image->gpuData = nullptr;

	memcpy(image->data, it->second.currentTextures.data, size);
	unlockMutex(&mutex);

	return image;
}

// simple function for releasing memory of an image
extern "C" void Captury_freeImage(CapturyImage* image)
{
	if (image != NULL)
		free(image);
}


// requests an update of the texture for the given actor. blocking
// returns 1 if successful otherwise 0
extern "C" uint64_t Captury_getMarkerTransform(int actorId, int joint, CapturyTransform* trafo)
{
	if (sock == -1)
		return 0;

	if (joint < 0)
		return 0;

	if (trafo == NULL)
		return 0;

	CapturyGetMarkerTransformPacket packet;
	packet.type = capturyGetMarkerTransform;
	packet.size = sizeof(packet);
	packet.actor = actorId;
	packet.joint = joint;

	//log("requesting marker transform for actor.joint %d.%d\n", actor->id, joint);

	if (!sendPacket((CapturyRequestPacket*)&packet, capturyMarkerTransform))
		return 0;

	ActorAndJoint aj(actorId, joint);
	if (markerTransforms.count(aj) == 0)
		return 0;

	*trafo = markerTransforms[aj].trafo;

	return getRemoteTime(markerTransforms[aj].timestamp);
}

extern "C" int Captury_getScalingProgress(int actorId)
{
	lockMutex(&mutex);
	int scaling = actorData.count(actorId) ? actorData[actorId].scalingProgress : 0;
	unlockMutex(&mutex);
	return scaling;
}

extern "C" int Captury_getTrackingQuality(int actorId)
{
	lockMutex(&mutex);
	auto it = actorData.find(actorId);
	if (it == actorData.end()) {
		unlockMutex(&mutex);
		return 0;
	}
	int quality = it->second.trackingQuality;
	unlockMutex(&mutex);

	return quality;
}

// change the name of the actor
extern "C" int Captury_setActorName(int actorId, const char* name)
{
	if (sock == -1)
		return 0;

	CapturySetActorNamePacket packet;
	packet.type = capturySetActorName;
	packet.size = sizeof(packet);
	packet.actor = actorId;
	strncpy(packet.name, name, 32);
	packet.name[31] = '\0';

	if (!sendPacket((CapturyRequestPacket*)&packet, capturySetActorNameAck))
		return 0;

	return 1;
}

// fills the pointers with the current day, session, shot tuple that is used in CapturyLive to identify a shot
// the strings are owned by the library - do not free or overwrite
// returns 1 if successful, 0 otherwise
extern "C" int Captury_getCurrentDaySessionShot(const char** day, const char** session, const char** shot)
{
	if (sock == -1 || day == NULL || session == NULL || shot == NULL)
		return 0;

	CapturyRequestPacket packet;
	packet.type = capturyDaySessionShot;
	packet.size = sizeof(packet);

	if (!sendPacket(&packet, capturyDaySessionShot))
		return 0;

	*day = &currentDay[0];
	*session = &currentSession[0];
	*shot = &currentShot[0];

	return 1;
}

// sets the shot name for the next recording
// returns 1 if successful, 0 otherwise
extern "C" int Captury_setShotName(const char* name)
{
	if (sock == -1 || name == NULL)
		return 0;

	if (strlen(name) > 99)
		return 0;

	CapturySetShotPacket packet;
	packet.type = capturySetShot;
	packet.size = sizeof(packet);
	strncpy(packet.shot, name, sizeof(packet.shot));

	if (!sendPacket((CapturyRequestPacket*)&packet, capturySetShotAck))
		return 0;

	return 1;
}

// you have to set the shot name before starting to record - or make sure that it has been set using CapturyLive
// returns 1 if successful, 0 otherwise
extern "C" int64_t Captury_startRecording()
{
	if (sock == -1)
		return 0;

	CapturyRequestPacket packet;
	packet.type = capturyStartRecording2;
	packet.size = sizeof(packet);

	startRecordingTime = 0;

	if (!sendPacket(&packet, capturyStartRecordingAck2))
		return 0;

	for (int i = 0; i < 100; ++i) {
		sleepMicroSeconds(1000);
		if (startRecordingTime != 0)
			return startRecordingTime;
	}

	return startRecordingTime;
}

// returns 1 if successful, 0 otherwise
extern "C" int Captury_stopRecording()
{
	if (sock == -1)
		return 0;

	CapturyRequestPacket packet;
	packet.type = capturyStopRecording;
	packet.size = sizeof(packet);

	if (!sendPacket(&packet, capturyStopRecordingAck))
		return 0;

	return 1;
}

#ifdef WIN32
static DWORD WINAPI syncLoop(void* arg)
#else
static void* syncLoop(void* arg)
#endif
{
	CapturyTimePacket2 packet;
	packet.type = capturyGetTime2;
	packet.size = sizeof(packet);

	while (true) {
		++nextTimeId;
		packet.timeId = nextTimeId;

		pingTime = getTime();
		sendPacket((CapturyRequestPacket*)&packet, capturyTime2);

		sleepMicroSeconds(1000000);
	}
}

extern "C" void Captury_startTimeSynchronizationLoop()
{
	if (syncLoopIsRunning)
		return;

#ifdef WIN32
	syncThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)syncLoop, NULL, 0, NULL);
#else
	pthread_create(&syncThread, NULL, syncLoop, NULL);
#endif

	syncLoopIsRunning = true;
}

extern "C" uint64_t Captury_synchronizeTime()
{
	CapturyTimePacket2 packet;
	packet.type = capturyGetTime2;
	packet.size = sizeof(packet);
	++nextTimeId;
	packet.timeId = nextTimeId;

	pingTime = getTime();

	if (!sendPacket((CapturyRequestPacket*)&packet, capturyTime2)) {
		CapturyRequestPacket req;
		req.type = capturyGetTime;
		req.size = sizeof(req);
		if (!sendPacket((CapturyRequestPacket*)&req, capturyTime))
			return 0;
	}

	return Captury_getTime();
}

extern "C" int64_t Captury_getTimeOffset()
{
	lockMutex(&syncMutex);
	int64_t offset = (int64_t)currentSync.offset;
	unlockMutex(&syncMutex);
	return offset;
}

extern "C" int Captury_setHalfplaneConstraint(int actorId, int jointIndex, float* originOffset, float* normal, float offset, uint64_t timestamp, float weight)
{
	CapturyConstraintPacket ccp;
	ccp.type = capturyConstraint;
	ccp.size = sizeof(ccp);
	ccp.constrType = CAPTURY_CONSTRAINT_HALF_PLANE;
	ccp.originActor = actorId;
	ccp.originJoint = jointIndex;
	ccp.originOffset[0] = originOffset[0];
	ccp.originOffset[1] = originOffset[1];
	ccp.originOffset[2] = originOffset[2];
	ccp.targetActor = -1;
	ccp.targetJoint = -1;
	ccp.targetVector[0] = normal[0];
	ccp.targetVector[1] = normal[1];
	ccp.targetVector[2] = normal[2];
	ccp.targetValue = offset;
	ccp.targetRotation[0] = 0.0;
	ccp.targetRotation[1] = 0.0;
	ccp.targetRotation[2] = 0.0;
	ccp.targetRotation[3] = 0.0;
	ccp.weight = weight;

	if (!sendPacket((CapturyRequestPacket*)&ccp, capturyError))
		return 0;

	return 1;
}

extern "C" int Captury_setRotationConstraint(int actorId, int jointIndex, float* rotation, uint64_t timestamp, float weight)
{
	CapturyConstraintPacket ccp;
	ccp.type = capturyConstraint;
	ccp.size = sizeof(ccp);
	ccp.constrType = CAPTURY_CONSTRAINT_ROTATION;
	ccp.originActor = actorId;
	ccp.originJoint = jointIndex;
	ccp.originOffset[0] = 0.0f;
	ccp.originOffset[1] = 0.0f;
	ccp.originOffset[2] = 0.0f;
	ccp.targetActor = -1;
	ccp.targetJoint = -1;
	ccp.targetVector[0] = 0.0f;
	ccp.targetVector[1] = 0.0f;
	ccp.targetVector[2] = 0.0f;
	ccp.targetRotation[0] = rotation[0];
	ccp.targetRotation[1] = rotation[1];
	ccp.targetRotation[2] = rotation[2];
	ccp.targetRotation[3] = rotation[3];
	ccp.weight = weight;

	if (!sendPacket((CapturyRequestPacket*)&ccp, capturyError))
		return 0;

	return 1;
}

extern "C" int Captury_setFixedAxisConstraint(int actorId, int jointIndex, float* axis, float* targetAxis, uint64_t timestamp, float weight)
{
	CapturyConstraintPacket ccp;
	ccp.type = capturyConstraint;
	ccp.size = sizeof(ccp);
	ccp.constrType = CAPTURY_CONSTRAINT_FIXED_AXIS;
	ccp.originActor = actorId;
	ccp.originJoint = jointIndex;
	ccp.originOffset[0] = axis[0];
	ccp.originOffset[1] = axis[1];
	ccp.originOffset[2] = axis[2];
	ccp.targetActor = -1;
	ccp.targetJoint = -1;
	ccp.targetVector[0] = targetAxis[0];
	ccp.targetVector[1] = targetAxis[1];
	ccp.targetVector[2] = targetAxis[2];
	ccp.targetRotation[0] = 0.0;
	ccp.targetRotation[1] = 0.0;
	ccp.targetRotation[2] = 0.0;
	ccp.targetRotation[3] = 0.0;
	ccp.weight = weight;

	if (!sendPacket((CapturyRequestPacket*)&ccp, capturyError))
		return 0;

	return 1;
}

extern "C" int Captury_setOffsetConstraint(int originActorId, int originJointIndex, float* originOffset, int targetActorId, int targetJointIndex, float* targetOffset, float* offset, uint64_t timestamp, float weight)
{
	CapturyConstraintPacket ccp;
	ccp.type = capturyConstraint;
	ccp.size = sizeof(ccp);
	ccp.constrType = CAPTURY_CONSTRAINT_OFFSET;
	ccp.originActor = originActorId;
	ccp.originJoint = originJointIndex;
	ccp.originOffset[0] = originOffset[0];
	ccp.originOffset[1] = originOffset[1];
	ccp.originOffset[2] = originOffset[2];
	ccp.targetActor = targetActorId;
	ccp.targetJoint = targetJointIndex;
	ccp.targetOffset[0] = targetOffset[0];
	ccp.targetOffset[1] = targetOffset[1];
	ccp.targetOffset[2] = targetOffset[2];
	ccp.targetVector[0] = offset[0];
	ccp.targetVector[1] = offset[1];
	ccp.targetVector[2] = offset[2];
	ccp.targetRotation[0] = 0.0;
	ccp.targetRotation[1] = 0.0;
	ccp.targetRotation[2] = 0.0;
	ccp.targetRotation[3] = 0.0;
	ccp.weight = weight;

	if (!sendPacket((CapturyRequestPacket*)&ccp, capturyError))
		return 0;

	return 1;
}

int Captury_setDistanceConstraint(int originActorId, int originJointIndex, float* originOffset, int targetActorId, int targetJointIndex, float* targetOffset, float distance, uint64_t timestamp, float weight)
{
	CapturyConstraintPacket ccp;
	ccp.type = capturyConstraint;
	ccp.size = sizeof(ccp);
	ccp.constrType = CAPTURY_CONSTRAINT_DISTANCE;
	ccp.originActor = originActorId;
	ccp.originJoint = originJointIndex;
	ccp.originOffset[0] = originOffset[0];
	ccp.originOffset[1] = originOffset[1];
	ccp.originOffset[2] = originOffset[2];
	ccp.targetActor = targetActorId;
	ccp.targetJoint = targetJointIndex;
	ccp.targetOffset[0] = targetOffset[0];
	ccp.targetOffset[1] = targetOffset[1];
	ccp.targetOffset[2] = targetOffset[2];
	ccp.targetValue = distance;
	ccp.targetRotation[0] = 0.0;
	ccp.targetRotation[1] = 0.0;
	ccp.targetRotation[2] = 0.0;
	ccp.targetRotation[3] = 0.0;
	ccp.weight = weight;

	if (!sendPacket((CapturyRequestPacket*)&ccp, capturyError))
		return 0;

	return 1;
}

int Captury_setRelativeRotationConstraint(int originActorId, int originJointIndex, int targetActorId, int targetJointIndex, float* rotation, uint64_t timestamp, float weight)
{
	CapturyConstraintPacket ccp;
	ccp.type = capturyConstraint;
	ccp.size = sizeof(ccp);
	ccp.constrType = CAPTURY_CONSTRAINT_ROTATION;
	ccp.originActor = originActorId;
	ccp.originJoint = originJointIndex;
	ccp.targetActor = targetActorId;
	ccp.targetJoint = targetJointIndex;
	ccp.targetVector[0] = 0.0f;
	ccp.targetVector[1] = 0.0f;
	ccp.targetVector[2] = 0.0f;
	ccp.targetRotation[0] = rotation[0];
	ccp.targetRotation[1] = rotation[1];
	ccp.targetRotation[2] = rotation[2];
	ccp.targetRotation[3] = rotation[3];
	ccp.weight = weight;

	if (!sendPacket((CapturyRequestPacket*)&ccp, capturyError))
		return 0;

	return 1;
}

int Captury_sendCustomPacket(char* pluginName, int size, void* data)
{
	std::vector<unsigned char> buffer(sizeof(CapturyCustomPacket) + size);
	CapturyCustomPacket* ccp = (CapturyCustomPacket*) &buffer[0];
	ccp->type = capturyCustom;
	ccp->size = (int)buffer.size();
	strncpy(ccp->name, pluginName, 15);
	ccp->name[15] = '\0';
	memcpy(ccp->data, data, size);

	if (!sendPacket((CapturyRequestPacket*)&ccp, capturyCustomAck))
		return 0;

	return 1;
}


int Captury_registerNewPoseCallback(CapturyNewPoseCallback callback)
{
	if (newPoseCallback != NULL) { // callback already exists
		if (callback == NULL) { // remove callback
			newPoseCallback = NULL;
			return 1;
		} else
			return 0;
	}

	if (callback == NULL) // trying to erase callback that is not there
		return 0;

	newPoseCallback = callback;
	return 1;
}


int Captury_registerNewAnglesCallback(CapturyNewAnglesCallback callback)
{
	if (newAnglesCallback != NULL) { // callback already exists
		if (callback == NULL) { // remove callback
			newAnglesCallback = NULL;
			return 1;
		} else
			return 0;
	}

	if (callback == NULL) // trying to erase callback that is not there
		return 0;

	newAnglesCallback = callback;
	return 1;
}


int Captury_registerActorChangedCallback(CapturyActorChangedCallback callback)
{
	if (actorChangedCallback != NULL) { // callback already exists
		if (callback == NULL) { // remove callback
			actorChangedCallback = NULL;
			return 1;
		} else
			return 0;
	}

	if (callback == NULL) // trying to erase callback that is not there
		return 0;

	actorChangedCallback = callback;
	return 1;
}


int Captury_registerARTagCallback(CapturyARTagCallback callback)
{
	if (arTagCallback != NULL) { // callback already exists
		if (callback == NULL) { // remove callback
			arTagCallback = NULL;
			return 1;
		} else
			return 0;
	}

	if (callback == NULL) // trying to erase callback that is not there
		return 0;

	arTagCallback = callback;
	return 1;
}


int Captury_registerImageStreamingCallback(CapturyImageCallback callback)
{
	if (imageCallback != NULL) {
		if (callback == NULL) { // callback already exists
			imageCallback = NULL;
			return 1;
		}
		// remove existing callback
	} else if (callback == NULL) // trying to erase callback that is not there
		return 0;

	imageCallback = callback;

	return 1;
}


int Captury_registerCustomPacketCallback(const char* pluginName, CapturyCustomPacketCallback callback)
{
	if (callbacks.count(pluginName)) { // callback already exists
		if (callback == NULL) { // remove callback
			callbacks.erase(pluginName);
			return 1;
		} else
			return 0;
	}

	if (callback == NULL) // trying to erase callback that is not there
		return 0;

	callbacks.insert(std::make_pair(pluginName, callback));
	return 1;
}

#ifndef DEG2RADf
#define DEG2RADf		(0.0174532925199432958f)
#endif
#ifndef RAD2DEGf
#define RAD2DEGf		(57.29577951308232088f)
#endif

// initialize complete 4x4 matrix with rotation from euler angles and translation from translation vector
static float* transformationMatrix(const float* eulerAngles, const float* translation, float* m4x4) // checked
{
	float c3 = std::cos(eulerAngles[0] * DEG2RADf);
	float s3 = std::sin(eulerAngles[0] * DEG2RADf);
	float c2 = std::cos(eulerAngles[1] * DEG2RADf);
	float s2 = std::sin(eulerAngles[1] * DEG2RADf);
	float c1 = std::cos(eulerAngles[2] * DEG2RADf);
	float s1 = std::sin(eulerAngles[2] * DEG2RADf);

	m4x4[0]  = c1*c2; m4x4[1]  = c1*s2*s3-c3*s1; m4x4[2]  = s1*s3+c1*c3*s2; m4x4[3]  = translation[0];
	m4x4[4]  = c2*s1; m4x4[5]  = c1*c3+s1*s2*s3; m4x4[6]  = c3*s1*s2-c1*s3; m4x4[7]  = translation[1];
	m4x4[8]  = -s2;   m4x4[9]  = c2*s3;          m4x4[10] = c2*c3;          m4x4[11] = translation[2];
	m4x4[12] = 0.0f;  m4x4[13] = 0.0f;           m4x4[14] = 0.0f;           m4x4[15] = 1.0f;

	return m4x4;
}

// out = m1^-1
// m1 = [r1, t1; 0 0 0 1]
// m1^-1 = [r1', -r1'*t1; 0 0 0 1]
static float* matrixInv(const float* m4x4, float* out) // checked
{
	out[0]  = m4x4[0]; out[1] = m4x4[4]; out[2]  = m4x4[8];
	out[4]  = m4x4[1]; out[5] = m4x4[5]; out[6]  = m4x4[9];
	out[8]  = m4x4[2]; out[9] = m4x4[6]; out[10] = m4x4[10];
	out[3]  = -(out[0]*m4x4[3] + out[1]*m4x4[7] + out[2]*m4x4[11]);
	out[7]  = -(out[4]*m4x4[3] + out[5]*m4x4[7] + out[6]*m4x4[11]);
	out[11] = -(out[8]*m4x4[3] + out[9]*m4x4[7] + out[10]*m4x4[11]);

	out[12] = 0.0f; out[13] = 0.0f; out[14] = 0.0f; out[15] = 1.0f;

	return out;
}

// out = m1 * m2
static float* matrixMatrix(const float* m1, const float* m2, float* out) // checked
{
	out[0]  = m1[0]*m2[0] + m1[1]*m2[4] + m1[2]*m2[8];
	out[1]  = m1[0]*m2[1] + m1[1]*m2[5] + m1[2]*m2[9];
	out[2]  = m1[0]*m2[2] + m1[1]*m2[6] + m1[2]*m2[10];
	out[3]  = m1[0]*m2[3] + m1[1]*m2[7] + m1[2]*m2[11] + m1[3];

	out[4]  = m1[4]*m2[0] + m1[5]*m2[4] + m1[6]*m2[8];
	out[5]  = m1[4]*m2[1] + m1[5]*m2[5] + m1[6]*m2[9];
	out[6]  = m1[4]*m2[2] + m1[5]*m2[6] + m1[6]*m2[10];
	out[7]  = m1[4]*m2[3] + m1[5]*m2[7] + m1[6]*m2[11] + m1[7];

	out[8]  = m1[8]*m2[0] + m1[9]*m2[4] + m1[10]*m2[8];
	out[9]  = m1[8]*m2[1] + m1[9]*m2[5] + m1[10]*m2[9];
	out[10] = m1[8]*m2[2] + m1[9]*m2[6] + m1[10]*m2[10];
	out[11] = m1[8]*m2[3] + m1[9]*m2[7] + m1[10]*m2[11] + m1[11];

	out[12] = 0.0f; out[13] = 0.0f; out[14] = 0.0f; out[15] = 1.0f;

	return out;
}

static void decompose(const float* mat, float* euler) // checked
{
	euler[1] = -std::asin(mat[8]);
	float C =  std::cos(euler[1]);
	if (std::fabs(C) > 0.005) {
		euler[2] = std::atan2(mat[4] / C, mat[0]  / C) * RAD2DEGf;
		euler[0] = std::atan2(mat[9] / C, mat[10] / C) * RAD2DEGf;
	} else {
		euler[2] = 0;
		if (mat[8] < 0)
			euler[0] = std::atan2((mat[1]-mat[6])*0.5f, (mat[5]+mat[2])*0.5f) * RAD2DEGf;
		else
			euler[0] = std::atan2((mat[1]+mat[6])*0.5f, (mat[5]-mat[2])*0.5f) * RAD2DEGf;
	}
	euler[1] *= RAD2DEGf;
}

// static void dumpMatrix(const float* mat)
// {
// 	log("%.4f %.4f %.4f  %.4f\n", mat[0], mat[1], mat[2], mat[3]);
// 	log("%.4f %.4f %.4f  %.4f\n", mat[4], mat[5], mat[6], mat[7]);
// 	log("%.4f %.4f %.4f  %.4f\n", mat[8], mat[9], mat[10], mat[11]);
// 	log("%.4f %.4f %.4f  %.4f\n", mat[12], mat[13], mat[14], mat[15]);
// }

void Captury_convertPoseToLocal(CapturyPose* pose, int actorId) REQUIRES(mutex)
{
	if (actorsById.count(actorId) == 0)
		return;

	CapturyActor* actor = actorsById[actorId].get();
	CapturyTransform* at = pose->transforms;
	float* matrices = (float*)malloc(sizeof(float) * 16 * actor->numJoints);
	for (int i = 0; i < actor->numJoints; ++i, ++at) {
		transformationMatrix(at->rotation, at->translation, &matrices[i*16]);
// 		float out[6];
// 		decompose(&matrices[i*16], out+3);
// 		log("% .4f % .4f % .4f\n", at[3], at[4], at[5]);
// 		log("% .4f % .4f % .4f\n", out[3], out[4], out[5]);
// 		float test[16];
// 		transformationMatrix(out+3, at, test);
// 		dumpMatrix(&matrices[i*16]);
// 		dumpMatrix(test);
		if (i == 0 || actor->joints[i].parent == -1) { // copy global pose for root joint
			; // nothing to be done here - the values stay the same
		} else {
			float inv[16];
			matrixInv(&matrices[actor->joints[i].parent*16], inv);
			float local[16];
			matrixMatrix(inv, &matrices[i*16], local);

			at->translation[0] = local[3]; // set translation
			at->translation[1] = local[7];
			at->translation[2] = local[11];
			decompose(local, at->rotation);

// 			float test[16];
// 			float test2[16];
// 			transformationMatrix(at+3, at, test);
// 			matrixMatrix(&matrices[actor->joints[i].parent*16], test, test2);
// 			dumpMatrix(&matrices[i*16]);
// 			dumpMatrix(test2);
		}
	}

	free(matrices);
}


extern "C" int Captury_snapActor(float x, float z, float heading)
{
	return Captury_snapActorEx(x, z, 1000.0f, heading, "", SNAP_DEFAULT, 0);
}

extern "C" int Captury_snapActorEx(float x, float z, float radius, float heading, const char* skeletonName, int snapMethod, int quickScaling)
{
	if (sock == -1) {
		lastErrorMessage = "socket is not open";
		return 0;
	}

	if (snapMethod < SNAP_BACKGROUND_LOCAL || snapMethod > SNAP_DEFAULT) {
		lastErrorMessage = "invalid parameter: snapMethod";
		return 0;
	}

	CapturySnapActorPacket2 packet;
	packet.type = capturySnapActor;
	packet.size = sizeof(packet);
	packet.x = x;
	packet.z = z;
	packet.radius = radius;
	packet.heading = heading;
	packet.snapMethod = snapMethod;
	packet.quickScaling = quickScaling;
	strncpy(packet.skeletonName, skeletonName, 32);
	packet.skeletonName[31] = '\0';

	if (!sendPacket((CapturyRequestPacket*)&packet, capturySnapActorAck))
		return 0;

	return 1;
}

extern "C" int Captury_startTracking(int actorId, float x, float z, float heading)
{
	if (sock == -1)
		return 0;

	CapturyStartTrackingPacket packet;
	packet.type = capturyStartTracking;
	packet.size = sizeof(packet);
	packet.actor = actorId;
	packet.x = x;
	packet.z = z;
	packet.heading = heading;

	if (!sendPacket((CapturyRequestPacket*)&packet, capturyStartTrackingAck))
		return 0;

	return 1;
}

extern "C" int Captury_rescaleActor(int actorId)
{
	if (sock == -1)
		return 0;

	CapturyStopTrackingPacket packet;
	packet.type = capturyRescaleActor;
	packet.size = sizeof(packet);
	packet.actor = actorId;

	if (!sendPacket((CapturyRequestPacket*)&packet, capturyRescaleActorAck))
		return 0;

	return 1;
}

extern "C" int Captury_recolorActor(int actorId)
{
	if (sock == -1)
		return 0;

	CapturyStopTrackingPacket packet;
	packet.type = capturyRecolorActor;
	packet.size = sizeof(packet);
	packet.actor = actorId;

	if (!sendPacket((CapturyRequestPacket*)&packet, capturyRecolorActorAck))
		return 0;

	return 1;
}

extern "C" int Captury_updateActorColors(int actorId)
{
	if (sock == -1)
		return 0;

	CapturyStopTrackingPacket packet;
	packet.type = capturyUpdateActorColors;
	packet.size = sizeof(packet);
	packet.actor = actorId;

	if (!sendPacket((CapturyRequestPacket*)&packet, capturyRecolorActorAck))
		return 0;

	return 1;
}

extern "C" int Captury_stopTracking(int actorId)
{
	if (sock == -1)
		return 0;

	CapturyStopTrackingPacket packet;
	packet.type = capturyStopTracking;
	packet.size = sizeof(packet);
	packet.actor = actorId;

	if (!sendPacket((CapturyRequestPacket*)&packet, capturyStopTrackingAck))
		return 0;

	return 1;
}

extern "C" int Captury_deleteActor(int actorId)
{
	if (sock == -1)
		return 0;

	CapturyStopTrackingPacket packet;
	packet.type = capturyDeleteActor;
	packet.size = sizeof(packet);
	packet.actor = actorId;

	if (!sendPacket((CapturyRequestPacket*)&packet, capturyDeleteActorAck))
		return 0;

	return 1;
}

extern "C" int Captury_getBackgroundQuality()
{
	if (sock == -1)
		return -1;

	CapturyRequestPacket packet;
	packet.type = capturyGetBackgroundQuality;
	packet.size = sizeof(packet);

	if (!sendPacket((CapturyRequestPacket*)&packet, capturyBackgroundQuality))
		return -1;

	return backgroundQuality;
}

extern "C" int Captury_captureBackground(CapturyBackgroundFinishedCallback callback, void* userData)
{
	if (sock == -1)
		return 0;

	CapturyRequestPacket packet;
	packet.type = capturyCaptureBackground;
	packet.size = sizeof(packet);

	backgroundFinishedCallback = callback;
	backgroundFinishedCallbackUserData = userData;

	if (!sendPacket((CapturyRequestPacket*)&packet, capturyCaptureBackgroundAck))
		return 0;

	return 1;
}

extern "C" const char* Captury_getStatus()
{
	if (sock == -1)
		return 0;

	CapturyRequestPacket packet;
	packet.type = capturyGetStatus;
	packet.size = sizeof(packet);

	if (!sendPacket((CapturyRequestPacket*)&packet, capturyStatus))
		return 0;

	return lastStatusMessage.c_str();
}

extern "C" int Captury_getCurrentLatency(CapturyLatencyInfo* latencyInfo)
{
	if (latencyInfo == nullptr)
		return 0;

	latencyInfo->firstImagePacketTime = currentLatency.firstImagePacket;
	latencyInfo->optimizationStartTime = currentLatency.optimizationStart;
	latencyInfo->optimizationEndTime = currentLatency.optimizationEnd;
	latencyInfo->poseSentTime = currentLatency.sendPacketTime;
	latencyInfo->poseReceivedTime = receivedPoseTime;
	latencyInfo->timestampOfCorrespondingPose = receivedPoseTimestamp;

	return 1;
}

#endif
