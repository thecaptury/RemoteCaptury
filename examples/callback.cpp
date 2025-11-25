#include <stdio.h>
#include "RemoteCaptury.h"
#ifndef _WIN32
#include <unistd.h>
#endif

// This example uses the callback interface to get the current pose of all actors from CapturyLive



// This callback is called every time a new pose becomes available.
// Make sure this callback is quick. Otherwise frames may get dropped.
void newPoseCallback(RemoteCaptury* rc, CapturyActor* actor, CapturyPose* pose, int trackingQuality, void* userArg)
{
	Captury_log(rc, CAPTURY_LOG_INFO, "actor %x has a new pose now\n", actor->id);
}

// this is called whenever an actor changes state (starts or stops tracking or finishes scaling or is deleted).
// Make sure this callback is quick. Otherwise frames may get dropped.
void actorChangedCallback(RemoteCaptury* rc, int actorId, int mode, void* userArg)
{
	Captury_log(rc, CAPTURY_LOG_INFO, "actor %x has state %s now\n", actorId, CapturyActorStatusString[mode]);

	switch (mode) {
	case ACTOR_SCALING:
		break;
	case ACTOR_TRACKING:
		break;
	case ACTOR_STOPPED:
		break;
	case ACTOR_DELETED:
		break;
	case ACTOR_UNKNOWN:
		break;
	}

	const CapturyActor* actor = Captury_getActor(rc, actorId);
	// do something with it

	// free it
	Captury_freeActor(rc, actor);
}

int main(int argc, char** argv)
{
	// This is cheating a little as we're not connecting to a remote machine
	// but you can change the IP. There is also a more advanced version of Captury_connect() that gives you more options
	// to control ports or to enable asynchronous connect mode.
	
	RemoteCaptury* rc = Captury_create();
	
	int ret = Captury_connect(rc, "127.0.0.1", 2101);
	if (ret == 0)
		return -1;

	// This is optional but sometimes it's quite useful to know the exact time on the CapturyLive machine.
	// The accuracy of the synchronization depends primarily on your OS and the network topology between
	// the CapturyLive machine and this machine. On a LAN running Windows on the target machine you can
	// expect about 100us accuracy.
	Captury_startTimeSynchronizationLoop(rc);

	// register callback that is called in a different thread whenever a new pose becomes available
	ret = Captury_registerNewPoseCallback(rc, newPoseCallback, nullptr);
	if (ret == 0)
		return -1;

	ret = Captury_registerActorChangedCallback(rc, actorChangedCallback, nullptr);
	if (ret == 0)
		return -1;

	// start streaming
	// The recommended features are CAPTURY_STREAM_POSES and CAPTURY_STREAM_COMPRESSED.
	// Additional flags should be added as required.
	Captury_startStreaming(rc, CAPTURY_STREAM_POSES | CAPTURY_STREAM_COMPRESSED);

	#ifdef _WIN32
	Sleep(100000);
	#else
	usleep(100000*1000);
	#endif

	Captury_stopStreaming(rc);

	Captury_disconnect(rc);

	Captury_destroy(rc);

	return 0;
}
